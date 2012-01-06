/*WIKI* 
    Transfrom a workspace into MD Event workspace with dimensions defined by user. 
   
    Gateway for set of subalgorithms, combined together to convert inpuy matrix workspace with any units or event workspace into  multidimensional events workspace. 

    Depending on the user input and the data, find in the input workspace, the algorithms transform the input workspace into 1 to 4 dimemsional MDEvent workspace and 
    adds to this workspace additional dimensions, which are described by the workspace properties and requested by user. 

*WIKI*/

#include "MantidMDAlgorithms/ConvertToMDEvents.h"

#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayLengthValidator.h"
//
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceValidators.h"
//
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventFactory.h"
//
#include "MantidDataObjects/Workspace2D.h"
//
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"
#include "MantidMDAlgorithms/ConvertToMDEventsCoordTransf.h"
#include "MantidMDAlgorithms/ConvertToMDEventsMethods.h"

#include <algorithm>
#include <float.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDAlgorithms
{

// logger for loading workspaces  
   Kernel::Logger& ConvertToMDEvents::convert_log =Kernel::Logger::get("MD-Algorithms");
// the variable describes the locations of the preprocessed detectors, which can be stored and reused if the algorithm runs more then once;
preprocessed_detectors ConvertToMDEvents::det_loc;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToMDEvents)
  

// Sets documentation strings for this algorithm
void ConvertToMDEvents::initDocs()
{
    this->setWikiSummary("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions. If the OutputWorkspace exists, then events are added to it.");
    this->setOptionalMessage("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions. If the OutputWorkspace exists, then events are added to it.");
}
/** Helper Static function to obtain the natural units for input workspace. 
  *  Natural units are the units, which subalgorithm is working with without any initial unit transformation.
  *  Other units have to be transfromed into natural untis first
  *
  *@param pHost the pointer to the algorithm to work with
  *
  *@returns the name(ID) of the unit, current algorithm expects to work with internaly 
*/
std::string          
ConvertToMDEvents::getNativeUnitsID(ConvertToMDEvents const *const pHost)
{
    if(pHost->subalgorithm_units.empty()){
        convert_log.error()<<" getNativeUnitsID: requested undefined subalgorithm units, the subalgorithm is probably not yet defined itself\n";
        throw(std::logic_error(" should not be able to call this function when subalgorithm is undefined"));
    }
    return pHost->subalgorithm_units;
}
/** Helper Static function to obtain the units set along X-axis of the input workspace. 
  *
  *@param pHost the pointer to the algorithm to work with
  *
  *@returns the name(ID) of the unit, specified along X-axis of current workspace
*/
Kernel::Unit_sptr    
ConvertToMDEvents::getAxisUnits(ConvertToMDEvents const *const pHost){
    if(!pHost->inWS2D){
        convert_log.error()<<"getAxisUnits: invoked when input workspace is undefined\n";
        throw(std::logic_error(" should not be able to call this function when workpsace is undefined"));
    }
    API::NumericAxis *pAxis = dynamic_cast<API::NumericAxis *>(pHost->inWS2D->getAxis(0));
    if(!pAxis){
        convert_log.error()<<"getAxisUnits: can not obtained when first workspace axis is undefined or not numeric\n";
        throw(std::logic_error(" should not be able to call this function when X-axis is wrong"));
    }
    return pHost->inWS2D->getAxis(0)->unit();
}
/** Helper Static function to obtain the reference, to the structure with preprocessed detectors
  *
  *@param pHost the pointer to the algorithm to work with
  *
  *@returns the reference to the structure with information about the preprocessed detectors. 
  *         Throws if the structure has not been defined
*/
preprocessed_detectors & 
ConvertToMDEvents::getPrepDetectors(ConvertToMDEvents const *const pHost)
{       
        UNUSED_ARG(pHost);
        if(!det_loc.is_defined()){
            convert_log.error()<<"getPrepDetectors: invoked when preprocessed detectors are undefined\n";
            throw(std::logic_error(" should not be able to call this function when detectors are undefined"));
        }
        return ConvertToMDEvents::det_loc;
}
/** Helper Static function to obtain the energy of incident neutrons 
  *
  *@param pHost the pointer to the algorithm to work with
  *
  *@returns the incident energy of the neutrons. 
  *         Throws if the energy property is not defined or can not be retrieved from the workspace
*/
double
ConvertToMDEvents::getEi(ConvertToMDEvents const *const pHost)
{
    if(!pHost->inWS2D){
        convert_log.error()<<"getEi: invoked when input workspace is undefined\n";
        throw(std::logic_error(" should not call this function when input workpace is undefined"));
    }
    Kernel::PropertyWithValue<double>  *pProp(NULL);
    try{
       pProp  =dynamic_cast<Kernel::PropertyWithValue<double>  *>(pHost->inWS2D->run().getProperty("Ei"));
    }catch(...){
    }
    if(!pProp){
        //convert_log.error()<<"getEi: can not obtain incident energy of neutrons\n";
        //throw(std::logic_error(" should not call this function when incident energy is undefined"));
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*pProp); 
}
/** Helper Static function to obtain current analysis mode 
  *
  *@param pHost the pointer to the algorithm to work with
  *
  *@returns the mode 0-elastic, 1--direct, 2 indirect. Throws if the mode is not defined or should not be defined 
  (NoQ mode -- no analysis expected)
*/
int  
ConvertToMDEvents::getEMode(ConvertToMDEvents const *const pHost){
    if(pHost->algo_id.empty()){
        convert_log.error()<<"getEMode: emode undefined\n";
        throw(std::logic_error(" should not call this function when emode is undefined"));
    }
    if(pHost->algo_id.find(pHost->dE_modes[Elastic])!=std::string::npos){
        // elastic emode
        return (int)Elastic;
    }
    if(pHost->algo_id.find(pHost->dE_modes[Direct])!=std::string::npos){
        // direct emode
        return (int)Direct;
    }
    if(pHost->algo_id.find(pHost->dE_modes[Indir])!=std::string::npos){
        // indirect emode
        return (int)Indir;
    }
    convert_log.error()<<"getEMode: emode for algorithm with ID: "<<pHost->algo_id<<" not defined \n";
    throw(std::logic_error(" can not identify correct emode"));
    return -1;
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToMDEvents::~ConvertToMDEvents()
{}
/** function checks if the candidate belongs to the group and returns its number in the group or -1 if the candidate is not a group member */
int is_member(const std::vector<std::string> &group,const std::string &candidate)
{
    int num(-1);
    for(size_t i=0;i<group.size();i++){
        if(candidate.compare(group[i])==0){
            num = int(i);
            return num;
        }
    }
    return num;
}
//
const double rad2deg = 180.0 / M_PI;
//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void 
ConvertToMDEvents::init()
{
      CompositeWorkspaceValidator<> *ws_valid = new CompositeWorkspaceValidator<>;
      //
      ws_valid->add(new InstrumentValidator<>);
      // the validator which checks if the workspace has axis and any units
      ws_valid->add(new WorkspaceUnitValidator<>(""));


    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,ws_valid),
        "An input Matrix Workspace (Matrix 2D or Event) with units along X-axis and defined instrument with sample ");
   
     declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace. If the workspace already exists, new MD events will be added to it (this may be not very efficient for HDD-based workspaces)");

     
     /// this variable describes default possible ID-s for Q-dimensions   
     declareProperty("QDimensions",Q_modes[modQ],new ListValidator(Q_modes),
         "You can to transfer source workspace dimensions into target worskpace directly (NoQ) or transform into mod(Q) (1 dimension) or QxQyQz (3 dimensions) in Q space",Direction::InOut);        

     /// this variable describes implemented modes for energy transfer analysis
     declareProperty("dEAnalysisMode",dE_modes[Direct],new ListValidator(dE_modes),
        "You can analyze neutron energy transfer in direct, indirect or elastic mode. The analysis mode has to correspond to experimenal set up."
        " Inelastic modes add to the target workspace one additional dimension",Direction::InOut);        

     
    declareProperty(new ArrayProperty<std::string>("OtherDimensions",Direction::Input),
        " List(comma separated) of additional to Q (orthogonal) dimensions in the target workspace.\n"
        " The names of these dimensions have to coinside with the log names in the source workspace");
    // this property is mainly for subalgorithms to set-up as they have to identify 
    declareProperty(new PropertyWithValue<bool>("UsePreprocessedDetectors", true, Direction::Input), 
        "Store the part of the detectors transfromation into reciprocal space to save/reuse it later;");
 

    declareProperty(new ArrayProperty<double>("MinValues"),
        "An array of size: \n"
        "a) 1+N_OtherDimensions if the first dimension (QDimensions property) is equal to |Q| or \n"
        "b) 3+N_OtherDimensions if the first (3) dimensions (QDimensions property) equal  QxQyQz or \n"
        "c) (1 or 2)+N_OtherDimesnions if QDimesnins property is emtpty. \n"
        " In case c) the target workspace dimensions are defined by the units of the input workspace axis\n\n"
         " This array contains minimal values for all dimensions.\n"
         " Momentum values expected to be in [A^-1] and energy transfer (if any) expressed in [meV]\n"
         " All other values are in uints they are in their log files\n"
         " Values lower then the specified one will be ignored\n"
         " If a minimal target workspace range is higer then the one specified here, the target workspace range will be used intstead" );

   declareProperty(new ArrayProperty<double>("MaxValues"),
         "An array of the same size and the same units as MinValues array"
         "Values higher then the specified by this array will be ignored\n"
         "If a maximal target workspace range is lower, then one of specified here, the target workspace range will be used instead" );
    
    declareProperty(new ArrayProperty<double>("u"),
     "Optional: first  base vector (in hkl) defining fractional coordinate system for neutron diffraction; default value is [1,0,0] or powder mode");
    declareProperty(new ArrayProperty<double>("v"),
      "Optional: second base vector (in hkl) defining fractional coordinate system for neutron diffraction; default value is [0,1,0] or powder mode");  

   // Box controller properties. These are the defaults
    this->initBoxControllerProps("5" /*SplitInto*/, 1500 /*SplitThreshold*/, 20 /*MaxRecursionDepth*/);

}

 //----------------------------------------------------------------------------------------------
/* Execute the algorithm.   */
void ConvertToMDEvents::exec()
{
  // in case of subsequent calls
  this->algo_id="";
  // initiate class which would deal with any dimension workspaces
  if(!pWSWrapper.get())
  {
    pWSWrapper = std::auto_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
  }
  // -------- Input workspace
  this->inWS2D = getProperty("InputWorkspace");
  if(!inWS2D)
  {
    convert_log.error()<<" can not obtain input matrix workspace from analysis data service\n";
  }
  // ------- Is there any output workspace?
  // shared pointer to target workspace
  API::IMDEventWorkspace_sptr spws = getProperty("OutputWorkspace");
  bool create_new_ws(false);
  if(!spws)
  {
    create_new_ws = true;
  }

  //identify if u,v are present among input parameters and use defaults if not
  Kernel::V3D u,v;
  std::vector<double> ut = getProperty("u");
  std::vector<double> vt = getProperty("v");
  this->checkUVsettings(ut,vt,u,v);

  // set up target coordinate system
  this ->rotMatrix = getTransfMatrix(inWS2D,u,v);

  // if new workspace is created, its properties are determened by the user's input
  if (create_new_ws)
  {
    // what dimension names requested by the user by:
    //a) Q selector:
    std::string Q_mod_req                    = getProperty("QDimensions");
    //b) the energy exchange mode
    std::string dE_mod_req                   = getProperty("dEAnalysisMode");
    //c) other dim property;
    std::vector<std::string> other_dim_names = getProperty("OtherDimensions");

    // Identify the algorithm to deploy and identify/set the dimension names to use
    algo_id = identifyTheAlg(inWS2D,Q_mod_req,dE_mod_req,other_dim_names,targ_dim_names,targ_dim_units);

    // set the min and max values for the dimensions from the input porperties
    dim_min = getProperty("MinValues");
    dim_max = getProperty("MaxValues");
    // verify that the number min/max values is equivalent to the number of dimensions defined by properties
    if(dim_min.size()!=dim_max.size()||dim_min.size()!=n_activated_dimensions)
    {
      g_log.error()<<" number of specified min dimension values: "<<dim_min.size()<<", number of max values: "<<dim_max.size()<<
                     " and total number of target dimensions: "<<n_activated_dimensions<<" are not consistent\n";
      throw(std::invalid_argument("wrong number of dimension limits"));
    }
    this->checkMaxMoreThenMin(dim_min,dim_max);

    // the output dimensions and almost everything else will be determined by the dimensions of the target workspace
    // user input is mainly ignored
  }
  else
  {
    dim_min.assign(n_activated_dimensions,-1);
    dim_max.assign(n_activated_dimensions,1);
    throw(Kernel::Exception::NotImplementedError("Adding to existing MD workspace not Yet Implemented"));
  }

  bool reuse_preprocecced_detectors = getProperty("UsePreprocessedDetectors");
  if(!(reuse_preprocecced_detectors&&det_loc.is_defined()))processDetectorsPositions(inWS2D,det_loc,convert_log);

  if(create_new_ws)
  {
    spws = pWSWrapper->createEmptyMDWS(n_activated_dimensions, targ_dim_names,targ_dim_units, dim_min, dim_max);
    if(!spws)
    {
      g_log.error()<<"can not create target event workspace with :"<<n_activated_dimensions<<" dimensions\n";
      throw(std::invalid_argument("can not create target workspace"));
    }
    // Build up the box controller
    Mantid::API::BoxController_sptr bc = pWSWrapper->getBoxController();
    // Build up the box controller, using the properties in BoxControllerSettingsAlgorithm
    this->setBoxController(bc);
    // split boxes;
    pWSWrapper->splitBox();
  }

  // call selected algorithm
  pMethod algo =  alg_selector[algo_id];
  if(algo)
  {
    algo(this);
  }
  else
  {
    g_log.error()<<"requested undefined subalgorithm :"<<algo_id<<std::endl;
    throw(std::invalid_argument("undefined subalgoritm requested "));
  }
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(spws));

  // free the algorithm from the responsibility for the workspace to allow it to be deleted if necessary
  pWSWrapper->releaseWorkspace();
  return;
}

void 
ConvertToMDEvents::checkMaxMoreThenMin(const std::vector<double> &min,const std::vector<double> &max)const
{
  for(size_t i=0; i<min.size();i++)
  {
    if(max[i]<=min[i])
    {
      convert_log.error()<<" min value "<<min[i]<<" not less then max value"<<max[i]<<" in direction: "<<i<<std::endl;
      throw(std::invalid_argument("min limit not smaller then max limit"));
    }
  }
}

/**  
  *  The dimensions, which can be obtained from workspace are determined by the availible algorithms.
  *  E.g. an inelastic algorithm can transform matrix workspace into 2D-4D workpsace depending on what requested.
  *  If additional algorithms can be generated through algorithm template, this function shluld be modified accordingly
  *
  * @param inMatrixWS -- const pointer to const matrix workspace, which provides information about availible axis
  * @param Q_mode_req     -- what to do with Q-dimensions e.g. calculate either mod|Q| or Q3D;
  * @param dE_mode_req    -- desirable dE analysis mode (elastic, direct/indirect)
  * @param out_dim_names -- the vector of strings, with each string identify the dimensions names, derived from current workspace by the algorithm
  * @param out_dim_units -- vector of units for target workspace, if inelastic, one of the dimension units have to be DeltaE
*/
std::string 
ConvertToMDEvents::identifyMatrixAlg(API::MatrixWorkspace_const_sptr inMatrixWS, const std::string &Q_mode_req, const std::string &dE_mode_req,
                                     std::vector<std::string> &out_dim_names,std::vector<std::string> &out_dim_units)
{

    // dimension names present in input workspace
    std::vector<std::string> ws_dim_names;
    // unit IS-s the input workspace dimensions have
    std::vector<std::string> ws_dim_units;


   // get the X axis of input workspace, it has to be there; if not axis throws invalid index
    API::NumericAxis *pXAxis = dynamic_cast<API::NumericAxis *>(inMatrixWS->getAxis(0));
    if(!pXAxis ){
        convert_log.error()<<"Can not retrieve X axis from the source workspace: "<<inMatrixWS->getName()<<std::endl;
        throw(std::invalid_argument("Input workspace has to have X-axis"));
    }else{
        std::string Dim1Unit   = pXAxis->unit()->unitID();
        ws_dim_names.push_back(pXAxis->title());
        ws_dim_units.push_back(Dim1Unit);
    }
    // get optional Y axis which can be used in NoQ-kind of algorithms
    API::NumericAxis *pYAxis = dynamic_cast<API::NumericAxis *>(inMatrixWS->getAxis(1));
    if(pYAxis){
        std::string Dim2Unit = pYAxis->unit()->unitID();
        ws_dim_names.push_back(pYAxis->title());
        ws_dim_units.push_back(Dim2Unit);
    }          


    std::string Q_MODE_ID,DE_MODE_ID,CONV_MODE_ID,WS_ID;
    int nQ_dims(0),ndE_dims(0);
    // identify, what kind of input workspace is there:
    WS_ID = parseWSType(inMatrixWS);
    this->algo_id =WS_ID;
    // identify Q_mode
    Q_MODE_ID = parseQMode (Q_mode_req,ws_dim_names,ws_dim_units,out_dim_names,out_dim_units,nQ_dims);  
    this->algo_id += Q_MODE_ID;
    // identify dE mode    
    DE_MODE_ID= parseDEMode(Q_MODE_ID,dE_mode_req,ws_dim_units,out_dim_names,out_dim_units,ndE_dims,subalgorithm_units);
    // identify conversion mode;
    this->algo_id+=DE_MODE_ID; // just in case, to resolve cyclic dependence on emode, as CovMode can ask for emode (not any more)
    CONV_MODE_ID=parseConvMode(Q_MODE_ID,subalgorithm_units,ws_dim_units);
    this->algo_id+=CONV_MODE_ID;

    //the_WSalgID = Q_MODE_ID+DE_MODE_ID+CONV_MODE_ID;

    return this->algo_id;

}
/** Identify the Unit conversion mode, deployed by the subalgorith 
  * 
  *@param Q_MODE_ID     -- the momentum conversion mode. Unit conversion depends on it
  *@param natural_units -- units, expected by the subalgorithm from input workspace. All other untis have to be transformed into these. 
  *@param ws_dim_units  -- vector of input workspace dimensions units ID-s
  *
  *@returns CONV_MODE_ID -- the string identifier, which says what energy mode is deployed. Current  
*/
std::string
ConvertToMDEvents::parseConvMode(const std::string &Q_MODE_ID,const std::string &natural_units,const Strings &ws_dim_units)
{
    std::string CONV_MODE_ID("Unknown");
    // IDENTIFY UNITS CONVERSION MODE
    // get all known units:
    Strings AllKnownUnits= Kernel::UnitFactory::Instance().getKeys();
    // check if unit conversion is possible at all:
    if(Q_MODE_ID.compare(Q_modes[NoQ])!=0){
        if(is_member(AllKnownUnits,ws_dim_units[0])<0){
            convert_log.error() <<" Unknown unit"<<ws_dim_units[0]<<" along X-axis provided for conversion\n";
            throw(std::invalid_argument("ConvertToMDEvents needs to known units conversion"));
        }  
    }else{ // NoQ mode -- no conversion
        CONV_MODE_ID =ConvModes[ConvertNo];  
        return CONV_MODE_ID;
    }
    // are the existing units already what is needed, so no conversion?    
    if(ws_dim_units[0].compare(natural_units)==0){
        CONV_MODE_ID = ConvModes[ConvertNo];
    }else{
        // is a quick conversion availible?
        double factor,power;
        const Kernel::Unit_sptr pThisUnit=Kernel::UnitFactory::Instance().create(ws_dim_units[0]);
        if(pThisUnit->quickConversion(natural_units,factor,power)){
            CONV_MODE_ID = ConvModes[ConvFast];
        }else{
            if(ws_dim_units[0].compare("TOF")==0){ // may be it is TOF?
                CONV_MODE_ID = ConvModes[ConvFromTOF];
            }else{                                 // convert via TOF
                CONV_MODE_ID = ConvModes[ConvByTOF];
            }
        }
    } 

    return CONV_MODE_ID;
}

/** Identify the Energy conversion mode requested by user 
  * 
  *@param Q_MODE_ID     -- the momentum conversion mode. Energy conversion depends on it
  *@param dE_mode_req   -- What conversion algorithm user wants to deploy (direct/indirect elastic)
  *@param ws_dim_units  -- vector of input workspace dimensions units ID-s
  *@param out_dim_names [out] -- vector of names for target workspace, if inelastic, one of the dimension units have to be DeltaE
  *@param out_dim_units [out] -- vector of units for target workspace, if inelastic, one of the dimension units have to be DeltaE
  *@param ndE_dims [out]      -- number of additional dimensions, if inelastic, it would be one dimension more.
  *@param natural_units [out] -- name of the units, the algorithm expects to work with.
*/
std::string 
ConvertToMDEvents::parseDEMode(const std::string &Q_MODE_ID,const std::string &dE_mode_req,const Strings &ws_dim_units,Strings &out_dim_names,Strings &out_dim_units, 
                               int &ndE_dims,std::string &natural_units)
{
    if(is_member(dE_modes,dE_mode_req)<0){
         convert_log.error()<<" dE-mode: "<<dE_mode_req<<" not recognized\n";
         throw(std::invalid_argument(" Non-existing dE-mode"));
    }
    ndE_dims = 0;

    std::string DE_MODE_ID= dE_mode_req;
    // no_Q mode can only be compartible with no_dE mode
    if((Q_MODE_ID.compare(Q_modes[NoQ])==0)){
        DE_MODE_ID = dE_modes[ANY_Mode];
      // no-Q mode -- no conversion, so natural units are the one, already used by the workspace
        natural_units=ws_dim_units[0];   
    }
    // inelastic modes have one additional dimension and need special units on X-axis
    if((DE_MODE_ID.compare(dE_modes[Direct])==0)||(DE_MODE_ID.compare(dE_modes[Indir])==0)){
        ndE_dims = 1;
        out_dim_names.push_back("DeltaE");
        out_dim_units.push_back("DeltaE");
        // natural units defined in subalgorithm doing the conversion and their ID has to be defined correctly in class constructor
        natural_units = native_inelastic_unitID;
    }

    if(DE_MODE_ID.compare(dE_modes[Elastic])==0){
        natural_units = native_elastic_unitID;
    }    
    return DE_MODE_ID;

}
/** Identify the Momentum conversion mode requested by user
  * 
  *@param Q_mode_req    -- What conversion algorithm user wants to deploy (Q3d, modQ, no Q)
  *@param ws_dim_names  -- vector of input workspace dimensions names 
  *@param ws_dim_units  -- vector of input workspace dimensions units ID-s
  *@param out_dim_names [out] -- vector of dimension names for momentuns in target workspace
  *@param out_dim_units [out] -- vector of units for target workspace
  *@param nQ_dims [out]       -- number of Q or other dimensions. When converting into Q, it is 1 or 3 dimensions, if NoQ -- workspace dimensions are copied.
*/
std::string 
ConvertToMDEvents::parseQMode(const std::string &Q_mode_req,const Strings &ws_dim_names,const Strings &ws_dim_units,Strings &out_dim_names,Strings &out_dim_units, int &nQ_dims)
{
    std::string Q_MODE_ID("Unknown");
    if(is_member(Q_modes,Q_mode_req)<0){
         convert_log.error()<<" Q-mode: "<<Q_mode_req<<" not recognized\n";
         throw(std::invalid_argument(" Non-existing Q-mode"));
    }
    // Q_mode (one of 3 possible)  
    if(Q_mode_req.compare(Q_modes[NoQ])==0)
    { 
        nQ_dims      = int(ws_dim_names.size());
        Q_MODE_ID    = Q_modes[NoQ];
        out_dim_names= ws_dim_names;
        out_dim_units= ws_dim_units;
    }
    if(Q_mode_req.compare(Q_modes[modQ])==0) // At the moment we assume that |Q| make sense for inelastic only,      
    {  // so the only one variable is availible form the workspace. 
        nQ_dims=1;      
        out_dim_names.resize(1);
        out_dim_units.resize(1);
        out_dim_names[0] = "|Q|";
        out_dim_units[0] = native_elastic_unitID;
        Q_MODE_ID = Q_modes[modQ];

    }
    if((Q_mode_req.compare(Q_modes[Q3D])==0))
    {
       nQ_dims=3;
       out_dim_names.resize(3);       
       out_dim_names[0]="Q_x";
       out_dim_names[1]="Q_y";
       out_dim_names[2]="Q_z";
       Q_MODE_ID = Q_modes[Q3D];
       out_dim_units.assign(3,native_elastic_unitID);

    }
    return Q_MODE_ID;
}

/** identify what kind of input workspace is provided as input argument
 *
 *@param inMatrixWS  a pointer to the workspace, obtained from analysis data service
 *
 *@returns   -- the ID of the workspace of one of the supported types. Throws if can not dynamiucally cast the pointer to the workspace:
*/
std::string 
ConvertToMDEvents::parseWSType(API::MatrixWorkspace_const_sptr inMatrixWS)const
{
    const DataObjects::EventWorkspace *pWSEv= dynamic_cast<const DataObjects::EventWorkspace *>(inMatrixWS.get());
    if(pWSEv){
        return SupportedWS[EventWSType];
    }

    const DataObjects::Workspace2D *pMWS2D = dynamic_cast<const DataObjects::Workspace2D *>(inMatrixWS.get());
    if(pMWS2D){
        return SupportedWS[Workspace2DType];
    }
 
    convert_log.error()<<" Unsupported workspace type provided. Currently supported types are:\n";
    for(int i=0;i<NInWSTypes;i++){
        convert_log.error()<<" WS ID: "<<SupportedWS[i];
    }
    convert_log.error()<<std::endl;
    throw(std::invalid_argument("Unsupported worspace type provided"));
    return "";
}

/** function processes the input arguments and tries to establish what subalgorithm should be deployed; 
    *
    * @param inWS           -- input workspace (2D or Events)
    * @param Q_mode_req     -- what to do with Q-dimensions e.g. calculate either mod|Q| or Q3D;
    * @param dE_mode_req    -- desirable dE analysis mode (elastic, direct/indirect)
    * @param other_dim_names  -- vector of other dimension names requested by the algorithm
    * @param dim_names_requested [out]  -- dimension names for the target workspace
    * @param dim_units_requested [out] -- dimension units for the target workspace
*/
std::string 
ConvertToMDEvents::identifyTheAlg(API::MatrixWorkspace_const_sptr inWS,const std::string &Q_mode_req, 
                                 const std::string &dE_mode_req,const std::vector<std::string> &other_dim_names,
                                 std::vector<std::string> &dim_names_requested,std::vector<std::string> &dim_units_requested)
{

   std::vector<std::string> ws_dim_names,ws_dim_units;
   std::string the_algID;

   // identify the matrix conversion part of subalgorithm as function of user input and workspace Matrix parameters (axis)
   the_algID = identifyMatrixAlg(inWS, Q_mode_req, dE_mode_req,ws_dim_names,ws_dim_units);
   if(the_algID.find("Unknown")!=std::string::npos){
       convert_log.error()<<" Input parameters indentify uncomplete algorithm ID: "<<the_algID<<std::endl;
       throw(std::logic_error("can not parse input parameters propertly"));
   }

   // retrieve additional dimension names and dimension units, which can be derived from the workspace properties;
   std::vector<std::string> all_add_dim_names,all_add_dim_units;
   this->getAddDimensionNames(inWS,all_add_dim_names,all_add_dim_units);

   // check if additional dimension names can satisfy the user requests:
   std::vector<std::string> add_dim_names,add_dim_units;
   for(size_t i=0;i<other_dim_names.size();i++){
       int n_dim=is_member(all_add_dim_names,other_dim_names[i]);
       if(n_dim<0){
           convert_log.error()<<" Dimension :"<<other_dim_names[i]<<" requested but can not be derived from the input workspace\n";
           throw(std::invalid_argument(" Undefined dimension"));
       }
       add_dim_names.push_back(all_add_dim_names[n_dim]);
       add_dim_units.push_back(all_add_dim_units[n_dim]);
   }
   // assign output:

   dim_names_requested = ws_dim_names;
   dim_units_requested = ws_dim_units;
   // add additional dimensions (from properties)
   dim_names_requested.insert(dim_names_requested.end(),add_dim_names.begin(),add_dim_names.end());
   dim_units_requested.insert(dim_units_requested.end(),add_dim_units.begin(),add_dim_units.end());

   size_t nDims      = dim_names_requested.size();

   // Sanity checks:
    if(nDims<3&&(the_algID.find(Q_modes[Q3D])!=std::string::npos)){
        convert_log.error()<<"Algorithm with ID:"<<the_algID<<" should produce at least 3 dimensions and it requested to provie just:"<<nDims<<" dims \n";
        throw(std::logic_error("can not parse input parameters propertly"));
    }
    // we have currenlty instanciated only N input dimensions. See algorithm constructor to change that. 
    if(nDims>pWSWrapper->getMaxNDim()){
        convert_log.error()<<"Can not currently deal with more then: "<<pWSWrapper->getMaxNDim()<< " dimesnions, but requested: "<<nDims<<std::endl;
        throw(std::invalid_argument(" Too many dimensions requested "));
    }

    int emode;
    // any inelastic mode or unit conversion involing TOF needs Ei to be among the input workspace properties
    if (!Q_mode_req.empty()){
        emode = getEMode(this);
    }else{
        emode = -1;
    }
    //if((emode == 1)||(emode == 2)||(the_algID.find("TOF")!=std::string::npos))
    if((emode == 1)||(emode == 2))
    {        
        if(!inWS->run().hasProperty("Ei")){
            convert_log.error()<<" Conversion sub-algorithm with ID: "<<the_algID<<" needs input energy to be present among run properties\n";
            throw(std::invalid_argument(" Needs Input energy to be present "));
        }
    }

    this->n_activated_dimensions = nDims;

    return the_algID;

}

/** function returns the list of the property names, which can be treated as additional dimensions present in current matrix workspace 
 * TODO: Currenly logically wrong (at least for inelastic)  Specific processed properties have to be introudced
 * 
 * @param inMatrixWS -- shared pointer to input workspace for analysis
 * @param add_dim_names [out] -- the ID-s for the dimension names, which can be obtained from the workspace
 * @param add_dim_units [out] -- the Units ID-s (if any) existing dimensions
*/
void
ConvertToMDEvents::getAddDimensionNames(MatrixWorkspace_const_sptr inMatrixWS,std::vector<std::string> &add_dim_names,std::vector<std::string> &add_dim_units)const
{   
    // get dimension names from properties
    // TODO: this should be only special processed properties, not all of them, as it is at the moment
    const std::vector< Kernel::Property * > run_properties =  inMatrixWS->run().getProperties();  

    add_dim_names.resize(run_properties.size());    
    add_dim_units.resize(run_properties.size());

    // extract names for all properties, which can be treated as dimension names;
    for(size_t i=0;i<run_properties.size();i++){
        add_dim_names[i]=run_properties[i]->name();
        std::string UnitID = run_properties[i]->units();
       // if(UnitID.empty()||(UnitID.compare("Empty")==0)){ // it is questionable if we want to have unit ID equal to the dimension name and not empty
        //    UnitID =add_dim_names[i];
        //}
        add_dim_units[i]=UnitID;
    }

}


/** The matrix to convert neutron momentums into the fractional coordinate system   */
std::vector<double>
ConvertToMDEvents::getTransfMatrix(API::MatrixWorkspace_sptr inWS,const Kernel::V3D &u, const Kernel::V3D &v,bool is_powder)const
{
  
    Kernel::Matrix<double> mat(3,3);
    mat.identityMatrix();

    if(!is_powder){
        try{
           // set the transformation matrix on the basis of the oriented lattice
           Geometry::OrientedLattice Latt = inWS->sample().getOrientedLattice();

          // thansform the lattice above into the notional coordinate system related to projection vectors u,v;
           Kernel::Matrix<double> umat = Latt.setUFromVectors(u,v);

           Kernel::Matrix<double> gon =inWS->run().getGoniometer().getR();

          // Obtain the transformation matrix:
           mat = umat*gon ; //*(2*M_PI)?;
           mat.Invert();
        }catch(std::runtime_error &){
            convert_log.warning()<<" Can not obtain transformation matrix from the input workspace: "<<inWS->name()<<" as no oriented lattice has been defined. Use unit transformation matrix anyway\n";
        }
    }
    std::vector<double> rotMat = mat.get_vector();
    return rotMat;
}

/** function extracts the coordinates from additional workspace porperties and places them to proper position within the vector of MD coodinates for 
    the particular workspace.

    @param Coord             -- vector of coordinates for current multidimensional event
    @param nd                -- number of the event's dimensions
    @param n_ws_properties   -- number of dimensions, provided by the workspace itself. E.g., processed inelastic matrix
                                workspace with provides 4 dimensions, matrix workspace in elastic mode -- 3 dimensions, powder 
                                -- 1 for elastic and 2 for inelastic mode. Number of these properties is determined by the deployed algorithm
                                The coordinates, obtained from the workspace placed first in the array of coordinates, and the coordinates, 
                                obtained from dimensions placed after them. 
    *@returns        -- true if all coordinates are within the range allowed for the algorithm and false otherwise

 */
bool 
ConvertToMDEvents::fillAddProperties(std::vector<coord_t> &Coord,size_t nd,size_t n_ws_properties)
{
     for(size_t i=n_ws_properties;i<nd;i++){
         //HACK: A METHOD, Which converts TSP into value, correspondent to time scale of matrix workspace has to be developed and deployed!
         Kernel::Property *pProperty = (inWS2D->run().getProperty(this->targ_dim_names[i]));
         Kernel::TimeSeriesProperty<double> *run_property = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pProperty);  
         if(run_property){
                Coord[i]=run_property->firstValue();
         }else{
              // e.g Ei can be a property and dimenson
              Kernel::PropertyWithValue<double> *proc_property = dynamic_cast<Kernel::PropertyWithValue<double> *>(pProperty);  
              if(!proc_property){
                 convert_log.error()<<" property: "<<this->targ_dim_names[i]<<" is neither a time series (run) property nor a property with double value\n";
                 throw(std::invalid_argument(" can not interpret property, used as dimension"));
              }
              Coord[i]  = *(proc_property);
         }
        if(Coord[i]<dim_min[i] || Coord[i]>=dim_max[i])return false;
     }
     return true;
}
//
void 
ConvertToMDEvents::checkUVsettings(const std::vector<double> &ut,const std::vector<double> &vt,Kernel::V3D &u,Kernel::V3D &v)const
{
//identify if u,v are present among input parameters and use defaults if not
    bool u_default(true),v_default(true);
    if(!ut.empty()){
        if(ut.size()==3){ u_default =false;
        }else{convert_log.warning() <<" u projection vector specified but its dimensions are not equal to 3, using default values [1,0,0]\n";
        }
    }
    if(!vt.empty()){
        if(vt.size()==3){ v_default  =false;
        }else{ convert_log.warning() <<" v projection vector specified but its dimensions are not equal to 3, using default values [0,1,0]\n";
        }
    }
    if(u_default){  
        u[0] = 1;         u[1] = 0;        u[2] = 0;
    }else{
        u[0] = ut[0];     u[1] = ut[1];    u[2] = ut[2];
    }
    if(v_default){
        v[0] = 0;         v[1] = 1;        v[2] = 0;
    }else{
        v[0] = vt[0];     v[1] = vt[1];    v[2] = vt[2];
    }
}
// TEMPLATES INSTANTIATION: User encouraged to specialize its own specific algorithm 
//e.g.
// template<> void ConvertToMDEvents::processQND<modQ,Elastic,ConvertNo>()
// {
//   User specific code for workspace  processed to obtain modQ in elastic mode, without unit conversion:
// }
//----------------------------------------------------------------------------------------------
// AUTOINSTANSIATION OF EXISTING CODE:
// Templated loop over dependant templated arguments
template<Q_state Q, size_t NumAlgorithms=0>
class LOOP_ND{
private:
    enum{
        CONV = NumAlgorithms%NConvUintsStates,           // internal oop over conversion modes, the variable changes first
        MODE = (NumAlgorithms/NConvUintsStates)%ANY_Mode // one level up loop over momentum conversion mode  
    
    };
  public:
    static inline void EXEC(ConvertToMDEvents *pH){
                
            std::string Key0=pH->Q_modes[Q]+pH->dE_modes[MODE]+pH->ConvModes[CONV];

            std::string Key;
            Key = pH->SupportedWS[Workspace2DType]+Key0;
            pH->alg_selector.insert(std::pair<std::string,pMethod>(Key,
                &ConvertToMDEvents::processQNDHWS<Q,static_cast<AnalMode>(MODE),static_cast<CnvrtUnits>(CONV)>));
/*#ifdef _DEBUG
            std::cout<<" Instansiating algorithm with ID: "<<Key<<std::endl;
#endif*/
            Key = pH->SupportedWS[EventWSType]+Key0;
            pH->alg_selector.insert(std::pair<std::string,pMethod>(Key,
                &ConvertToMDEvents::processQNDEWS<Q,static_cast<AnalMode>(MODE),static_cast<CnvrtUnits>(CONV)>));
/*#ifdef _DEBUG
            std::cout<<" Instansiating algorithm with ID: "<<Key<<std::endl;
#endif   */ 
         LOOP_ND<Q, NumAlgorithms+1>::EXEC(pH);
    }
};

// Templated loop specialization for the noQ case
template< size_t NumAlgorithms>
class LOOP_ND<NoQ,NumAlgorithms>{
private:
    enum{
        CONV = NumAlgorithms%NConvUintsStates       // internal Loop over conversion modes, the variable changes first
        //MODE => noQ -- no mode conversion ANY_Mode, 
 
      
    };
  public:
    static inline void EXEC(ConvertToMDEvents *pH){
                
            std::string Key0 = pH->Q_modes[NoQ]+pH->dE_modes[ANY_Mode]+pH->ConvModes[CONV];
            std::string Key;

            Key = pH->SupportedWS[Workspace2DType]+Key0;
            pH->alg_selector.insert(std::pair<std::string,pMethod>(Key,
                &ConvertToMDEvents::processQNDHWS<NoQ,ANY_Mode,static_cast<CnvrtUnits>(CONV)>));
//#ifdef _DEBUG
//            std::cout<<" Instansiating algorithm with ID: "<<Key<<std::endl;
//#endif

            Key = pH->SupportedWS[EventWSType]+Key0;
            pH->alg_selector.insert(std::pair<std::string,pMethod>(Key,
                &ConvertToMDEvents::processQNDEWS<NoQ,ANY_Mode,static_cast<CnvrtUnits>(CONV)>));
//#ifdef _DEBUG
//            std::cout<<" Instansiating algorithm with ID: "<<Key<<std::endl;
//#endif


            LOOP_ND<NoQ,NumAlgorithms+1>::EXEC(pH);
    }
};

// Q3d and modQ terminator
template<Q_state Q >
class LOOP_ND<Q,static_cast<size_t>(ANY_Mode*NConvUintsStates) >{
  public:
      static inline void EXEC(ConvertToMDEvents *pH){UNUSED_ARG(pH);} 
};

// any mode terminator
template<>
class LOOP_ND<NoQ,static_cast<size_t>(NConvUintsStates) >{
  public:
      static inline void EXEC(ConvertToMDEvents *pH){UNUSED_ARG(pH);} 
};


/** Constructor 
 *  needs to pick up all known algorithms. 
*/
ConvertToMDEvents::ConvertToMDEvents():
Q_modes(NQStates),
dE_modes(4),
ConvModes(NConvUintsStates),
SupportedWS(NInWSTypes),
// The conversion subalgorithm processes input data expressed in these units; 
// Change of the units have to be accompanied by correspondent change in conversion subalgorithm
native_elastic_unitID("Momentum"), 
native_inelastic_unitID("DeltaE")
{
     Q_modes[modQ] = "|Q|";
     Q_modes[Q3D]  = "QxQyQz";    
     Q_modes[NoQ]  = "";    // no Q dimension (does it have any interest&relevance to ISIS/SNS?) 
     dE_modes[ANY_Mode]  = ""; // no Q uses it to run without conversion. 
     dE_modes[Direct]    = "Direct";
     dE_modes[Indir]     = "Indirect";
     dE_modes[Elastic]   = "Elastic";
     // possible unit conversion modes
     ConvModes[ConvertNo]  = "CnvNo";
     ConvModes[ConvFast]   = "CnvFast";
     ConvModes[ConvByTOF]  = "CnvByTOF";
     ConvModes[ConvFromTOF]= "CnvFromTOF";
     // possible input workspace ID-s
     SupportedWS[Workspace2DType] = "WS2D";
     SupportedWS[EventWSType]     = "WSEvent";

// Subalgorithm factories:
// NoQ --> any Analysis mode will do as it does not depend on it; we may want to convert unuts
   LOOP_ND<NoQ,0>::EXEC(this);
 
// MOD Q
    LOOP_ND<modQ,0>::EXEC(this);

 // Q3D
    LOOP_ND<Q3D,0>::EXEC(this);
  
}

//

} // namespace Mantid
} // namespace MDAlgorithms


