/*WIKI* 


*WIKI*/
#include "MantidMDAlgorithms/ConvertToQNDany.h"

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
   Kernel::Logger& ConvertToQNDany::convert_log =Kernel::Logger::get("MD-Algorithms");


// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToQNDany)
  
/// Sets documentation strings for this algorithm
void ConvertToQNDany::initDocs()
{
    this->setWikiSummary("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions. If the OutputWorkspace exists, then events are added to it.");
    this->setOptionalMessage("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions. If the OutputWorkspace exists, then events are added to it.");
}

//----------------------------------------------------------------------------------------------
/** Constructor
*/
ConvertToQNDany::ConvertToQNDany():
 Q_ID_possible(3)
{
    Q_ID_possible[0]="|Q|";
    Q_ID_possible[1]="QxQyQz";    
    Q_ID_possible[2]="";    // no Q dimension (does it have any interest&relevance to ISIS/SNS?) 
  
    alg_selector.resize(ConvertToQNDany::Unknow);

    alg_selector[NoQND]   = &ConvertToQNDany::processNoQ;
    alg_selector[modQdE]  = &ConvertToQNDany::processModQdE;
    alg_selector[modQND]  = &ConvertToQNDany::processModQND;
    alg_selector[modQdEND]= &ConvertToQNDany::processModQdEND;
    alg_selector[Q3D]     = &ConvertToQNDany::processQ3D;
    alg_selector[Q3DdE]   = &ConvertToQNDany::processQ3DdE;
    alg_selector[Q3DND]   = &ConvertToQNDany::processQ3DND;
    alg_selector[Q3DdEND] = &ConvertToQNDany::processQ3DdEND;
}
    
//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToQNDany::~ConvertToQNDany()
{}


//
const double rad2deg = 180.0 / M_PI;
/// helper function to create empty MDEventWorkspace with nd dimensions 
template<size_t nd>
boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> > create_emptyNDEventWS(const std::string dimensionNames[nd],const std::string dimensionUnits[nd],
                                                                              const std::vector<double> &dimMin,const std::vector<double> &dimMax)
{

       boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> > ws = 
       boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> >(new MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd>());
    
      // Give all the dimensions
      for (size_t d=0; d<nd; d++)
      {
        MDHistoDimension * dim = new MDHistoDimension(dimensionNames[d], dimensionNames[d], dimensionUnits[d], dimMin[d], dimMax[d], 10);
        ws->addDimension(MDHistoDimension_sptr(dim));
      }
      ws->initialize();

      // Build up the box controller
      Mantid::API::BoxController_sptr bc = ws->getBoxController();
      bc->setSplitInto(5);
//      bc->setSplitThreshold(1500);
      bc->setSplitThreshold(10);
      bc->setMaxDepth(20);
      // We always want the box to be split (it will reject bad ones)
      ws->splitBox();
      return ws;
}


//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void 
ConvertToQNDany::init()
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
ConvertToQNDany::check_max_morethen_min(const std::vector<double> &min,const std::vector<double> &max){
    for(size_t i=0; i<min.size();i++){
        if(max[i]<=min[i]){
            convert_log.error()<<" min value "<<min[i]<<" not less then max value"<<max[i]<<" in direction: "<<i<<std::endl;
            throw(std::invalid_argument("min limit not smaller then max limit"));
        }
    }
}
 

 
  //----------------------------------------------------------------------------------------------
  /* Execute the algorithm.   */
void ConvertToQNDany::exec(){
    // -------- Input workspace 
    MatrixWorkspace_sptr inMatrixWS = getProperty("InputWorkspace");
    Workspace2D_sptr inWS2D         = boost::dynamic_pointer_cast<Workspace2D>(inMatrixWS);

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
   std::vector<std::string> other_dim  =getProperty("OtherDimensions");


   // Verify input parameters;
    std::vector<std::string> dim_selected;
    known_algorithms algo_id = identify_requested_alg(dim_names_availible, Q_dim_requested,other_dim,n_activated_dimensions);

    return;
   
}
 /** function processes input arguments and tries to istablish what algorithm should be deployed; 
    *
    * @param dim_names_availible -- array of the names of the dimension (includeing default dimensiton) which can be obtained from input workspace
    * @param Q_dim_requested     -- what to do with Q-dimensions e.g. calculate either mod|Q| or Q3D;
    * @param dim_selected        -- vector of other dimension names requested by the algorithm
    *
    * @return known_algorithms   -- enum identifying one of the known algorithms; if unknown, should fail. 
   */
ConvertToQNDany::known_algorithms
ConvertToQNDany::identify_requested_alg(const std::vector<std::string> &dim_names_availible, const std::string &Q_dim_requested, const std::vector<std::string> &dim_requested, size_t &nDims)const
{

    nDims = 0;
    // verify if everything requested is availible:
    for(size_t i=0;i<dim_requested.size();i++){
        if(std::find(dim_names_availible.begin(),dim_names_availible.end(),dim_requested[i])==dim_names_availible.end()){
            g_log.error()<<" The dimension: "<<dim_requested[i]<<" requested but can not be found in the list of availible parameters & data\n";
            throw(std::invalid_argument(" the data for availible dimension are not among the input data"));
        }
    }
   // NoQND
    if(Q_dim_requested.empty()){ 
        nDims = dim_requested.size();
        return ConvertToQNDany::NoQND;
    }

    // modQdE,modQND,modQdEND
    if(Q_dim_requested.compare("|Q|")==0){
        if(std::find(dim_requested.begin(),dim_requested.end(),"DeltaE")!=dim_requested.end()){ // modQdE,modQdEND
            if(dim_requested.size()==1){ //  modQdE;
                nDims = 2;
                return ConvertToQNDany::modQdE;
            }else{
                nDims = 1+dim_requested.size();
                return ConvertToQNDany::modQdEND;
            }
        }else{                          // modQND
            nDims=    dim_requested.size()+1;
            return ConvertToQNDany::modQND;
        }
    }

    if(!(Q_dim_requested.compare("QxQyQz")==0)){
            g_log.error()<<" Q-value equal to: "<<Q_dim_requested<<" is not recognized\n";
            throw(std::invalid_argument(" invalid Q argument"));
    }
    //Q3D,Q3DdE,Q3DND,Q3DdEND
    if(std::find(dim_requested.begin(),dim_requested.end(),"DeltaE")!=dim_requested.end()){ // modQdE,modQdEND
            if(dim_requested.size()==1){ //  Q3DdE,Q3DdEND;
                nDims = 4;
                return ConvertToQNDany::Q3DdE;
            }else{
                nDims = 3+dim_requested.size();
                return ConvertToQNDany::Q3DdEND;
            }
   }else{       // Q3D,Q3DND
       if(dim_requested.size()==0){
            nDims = 3;
            return ConvertToQNDany::Q3D;
       }else{
            nDims= dim_requested.size()+3;
            return ConvertToQNDany::Q3DND;
       }
    }

  
}

std::vector<std::string > 
ConvertToQNDany::get_dimension_names(const std::vector<std::string> &default_prop,MatrixWorkspace_const_sptr inMatrixWS)const{
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


void ConvertToQNDany::processNoQ()
{
    throw(Kernel::Exception::NotImplementedError(""));
}
void ConvertToQNDany::processModQdE()
{
    throw(Kernel::Exception::NotImplementedError(""));
}
void ConvertToQNDany::processModQND()
{
    throw(Kernel::Exception::NotImplementedError(""));
}
void ConvertToQNDany::processModQdEND()
{
    throw(Kernel::Exception::NotImplementedError(""));
}
void ConvertToQNDany::processQ3D()
{
    throw(Kernel::Exception::NotImplementedError(""));
}
void ConvertToQNDany::processQ3DdE( )
{
    throw(Kernel::Exception::NotImplementedError(""));
}

void ConvertToQNDany::processQ3DND( )
{
    throw(Kernel::Exception::NotImplementedError(""));
}
void ConvertToQNDany::processQ3DdEND()
{
    throw(Kernel::Exception::NotImplementedError(""));
}

} // namespace Mantid
} // namespace MDAlgorithms

