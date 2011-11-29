/*WIKI* 


*WIKI*/
#include "MantidMDAlgorithms/ConvertToMDEvents.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/NumericAxis.h"

#include "MantidKernel/PhysicalConstants.h"

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ProgressText.h"
#include "MantidAPI/Progress.h"

#include "MantidMDEvents/MDEvent.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDAlgorithms/ConvertToMDEventsMethodsTemplate.h"

#include <algorithm>
#include <float.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
//using namespace Mantid::MDEvents;

namespace Mantid
{
namespace MDAlgorithms
{
    // the parameter, which specifies maximal default number of dimensions, the algorithm accepts (should be moved to configuration). 



// logger for loading workspaces  
   Kernel::Logger& ConvertToMDEvents::convert_log =Kernel::Logger::get("MD-Algorithms");

// the variable describes the locations of the preprocessed detectors, which can be stored and reused it the algorithm runs for more once;
preprocessed_detectors ConvertToMDEvents::det_loc;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToMDEvents)
  
/// Sets documentation strings for this algorithm
void ConvertToMDEvents::initDocs()
{
    this->setWikiSummary("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions. If the OutputWorkspace exists, then events are added to it.");
    this->setOptionalMessage("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions. If the OutputWorkspace exists, then events are added to it.");
}

    
//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToMDEvents::~ConvertToMDEvents()
{}


//
const double rad2deg = 180.0 / M_PI;
//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void 
ConvertToMDEvents::init()
{
      CompositeWorkspaceValidator<> *ws_valid = new CompositeWorkspaceValidator<>;
      ws_valid->add(new HistogramValidator<>);
      ws_valid->add(new InstrumentValidator<>);
      // the validator which checks if the workspace has axis and any units
      ws_valid->add(new WorkspaceUnitValidator<>(""));


    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,ws_valid),
        "An input Matrix Workspace 2D has to have units, which can be used as one of the dimensions ");
   
     declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace. If the workspace already exists, then the events will be added to it.");

     declareProperty("QDimensions",Q_ID_possible[0],new ListValidator(Q_ID_possible),
                              "You can select mod(Q) (1 dimension) or QxQyQz (3 dimensions) in Q space",Direction::InOut);        

     
    declareProperty(new ArrayProperty<std::string>("OtherDimensions",Direction::Input),
        " List(comma separated) of additional to Q (orthogonal) dimensions in the target workspace.\n"
        " The names of these dimensions have to coinside with the log names in the source workspace");
    // this property is mainly for subalgorithms to set-up as they have to identify 
    declareProperty(new PropertyWithValue<bool>("UsePreprocessedDetectors", true, Direction::Input), 
        "Store the part of the detectors transfromation into reciprocal space to save/reuse it later;");
 

    declareProperty(new ArrayProperty<double>("MinValues"),
         "An array of size 1+N_OtherDimensions if first dimension is equal |Q| or \n"
         "3+N_OtherDimensions if first (3) dimensions  QxQyQz containing minimal values for all dimensions"
         " Momentum values expected to be in [A^-1] and energy transfer (if any) expressed in [meV]\n"
         " All other values are in uints they are expressed in their log files\n"
         " Values lower then the specified one will be ignored\n"
         " If a minimal output workspace range is higer then specified, the workspace range will be used intstead)" );

   declareProperty(new ArrayProperty<double>("MaxValues"),
         "An array of the same size as MinValues array"
         " Values higher then the specified by the array will be ignored\n"
        " If a maximal output workspace ranges is lower, then one of specified, the workspace range will be used instead)" );

    // // this property is mainly for subalgorithms to set-up as they have to identify 
    //declareProperty(new PropertyWithValue<bool>("UsePreprocessedDetectors", true, Direction::Input), 
    //    "Store the part of the detectors transfromation into reciprocal space to save/reuse it later;");
    //// // this property is mainly for subalgorithms to set-up as they have to identify 
    ////declareProperty(new ArrayProperty<double>("u"), "first base vecotor");
    ////declareProperty(new ArrayProperty<double>("v"), "second base vecotors");  

}

void 
ConvertToMDEvents::check_max_morethen_min(const std::vector<double> &min,const std::vector<double> &max){
    for(size_t i=0; i<min.size();i++){
        if(max[i]<=min[i]){
            convert_log.error()<<" min value "<<min[i]<<" not less then max value"<<max[i]<<" in direction: "<<i<<std::endl;
            throw(std::invalid_argument("min limit not smaller then max limit"));
        }
    }
}
 
/// helper function to preprocess the detectors directions
void 
ConvertToMDEvents::process_detectors_positions(const DataObjects::Workspace2D_const_sptr inputWS)
{

    const size_t nHist = inputWS->getNumberHistograms();

    det_loc.det_dir.resize(nHist);
    det_loc.det_id.resize(nHist);
     // Loop over the spectra
   size_t ic(0);
   for (size_t i = 0; i < nHist; i++){

     Geometry::IDetector_const_sptr spDet;
     try{
        spDet= inputWS->getDetector(i);
     }catch(Kernel::Exception::NotFoundError &){
        continue;
     }
 
    // Check that we aren't dealing with monitor...
    if (spDet->isMonitor())continue;   

     det_loc.det_id[ic] = spDet->getID();
    // dist     =  spDet->getDistance(*sample);
     double polar    =  inputWS->detectorTwoTheta(spDet);
     double azim     =  spDet->getPhi();    

     double sPhi=sin(polar);
     double ez = cos(polar);
     double ex = sPhi*cos(azim);
     double ey = sPhi*sin(azim);
 
     det_loc.det_dir[ic].setX(ex);
     det_loc.det_dir[ic].setY(ey);
     det_loc.det_dir[ic].setZ(ez);

     ic++;
   }
   // 
   if(ic<nHist){
       det_loc.det_dir.resize(ic);
       det_loc.det_id.resize(ic);
   }

}
  //----------------------------------------------------------------------------------------------
  /* Execute the algorithm.   */
void ConvertToMDEvents::exec(){
    // -------- Input workspace 
    MatrixWorkspace_sptr inMatrixWS = getProperty("InputWorkspace");
    if(!inMatrixWS){
        g_log.error()<<" can not obtain input matrix workspace from analysis data service\n";
    }
    inWS2D                           = boost::dynamic_pointer_cast<Workspace2D>(inMatrixWS);

    // Identify what dimension names we can obtain from the input workspace;
    std::vector<std::string> dim_names_availible;
    // assume that |Q| and QxQyQz availible from any workspace?
    std::vector<std::string> wsNames(2);
    wsNames[0]="|Q|";
    wsNames[1]="QxQyQz";

// get the X axis
    NumericAxis *pXAxis = dynamic_cast<NumericAxis *>(inWS2D->getAxis(0));
    if(!pXAxis )
    {
        convert_log.error()<<"Can not retrieve X axis from the source workspace: "<<inWS2D->getName()<<std::endl;
        throw(std::invalid_argument("Input workspace has to have X-axis"));
    }
    std::string Dim1Name = pXAxis->unit()->unitID();
    wsNames.push_back(Dim1Name);
    //
    dim_names_availible = this->get_dimension_names(wsNames,inWS2D);


    // get dim_names requested by user:
    std::vector<std::string> dim_requested;
    //a) by Q selector:
    std::string Q_dim_requested       = getProperty("QDimensions");  
    //b) by other dim property;
     other_dim_names                  = getProperty("OtherDimensions");


   // Identify the algorithm to deploy and idemtify/set the dimension names to use
    std::string algo_id = identify_the_alg(dim_names_availible, Q_dim_requested,other_dim_names,n_activated_dimensions);
   
    

    bool reuse_preprocecced_detectors = getProperty("UsePreprocessedDetectors");
    if(!(reuse_preprocecced_detectors&&det_loc.is_defined()))process_detectors_positions(inWS2D);
     dim_min.assign(n_activated_dimensions,-1);
     dim_max.assign(n_activated_dimensions,1);

    // create the event workspace with proper numner of dimensions and specified box controller parameters;
     API::IMDEventWorkspace_sptr spws = ws_creator[n_activated_dimensions](this,5,10,20);
     if(!spws){
        g_log.error()<<"can not create target event workspace with :"<<n_activated_dimensions<<" dimensions\n";
        throw(std::invalid_argument("can not create target workspace"));

     } 

    // call selected algorithm
    pMethod algo =  alg_selector[algo_id];
    if(algo){
        algo(this,spws.get());
    }else{
        g_log.error()<<"requested undefined subalgorithm :"<<algo_id<<std::endl;
        throw(std::invalid_argument("undefined subalgoritm requested "));
    }
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(spws));
    return;
   
}
 /** function processes input arguments and tries to istablish what algorithm should be deployed; 
    *
    * @param dim_names_availible -- array of the names of the dimension (includeing default dimensiton) which can be obtained from input workspace
    * @param Q_dim_requested     -- what to do with Q-dimensions e.g. calculate either mod|Q| or Q3D;
    * @param dim_selected        -- vector of other dimension names requested by the algorithm
    *
    * @return the_algID       -- the string, identifying one of the known algorithms; if unknown, should fail. 
*/
std::string
ConvertToMDEvents::identify_the_alg(const std::vector<std::string> &dim_names_availible, const std::string &Q_dim_requested, const std::vector<std::string> &dim_requested, size_t &nDims)
{
    std::string the_algID;
    std::string Q_mode("Unknown");
    std::string dE_mode("Unknown");
    std::string ND_mode("Unknown");

    std::vector<std::string> Q_Dim_names;
    //TODO: add direct./indirect;
    int nQ_dims(0),ndE_dims(0),nAdd_dims(0);

    nDims = 0;
    // verify if everything requested is availible in logs:
    for(size_t i=0;i<dim_requested.size();i++){
        if(std::find(dim_names_availible.begin(),dim_names_availible.end(),dim_requested[i])==dim_names_availible.end()){
            g_log.error()<<" The dimension: "<<dim_requested[i]<<" requested but can not be found in the list of availible parameters & data\n";
            throw(std::invalid_argument(" the data for availible dimension are not among the input data"));
        }
    }
    // Q_mode (one of 3 possible)  
    if(Q_dim_requested.empty())
    { 
        nQ_dims=0;
        Q_mode="NoQ";
    }
    if(Q_dim_requested.compare("|Q|")==0)
    {
        nQ_dims=1;
        Q_mode="modQ";
        Q_Dim_names.resize(1);
        Q_Dim_names[0]="|Q|";
    }
    if((Q_dim_requested.compare("QxQyQz")==0))
    {
       nQ_dims=3;
       Q_mode="Q3D";
       Q_Dim_names.resize(3);
       Q_Dim_names[0]="Q_h";
       Q_Dim_names[1]="Q_k";
       Q_Dim_names[2]="Q_l";

    }

   // Elastic/inelastic -- should introduce additional switch;
    nAdd_dims=(int)dim_requested.size();
    if(std::find(dim_requested.begin(),dim_requested.end(),"DeltaE")!=dim_requested.end()){
        ndE_dims   =1;
        nAdd_dims -=1;
        dE_mode   ="dE";
        Q_Dim_names.resize(Q_Dim_names.size()+1);
        Q_Dim_names[Q_Dim_names.size()-1]="DeltaE";
    }else{
        ndE_dims   =0;
        dE_mode   ="";
    }
    std::vector<std::string> Add_dims(nAdd_dims);
    //ND mode;
    if(nAdd_dims>0){
        ND_mode = "ND";
        if(!dE_mode.empty()){    // copy all dimensions names except DeltaE;
            size_t ic(0);
            for(size_t i=0;i<dim_requested.size();i++){
                if(dim_requested[i]!="DeltaE"){
                    Add_dims[ic]=dim_requested[i];
                    ic++;
                }
            }
        }else{ //   // copy all dimensions names 
            Add_dims.assign(dim_requested.begin(),dim_requested.end());
        }
    }else{
        ND_mode = "";
    }

 
    nDims      = nQ_dims+ndE_dims+nAdd_dims;
    if(nDims<2||nQ_dims<0||ndE_dims<0||nAdd_dims<0){
        g_log.error()<<" Requested: "<<nQ_dims<<" Q-dimensions, "<<ndE_dims<<" dE dimesions and "<<nAdd_dims<<" additional dimesnions not supported\n";
        throw(std::invalid_argument("wrong or unsupported number of dimensions"));
    }
    the_algID  = Q_mode+dE_mode+ND_mode+boost::lexical_cast<std::string>(nDims);

    if(the_algID.find("Unknown")!=std::string::npos){
        g_log.error()<<" Algorithm with ID: "<<the_algID<<" do not recognized\n";
        throw(std::invalid_argument("wrong or unsupported algorithm ID"));
    }
    // put Q-dimension names to target dimensions
    this->dim_names.assign(Q_Dim_names.begin(),Q_Dim_names.end());
    std::vector<std::string>::const_iterator it=Add_dims.begin();
    // push Add-dimension names to target dimensions
    for(;it!=Add_dims.end();it++){   dim_names.push_back(*it);
    }
    //TODO: make dim units equal to the dimension names (For the time being?)
    this->dim_units.assign(this->dim_names.begin(),this->dim_names.end());

    return the_algID;

}

std::vector<std::string > 
ConvertToMDEvents::get_dimension_names(const std::vector<std::string> &default_prop,MatrixWorkspace_const_sptr inMatrixWS)const{
    // number of properties we always want to have, it is Q3D, |Q| and some property form workspace;
    // if workspace unit is the energy transfer, it is a special property;
    size_t n_common_properties = default_prop.size();

    const std::vector< Kernel::Property * > run_properties =  inMatrixWS->run().getProperties();  

    std::vector<std::string> prop_names(default_prop.begin(),default_prop.end());
    prop_names.resize(n_common_properties+run_properties.size());
    
 // inelastic workspaces need special treatment, their own subalgorithms and have additional dimension
   // let's identify if it is inelastic ws
    Kernel::Unit_const_sptr unit = inMatrixWS->getAxis(0)->unit();
    if (unit){
        std::string ws_property_name = unit->unitID();
        std::vector<std::string >::iterator it;
        it = find (prop_names.begin(), prop_names.end(), "DeltaE"); // can not fail, DeltaE is in the list of the properties;
        *it          = ws_property_name;
    }else{
        convert_log.error()<<" input workspace has to have units\n";
        throw(std::invalid_argument(" input worspace has to have units"));
    }

    // extract all properties, which can be treated as dimension names;
    for(size_t i=0;i<run_properties.size();i++){
        prop_names[n_common_properties+i]=run_properties[i]->name();
    }
    return prop_names;
}



/** The matrix to convert 
 
*/
std::vector<double> 
ConvertToMDEvents::get_transf_matrix()const
{
    
    // Initalize the matrix to 3x3 identity
    Kernel::Matrix<double> mat = Kernel::Matrix<double>(3,3, true);
    // Set the matrix based on UB etc.
    Kernel::Matrix<double> ub = inWS2D->sample().getOrientedLattice().getUB();
    Kernel::Matrix<double> gon =inWS2D->run().getGoniometer().getR();
    // As per Busing and Levy 1967, HKL = Goniometer * UB * q_lab_frame
    mat = gon * ub;
    std::vector<double> rotMat = mat.get_vector();
    return rotMat;
}

/** function extracts the coordinates from additional workspace porperties and places them to proper position within array of coodinates for 
    the particular workspace.

    @param Coord             -- vector of coordinates for current multidimensional event
    @param nd                -- number of event's dimensions
    @param n_ws_properties   -- number of dimensions, provided by the workspace itself. E.g., processed inelastic matrix
                                workspace with provides 4 dimensions, matrix workspace in elastic mode -- 3 dimensions, powder 
                                -- 2 for elastic and 3 for inelastic mode. Number of these properties is determined by the deployed algorithm
                                The coordinates, obtained from the workspace placed first in the array of coordinates, and the coordinates, 
                                obtained from dimensions placed after them. */
void 
ConvertToMDEvents::fillAddProperties(std::vector<coord_t> &Coord,size_t nd,size_t n_ws_properties)
{
     for(size_t i=n_ws_properties;i<nd;i++){
         //HACK: A METHOD, Which converts TSP into value, correspondent to time scale of matrix workspace has to be developed and deployed!
          Kernel::TimeSeriesProperty<double> *run_property = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(inWS2D->run().getProperty(this->other_dim_names[i-n_ws_properties]));  
          if(!run_property){
             g_log.error()<<" property: "<<this->other_dim_names[i]<<" is not a time series (run) property\n";
          }
          Coord[i]=run_property->firstValue();
        }  
}


template void ConvertToMDEvents::processQND<2,NoQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<3,NoQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<4,NoQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<5,NoQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<6,NoQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<7,NoQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<8,NoQ>(API::IMDEventWorkspace *const);

template void ConvertToMDEvents::processQND<2,modQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<3,modQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<4,modQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<5,modQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<6,modQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<7,modQ>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<8,modQ>(API::IMDEventWorkspace *const);

template void ConvertToMDEvents::processQND<3,Q3D>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<4,Q3D>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<5,Q3D>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<6,Q3D>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<7,Q3D>(API::IMDEventWorkspace *const);
template void ConvertToMDEvents::processQND<8,Q3D>(API::IMDEventWorkspace *const);

template   API::IMDEventWorkspace_sptr  ConvertToMDEvents::createEmptyEventWS<2>(size_t ,size_t ,size_t );
template   API::IMDEventWorkspace_sptr  ConvertToMDEvents::createEmptyEventWS<3>(size_t ,size_t ,size_t );
template   API::IMDEventWorkspace_sptr  ConvertToMDEvents::createEmptyEventWS<4>(size_t ,size_t ,size_t );
template   API::IMDEventWorkspace_sptr  ConvertToMDEvents::createEmptyEventWS<5>(size_t ,size_t ,size_t );
template   API::IMDEventWorkspace_sptr  ConvertToMDEvents::createEmptyEventWS<6>(size_t ,size_t ,size_t );
template   API::IMDEventWorkspace_sptr  ConvertToMDEvents::createEmptyEventWS<7>(size_t ,size_t ,size_t );
template   API::IMDEventWorkspace_sptr  ConvertToMDEvents::createEmptyEventWS<8>(size_t ,size_t ,size_t );

//----------------------------------------------------------------------------------------------
/** Constructor
*/
ConvertToMDEvents::ConvertToMDEvents():
 Q_ID_possible(3)
{
    Q_ID_possible[0]="|Q|";
    Q_ID_possible[1]="QxQyQz";    
    Q_ID_possible[2]="";    // no Q dimension (does it have any interest&relevance to ISIS/SNS?) 
     
#define NOQ   
    alg_selector.insert(std::pair<std::string,pMethod>("NoQND2",&ConvertToMDEvents::processQND<2,NoQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("NoQND3",&ConvertToMDEvents::processQND<3,NoQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("NoQND4",&ConvertToMDEvents::processQND<4,NoQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("NoQND5",&ConvertToMDEvents::processQND<5,NoQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("NoQND6",&ConvertToMDEvents::processQND<6,NoQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("NoQND7",&ConvertToMDEvents::processQND<7,NoQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("NoQND8",&ConvertToMDEvents::processQND<8,NoQ>));
#undef NOQ
    //
#define MODQ
    alg_selector.insert(std::pair<std::string,pMethod>("modQND2",&ConvertToMDEvents::processQND<2,modQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("modQND3",&ConvertToMDEvents::processQND<3,modQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("modQND4",&ConvertToMDEvents::processQND<4,modQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("modQND5",&ConvertToMDEvents::processQND<5,modQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("modQND6",&ConvertToMDEvents::processQND<6,modQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("modQND7",&ConvertToMDEvents::processQND<7,modQ>));
    alg_selector.insert(std::pair<std::string,pMethod>("modQND8",&ConvertToMDEvents::processQND<8,modQ>));
#undef MODQ
    //
#define Q3D_
    alg_selector.insert(std::pair<std::string,pMethod>("Q3DND3",&ConvertToMDEvents::processQND<3,Q3D>));
    alg_selector.insert(std::pair<std::string,pMethod>("Q3DND4",&ConvertToMDEvents::processQND<4,Q3D>));
    alg_selector.insert(std::pair<std::string,pMethod>("Q3DND5",&ConvertToMDEvents::processQND<5,Q3D>));
    alg_selector.insert(std::pair<std::string,pMethod>("Q3DND6",&ConvertToMDEvents::processQND<6,Q3D>));
    alg_selector.insert(std::pair<std::string,pMethod>("Q3DND7",&ConvertToMDEvents::processQND<7,Q3D>));
    alg_selector.insert(std::pair<std::string,pMethod>("Q3DND8",&ConvertToMDEvents::processQND<8,Q3D>));
#undef Q3D_

    ws_creator.insert(std::pair<size_t,pWSCreator>(2,&ConvertToMDEvents::createEmptyEventWS<2>));
    ws_creator.insert(std::pair<size_t,pWSCreator>(3,&ConvertToMDEvents::createEmptyEventWS<3>));
    ws_creator.insert(std::pair<size_t,pWSCreator>(4,&ConvertToMDEvents::createEmptyEventWS<4>));
    ws_creator.insert(std::pair<size_t,pWSCreator>(5,&ConvertToMDEvents::createEmptyEventWS<5>));
    ws_creator.insert(std::pair<size_t,pWSCreator>(6,&ConvertToMDEvents::createEmptyEventWS<6>));
    ws_creator.insert(std::pair<size_t,pWSCreator>(7,&ConvertToMDEvents::createEmptyEventWS<7>));
    ws_creator.insert(std::pair<size_t,pWSCreator>(8,&ConvertToMDEvents::createEmptyEventWS<8>));
}

//

} // namespace Mantid
} // namespace MDAlgorithms


