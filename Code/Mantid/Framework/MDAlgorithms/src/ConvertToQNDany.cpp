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
#include "MantidMDEvents/MDEventFactory.h"

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

// the variable describes the locations of the preprocessed detectors, which can be stored and reused it the algorithm runs for more once;
preprocessed_detectors ConvertToQNDany::det_loc;

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
     
 //   alg_selector.insert(std::pair<std::string,pMethod>("NoQND",&ConvertToQNDany::process_NoQ__ND));
    alg_selector.insert(std::pair<std::string,pMethod>("modQdE",&ConvertToQNDany::process_ModQ_dE_));
    alg_selector.insert(std::pair<std::string,pMethod>("modQND",&ConvertToQNDany::process_ModQ__ND));
    alg_selector.insert(std::pair<std::string,pMethod>("modQdEND",&ConvertToQNDany::process_ModQ_dE_ND));
    alg_selector.insert(std::pair<std::string,pMethod>("Q3D",&ConvertToQNDany::process_Q3D___));
    alg_selector.insert(std::pair<std::string,pMethod>("Q3DdE",&ConvertToQNDany::process_Q3D_dE_));
    alg_selector.insert(std::pair<std::string,pMethod>("Q3DND",&ConvertToQNDany::process_Q3D__ND));
    alg_selector.insert(std::pair<std::string,pMethod>("Q3DdEND",&ConvertToQNDany::process_Q3D_dE_ND));
}
    
//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToQNDany::~ConvertToQNDany()
{}


//
const double rad2deg = 180.0 / M_PI;
/// helper function to create empty MDEventWorkspace with nd dimensions 
API::IMDEventWorkspace_sptr 
ConvertToQNDany::create_emptyNDEventWS(const std::vector<std::string> &dimensionNames,const std::vector<std::string> dimensionUnits,
                                                                              const std::vector<double> &dimMin,const std::vector<double> &dimMax,int nd)
{

     // Try to get the output workspace
    i_out = getProperty("OutputWorkspace");
    if(i_out){ // temporary, to agvoid complications of adding particular data to ws.
        g_log.warning()<<" Output workspace"<<i_out->name()<<" will be replaced\n";
    }
    MDEvents::MDEventFactory wsFactory;
    i_out = wsFactory.CreateMDWorkspace(n_activated_dimensions,"MDEvent");


    
    // Give all the dimensions
    for (size_t d=0; d<nd; d++)
    {
        MDHistoDimension * dim = new MDHistoDimension(dimensionNames[d], dimensionNames[d], dimensionUnits[d], dimMin[d], dimMax[d], 10);
        i_out->addDimension(MDHistoDimension_sptr(dim));
    }
    i_out->initialize();

     // Build up the box controller
    Mantid::API::BoxController_sptr bc = i_out->getBoxController();
      bc->setSplitInto(5);
//      bc->setSplitThreshold(1500);
      bc->setSplitThreshold(10);
      bc->setMaxDepth(20);
      // We always want the box to be split (it will reject bad ones)
     // i_out->splitBox();
      return i_out;
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
 
/// helper function to preprocess the detectors directions
void 
ConvertToQNDany::process_detectors_positions(const DataObjects::Workspace2D_const_sptr inputWS)
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
    std::string algo_id = identify_the_alg(dim_names_availible, Q_dim_requested,other_dim,n_activated_dimensions);

 
    bool reuse_preprocecced_detectors = getProperty("UsePreprocessedDetectors");
    if(!(reuse_preprocecced_detectors&&det_loc.is_defined()))process_detectors_positions(inWS2D);
   
   
    // call selected algorithm
    pMethod algo =  alg_selector[algo_id];
    if(algo){
        algo(this);
    }else{
        g_log.error()<<"requested undefined subalgorithm :"<<algo_id<<std::endl;
        throw(std::invalid_argument("undefined subalgoritm requested "));
    }

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
ConvertToQNDany::identify_the_alg(const std::vector<std::string> &dim_names_availible, const std::string &Q_dim_requested, const std::vector<std::string> &dim_requested, size_t &nDims)const
{
    std::string the_algID;
    std::string Q_mode("Unknown");
    std::string dE_mode("Unknown");
    std::string ND_mode("Unknown");
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
    }
    if((Q_dim_requested.compare("QxQyQz")==0))
    {
       nQ_dims=3;
       Q_mode="Q3D";
    }

   // Elastic/inelastic -- should introduce additional switch;
    nAdd_dims=(int)dim_requested.size();
    if(std::find(dim_requested.begin(),dim_requested.end(),"DeltaE")!=dim_requested.end()){
        ndE_dims   =1;
        nAdd_dims -=1;
        dE_mode   ="dE";
    }else{
        ndE_dims   =0;
        dE_mode   ="";
    }

    //ND mode;
    if(nAdd_dims>0){
        ND_mode = "ND";
    }else{
        ND_mode = "";
    }

    the_algID  = Q_mode+dE_mode+ND_mode;
    nDims      = nQ_dims+ndE_dims+nAdd_dims;
    if(nDims<2||nQ_dims<0||ndE_dims<0||nAdd_dims<0){
        g_log.error()<<" Requested: "<<nQ_dims<<" Q-dimensions, "<<ndE_dims<<" dE dimesions and "<<nAdd_dims<<" additional dimesnions not supported\n";
        throw(std::invalid_argument("wrong or unsupported number of dimensions"));
    }

    if(the_algID.find("Unknown")!=std::string::npos){
        g_log.error()<<" Algorithm with ID: "<<the_algID<<" do not recognized\n";
        throw(std::invalid_argument("wrong or unsupported algorithm ID"));
    }

    return the_algID;

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




void ConvertToQNDany::process_ModQ_dE_()
{
    throw(Kernel::Exception::NotImplementedError(""));
}
void ConvertToQNDany::process_ModQ__ND()
{
    throw(Kernel::Exception::NotImplementedError(""));
}
void ConvertToQNDany::process_ModQ_dE_ND()
{

      //size_t n_added_events(0);
      //size_t SPLIT_LEVEL(1000);
      //for (int64_t i = 0; i < int64_t(numSpec); ++i)
      //{

      //  const MantidVec& E_transfer = inWS2D->readX(i);
      //  const MantidVec& Signal     = inWS2D->readY(i);
      //  const MantidVec& Error      = inWS2D->readE(i);
      //  int32_t det_id              = det_loc.det_id[i];
    
      //  coord_t QE[4];
      //  for (size_t j = 0; j < specSize; ++j)
      //  {
      //      // drop emtpy events 
      //      if(Signal[j]<FLT_EPSILON)continue;

      //      double E_tr = 0.5*(E_transfer[j]+E_transfer[j+1]);
      //      if(E_tr<E_min||E_tr>=E_max)continue;

      //      double k_tr = sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
   
      //      double  ex = det_loc.det_dir[i].X();
      //      double  ey = det_loc.det_dir[i].Y();
      //      double  ez = det_loc.det_dir[i].Z();
      //      double  qx  =  -ex*k_tr;                
      //      double  qy  =  -ey*k_tr;
      //      double  qz  = ki - ez*k_tr;

      //      QE[0]  = (coord_t)(rotMat[0]*qx+rotMat[3]*qy+rotMat[6]*qz);  if(QE[0]<QEmin[0]||QE[0]>=QEmax[0])continue;
      //      QE[1]  = (coord_t)(rotMat[1]*qx+rotMat[4]*qy+rotMat[7]*qz);  if(QE[1]<QEmin[1]||QE[1]>=QEmax[1])continue;
      //      QE[2]  = (coord_t)(rotMat[2]*qx+rotMat[5]*qy+rotMat[8]*qz);  if(QE[2]<QEmin[2]||QE[2]>=QEmax[2])continue;
      //      QE[3]  = (coord_t)E_tr;
      //      float ErrSq = float(Error[j]*Error[j]);
      //      i_out->addEvent(MDE(float(Signal[j]),ErrSq,runIndex,det_id,QE));
      //      n_added_events++;
      //  }
  
      //// This splits up all the boxes according to split thresholds and sizes.
      //  //Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
      //  //ThreadPool tp(NULL);
      //  if(n_added_events>SPLIT_LEVEL){
      //      ws->splitAllIfNeeded(NULL);
      //      n_added_events=0;
      //  }
      //  //tp.joinAll();
      //  progress.report(i);  
      //}
      //if(n_added_events>0){
      //   i_out->splitAllIfNeeded(NULL);
      //   n_added_events=0;
      //}
      //i_out->refreshCache();
      //progress.report();      

      //setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(i_out));
}
void ConvertToQNDany::process_Q3D___()
{
    throw(Kernel::Exception::NotImplementedError(""));
}
void ConvertToQNDany::process_Q3D_dE_( )
{
    throw(Kernel::Exception::NotImplementedError(""));
}

void ConvertToQNDany::process_Q3D__ND( )
{
    throw(Kernel::Exception::NotImplementedError(""));
}
void ConvertToQNDany::process_Q3D_dE_ND()
{
    throw(Kernel::Exception::NotImplementedError(""));
}

template<size_t nd>
boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> > create_emptyEventWS(const std::string dimensionNames[nd],const std::string dimensionUnits[nd],
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

template<size_t nd>
void ConvertToQNDany::process_NoQ__ND(boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> > ws)
{

    // one of the dimensions has to be X-ws dimension -> need to add check for that;

    // copy experiment info into target workspace
    ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
    uint16_t runIndex = ws->addExperimentInfo(ExperimentInfo);


    const size_t numSpec  = inWS2D->getNumberHistograms();
    const size_t specSize = inWS2D->blocksize();    
    std::vector<coord_t> Coord(nd);


    size_t n_added_events(0);
    size_t SPLIT_LEVEL(1024);
    for (int64_t i = 0; i < int64_t(numSpec); ++i)
    {

        const MantidVec& E_transfer = inWS2D->readX(i);
        const MantidVec& Signal     = inWS2D->readY(i);
        const MantidVec& Error      = inWS2D->readE(i);
        int32_t det_id              = det_loc.det_id[i];
    
   
        for (size_t j = 0; j < specSize; ++j)
        {
            // drop emtpy events
            if(Signal[j]<FLT_EPSILON)continue;

            double E_tr = 0.5*(E_transfer[j]+E_transfer[j+1]);
            Coord[0]    = (coord_t)E_tr;
 

            float ErrSq = float(Error[j]*Error[j]);
            ws->addEvent(MDEvents::MDEvent<nd>(float(Signal[j]),ErrSq,runIndex,det_id,&Coord[0]));
            n_added_events++;
        }
  
      // This splits up all the boxes according to split thresholds and sizes.
        //Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
        //ThreadPool tp(NULL);
      if(n_added_events>SPLIT_LEVEL){
            ws->splitAllIfNeeded(NULL);
            n_added_events=0;
       }
        //tp.joinAll();
        progress.report(i);  
     }
     if(n_added_events>0){
         ws->splitAllIfNeeded(NULL);
         n_added_events=0;
     }
      ws->refreshCache();
      progress.report();      

      setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(i_out));
}

template void ConvertToQNDany::process_NoQ__ND<4>(boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>, 4> >);
template boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>, 4> >  create_emptyEventWS(const std::string dimensionNames[4],const std::string dimensionUnits[4],
                                                                              const std::vector<double> &dimMin,const std::vector<double> &dimMax);


} // namespace Mantid
} // namespace MDAlgorithms

