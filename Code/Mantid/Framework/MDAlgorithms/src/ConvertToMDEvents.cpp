/*WIKI* 
    Transfrom a workspace into MD Event workspace with components defined by user. 
   
    Gateway for number of subalgorithms, some are very important, some are questionable 
    Intended to cover wide range of cases; 

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
// the parameter, which specifies maximal default number of dimensions, the algorithm accepts. See constructor. 
   static const int MAX_NDIM=8;


// logger for loading workspaces  
   Kernel::Logger& ConvertToMDEvents::convert_log =Kernel::Logger::get("MD-Algorithms");

// the variable describes the locations of the preprocessed detectors, which can be stored and reused it the algorithm runs for more once;
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
  *  Natural units are the units, which subalgorithm is working with without any initial transformation.
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
  *@returns the reference to the structure. Throws if the structure has not been defined
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
  *@returns the energy. Throws if the energy property is not defined or can not be retrieved from the workspace
*/
double
ConvertToMDEvents::getEi(ConvertToMDEvents const *const pHost)
{
    if(!pHost->inWS2D){
        convert_log.error()<<"getEi: invoked when input workspace is undefined\n";
        throw(std::logic_error(" should not call this function when workpace is undefined"));
    }
    Kernel::PropertyWithValue<double>  *pProp  =dynamic_cast<Kernel::PropertyWithValue<double>  *>(pHost->inWS2D->run().getProperty("Ei"));
    if(!pProp){
        convert_log.error()<<"getEi: can not obtain incident energy of neutrons\n";
        throw(std::logic_error(" should not call this function when incident energy is undefined"));
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
      ws_valid->add(new HistogramValidator<>);
      ws_valid->add(new InstrumentValidator<>);
      // the validator which checks if the workspace has axis and any units
      ws_valid->add(new WorkspaceUnitValidator<>(""));


    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,ws_valid),
        "An input Matrix Workspace 2D has to have units, which can be used as one of the dimensions ");
   
     declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace. If the workspace already exists, then the events will be added to it.");

     
     /// this variable describes default possible ID-s for Q-dimensions   
     declareProperty("QDimensions",Q_modes[modQ],new ListValidator(Q_modes),
                              "You can to trsansfer sourcs workspace dimensions into target worskpace or process mod(Q) (1 dimension) or QxQyQz (3 dimensions) in Q space",Direction::InOut);        

     /// this variable describes implemented modes for energy transfer analysis
     declareProperty("dEAnalysisMode",dE_modes[Direct],new ListValidator(dE_modes),
                              "You can to trsansfer sourcs workspace dimensions into target worskpace or process mod(Q) (1 dimension) or QxQyQz (3 dimensions) in Q space",Direction::InOut);        

     
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
    
    declareProperty(new ArrayProperty<double>("u","1,0,0",new ArrayLengthValidator<double>(3)), "first  base vector (in hkl) defining fractional coordinate system for neutron diffraction");
    declareProperty(new ArrayProperty<double>("v","0,1,0",new ArrayLengthValidator<double>(3)), "second base vector (in hkl) defining fractional coordinate system for neutron diffraction");  

   // Box controller properties. These are the defaults
    this->initBoxControllerProps("5" /*SplitInto*/, 1500 /*SplitThreshold*/, 20 /*MaxRecursionDepth*/);

 

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
  // 
  Instrument_const_sptr instrument = inputWS->getInstrument();
  //
  IObjComponent_const_sptr source = instrument->getSource();
  IObjComponent_const_sptr sample = instrument->getSample();
  if ((!source) || (!sample)) {
    convert_log.error()<<" Instrument is not fully defined. Can not identify source or sample\n";
    throw Exception::InstrumentDefinitionError("Instrubment not sufficiently defined: failed to get source and/or sample");
  }

  // L1
  try{
    det_loc.L1 = source->getDistance(*sample);
    convert_log.debug() << "Source-sample distance: " << det_loc.L1 << std::endl;
  }catch (Exception::NotFoundError &)  {
    convert_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWS->getTitle());
  }
  //
  const size_t nHist = inputWS->getNumberHistograms();

    det_loc.det_dir.resize(nHist);
    det_loc.det_id.resize(nHist);
    det_loc.L2.resize(nHist);
    det_loc.TwoTheta.resize(nHist);
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
     det_loc.L2[ic]     = spDet->getDistance(*sample);

     double polar        =  inputWS->detectorTwoTheta(spDet);
     det_loc.TwoTheta[ic]=  polar;
     double azim         =  spDet->getPhi();    

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
       det_loc.L2.resize(ic);
       det_loc.TwoTheta.resize(ic);
   }

}
  //----------------------------------------------------------------------------------------------
  /* Execute the algorithm.   */
void ConvertToMDEvents::exec(){
    // in case of subsequent calls
    this->algo_id="";
    // -------- Input workspace 
    MatrixWorkspace_sptr inMatrixWS = getProperty("InputWorkspace");
    if(!inMatrixWS){
        g_log.error()<<" can not obtain input matrix workspace from analysis data service\n";
    }
    inWS2D                           = boost::dynamic_pointer_cast<Workspace2D>(inMatrixWS);
    // ------- Is there any output workspace?
    // shared pointer to target workspace
    API::IMDEventWorkspace_sptr spws = getProperty("OutputWorkspace");
    bool create_new_ws(false);
    if(!spws){
        create_new_ws = true;
    }

    std::vector<double> ut = getProperty("u");
    std::vector<double> vt = getProperty("v");
    Kernel::V3D u(ut[0],ut[1],ut[2]),v(vt[0],vt[1],vt[2]);

    // set up target coordinate system
    this ->rotMatrix = getTransfMatrix(inWS2D,u,v);


    // if new workspace is created, its properties are determened by the user's input
    if (create_new_ws){
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
        if(dim_min.size()!=dim_max.size()||dim_min.size()!=n_activated_dimensions){
            g_log.error()<<" number of specified min dimension values:"<<dim_min.size()<<", number of max values: "<<dim_max.size()<<
                           " and total number of target dimensions"<<n_activated_dimensions<<" are not consistent\n";
            throw(std::invalid_argument("wrong number of dimension limits"));
        }

   // the output dimensions and almost everything else will be determined by the dimensions of the target workspace
   // user input is mainly ignored
    }else{ 
         throw(Kernel::Exception::NotImplementedError("Not Yet Implemented"));
          dim_min.assign(n_activated_dimensions,-1);
          dim_max.assign(n_activated_dimensions,1);
    }
    

    bool reuse_preprocecced_detectors = getProperty("UsePreprocessedDetectors");
    if(!(reuse_preprocecced_detectors&&det_loc.is_defined()))process_detectors_positions(inWS2D);
      
   
     if(create_new_ws){
         spws = ws_creator[n_activated_dimensions](this);
         if(!spws){
             g_log.error()<<"can not create target event workspace with :"<<n_activated_dimensions<<" dimensions\n";
             throw(std::invalid_argument("can not create target workspace"));
         } 
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


/**  
  *  The dimensions, which can be obtained from workspace are determined by the availible algorithms.
  *  E.g. an inelastic algorithm can transform matrix workspace into 2D-4D workpsace depending on what requested.
  *  If additional algorithms can be generated through algorithm template, this function shluld be modified accordingly
  *
  * @param inMatrixWS -- const pointer to const matrix workspace, which provides information about availible axis
  *
  * @returns out_dim_names -- the vector of strings, with each string identify the dimensions names, derived from current workspace by the algorithm
*/
std::string 
ConvertToMDEvents::identifyMatrixAlg(API::MatrixWorkspace_const_sptr inMatrixWS, const std::string &Q_mode_req, const std::string &dE_mode_req,
                                     std::vector<std::string> &out_dim_names,std::vector<std::string> &out_dim_units)
{

    // dimension names present in input workspace
    std::vector<std::string> ws_dim_names;
    // units IS-s the input workspace dimensions have
    std::vector<std::string> ws_dim_units;

  
    // result: AlgorithmID 
    std::string the_WSalgID;   
 
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


    std::string Q_MODE_ID,DE_MODE_ID,CONV_MODE_ID;
    int nQ_dims(0),ndE_dims(0);

    // identify Q_mode
    Q_MODE_ID = parseQMode (Q_mode_req,ws_dim_names,ws_dim_units,out_dim_names,out_dim_units,nQ_dims);  
    // identify dE mode    
    DE_MODE_ID= parseDEMode(Q_MODE_ID,dE_mode_req,ws_dim_units,out_dim_names,out_dim_units,ndE_dims,subalgorithm_units);
    // identify conversion mode;
    this->algo_id=Q_MODE_ID+DE_MODE_ID; // just in case, to resolve cyclic dependence on emode, as CovMode asks for emode
    CONV_MODE_ID=parseConvMode(Q_MODE_ID,subalgorithm_units,ws_dim_units);

    the_WSalgID = Q_MODE_ID+DE_MODE_ID+CONV_MODE_ID;

    return the_WSalgID;

}
/** Identify the Unit conversion mode, deployed by the subalgorith 
  * 
  *@param Q_MODE_ID     -- the momentum conversion mode. Unit conversion depends on it
  *@param natural_units -- units, expected by the subalgorithm from input workspace. All other untis have to be transformed into these. 
  *@param ws_dim_names  -- vector of input workspace dimensions names 
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
            CONV_MODE_ID = ConvModes[ConvertFast];
        }else{
            if(ws_dim_units[0].compare("TOF")==0){ // may be it is TOF?
                CONV_MODE_ID = ConvModes[ConvFromTOF];
            }else{                                 // convert via TOF
                CONV_MODE_ID = ConvModes[ConvByTOF];
                if(getEMode(this) == 0){
                    convert_log.error() <<" conversion via TOF is not availible in elastic mode\n";
                    convert_log.error() <<" can not convert input workspce X-axis units: "<<ws_dim_units[0]<<" into: "<<getNativeUnitsID(this)
                                        <<" needed by elastic conversion\n";
                    throw(std::invalid_argument(" wrong X-axis units"));
                }
            }
        }
    } 

    return CONV_MODE_ID;
}

/** Identify the Energy conversion mode requested by user 
  * 
  *@param Q_MODE_ID     -- the momentum conversion mode. Energy conversion depends on it
  *@param dE_mode_req   -- What conversion algorithm user wants to deploy (direct/indirect elastic)
  *@param ws_dim_names  -- vector of input workspace dimensions names 
  *@param ws_dim_units  -- vector of input workspace dimensions units ID-s
  *
  *@returns out_dim_names -- vector of names for target workspace, if inelastic, one of the dimension units have to be DeltaE
  *@returns out_dim_units -- vector of units for target workspace, if inelastic, one of the dimension units have to be DeltaE
  *@returns ndE_dims      -- number of additional dimensions, if inelastic, it would be one dimension more.
  *@returns natural_units -- name of the units, the algorithm expects to work with. 
  *@returns emode        -- the integer number of the mode, (0 -- elastic, 1/2-Direct/Indirect) used by unit conversion procedute
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
/** Identify the Momentud conversion mode requested by user
  * 
  *@param Q_mode_req    -- What conversion algorithm user wants to deploy (Q3d, modQ, no Q)
  *@param ws_dim_names  -- vector of input workspace dimensions names 
  *@param ws_dim_units  -- vector of input workspace dimensions units ID-s
  *
  *@returns out_dim_names -- vector of dimension names for momentuns in target workspace 
  *@returns out_dim_units -- vector of units for target workspace
  *@returns nQ_dims       -- number of Q or other dimensions. When converting into Q, it is 1 or 3 dimensions, if NoQ -- workspace dimensions are copied. 
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

/** function processes the input arguments and tries to establish what algorithm should be deployed; 
    *
    * @param inWS2D         -- input 2D workspace
    * @param Q_mode_req     -- what to do with Q-dimensions e.g. calculate either mod|Q| or Q3D;
    * @param dE_mode_req    -- desirable dE analysis mode (elastic, direct/indirect)
    * @param dim_requested  -- vector of other dimension names requested by the algorithm
    *
    * @return the_algID             -- the string, identifying one of the known algorithms; if unknown, should fail. 
    * @return: dim_names_requested  -- dimension names for the target workspace
    * @return: dim_uinits_requested -- dimension units for the target workspace
*/
std::string 
ConvertToMDEvents::identifyTheAlg(API::MatrixWorkspace_const_sptr inWS2D,const std::string &Q_mode_req, 
                                 const std::string &dE_mode_req,const std::vector<std::string> &other_dim_names,
                                 std::vector<std::string> &dim_names_requested,std::vector<std::string> &dim_units_requested)
{

   std::vector<std::string> ws_dim_names,ws_dim_units;
   std::string the_algID;

   // identify the matrix conversion part of subalgorithm as function of user input and workspace Matrix parameters (axis)
   the_algID = identifyMatrixAlg(inWS2D, Q_mode_req, dE_mode_req,ws_dim_names,ws_dim_units);
   if(the_algID.find("Unknown")!=std::string::npos){
       convert_log.error()<<" Input parameters indentify uncomplete algorithm ID: "<<the_algID<<std::endl;
       throw(std::logic_error("can not parse input parameters propertly"));
   }

   // retrieve additional dimension names and dimension units, which can be derived from the workspace properties;
   std::vector<std::string> all_add_dim_names,all_add_dim_units;
   this->getAddDimensionNames(inWS2D,all_add_dim_names,all_add_dim_units);

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
    // we have currenlty instanciated only 8 input dimensions. See algorithm constructor to change that. 
    if(nDims>8){
        convert_log.error()<<"Can not currently produce more then 8 dimesnions, requested: "<<nDims<<std::endl;
        throw(std::invalid_argument(" Too many dimensions requested "));
    }

    // any inelastic mode or unit conversion involing TOF needs Ei to be among the input workspace properties
    int emode = getEMode(this);
    if((emode == 1)||(emode == 2)||(the_algID.find("TOD")!=std::string::npos))
    {        
        if(!inWS2D->run().hasProperty("Ei")){
            convert_log.error()<<" Conversion sub-algorithm with ID: "<<the_algID<<" needs input energy to be present among run properties\n";
            throw(std::invalid_argument(" Needs Input energy to be present "));
        }
    }

    //TODO: temporary, we will redefine the algorithm ID not to depend on dimension number in a future
    the_algID  = the_algID+boost::lexical_cast<std::string>(nDims);
    this->n_activated_dimensions = nDims;

    return the_algID;

}

/** function returns the list of the property names, which can be treated as additional dimensions present in current matrix workspace 
 * TODO: Currenly logically wrong (at least for inelastic)  Specific processed properties have to be introudced
 * 
 * @param inMatrixWS -- shared pointer to input workspace for analysis
 * 
 * @returns add_dim_names -- the ID-s for the dimension names, which can be obtained from the workspace
 * @returns add_dim_unit  -- the Units ID-s (if any) existing dimensions
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
ConvertToMDEvents::getTransfMatrix(API::MatrixWorkspace_sptr inWS2D,const Kernel::V3D &u, const Kernel::V3D &v)const
{
  
    // Set the matrix based on UB etc.
    Geometry::OrientedLattice Latt = inWS2D->sample().getOrientedLattice();

    // thansform the lattice above into the notional coordinate system related to projection vectors u,v;
    Kernel::Matrix<double> umat = Latt.setUFromVectors(u,v);

    Kernel::Matrix<double> gon =inWS2D->run().getGoniometer().getR();

  // Obtain the transformation matrix:
    Kernel::Matrix<double> mat = umat*gon ; //*(2*M_PI)?;
    mat.Invert();
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
     }
}

// TEMPLATES INSTANTIATION: User encouraged to specialize its own specific algorithm 
//e.g.
// template<> void ConvertToMDEvents::processQND<2,modQ>( API::IMDEventWorkspace *const)
// {
//   User specific code for target 2D workspace, processed to obtain modQ
// }
//----------------------------------------------------------------------------------------------
// AUTOINSTANSIATION OF EXISTING CODE:
// Templated loop over some templated arguments
template< size_t i, Q_state Q, AnalMode MODE, CnvrtUnits CONV >
class LOOP_ND{
  public:
    static inline void EXEC(ConvertToMDEvents *pH){
            LOOP_ND< i-1 , Q, MODE,CONV>::EXEC(pH);
            std::stringstream num;
            num << i;
            std::string Key = pH->Q_modes[Q]+pH->dE_modes[MODE]+pH->ConvModes[CONV]+num.str();

            pH->alg_selector.insert(std::pair<std::string,pMethod>(Key,&ConvertToMDEvents::processQND<i,Q,MODE,CONV>));
            pH->ws_creator.insert(std::pair<size_t,pWSCreator>(i,&ConvertToMDEvents::createEmptyEventWS<i>));
    }
};
template< Q_state Q, AnalMode MODE, CnvrtUnits CONV >
class LOOP_ND<2,Q,MODE,CONV>{
  public:
    static inline void EXEC(ConvertToMDEvents *pH){
            
            std::stringstream num;
            num << 2;
            std::string Key = pH->Q_modes[Q]+pH->dE_modes[MODE]+pH->ConvModes[CONV]+num.str();

            pH->alg_selector.insert(std::pair<std::string,pMethod>(Key,
                                   &ConvertToMDEvents::processQND<2,Q,MODE,CONV>));
            pH->ws_creator.insert(std::pair<size_t,pWSCreator>(2,&ConvertToMDEvents::createEmptyEventWS<2>));
#ifdef _DEBUG
            std::cout<<" Ending group by instansiating algorithm with ID: "<<Key<<std::endl;
#endif

    }
};
/** Constructor 
 *  needs to pick up all known algorithms. 
*/
ConvertToMDEvents::ConvertToMDEvents():
Q_modes(3),
dE_modes(4),
ConvModes(4),
// The conversion subalgorithm processes data in these units; 
// Change of the units have to be accompanied by correspondent change in conversion subalgorithm
native_elastic_unitID("MomentumTransfer"), // Why it is a transfer? Hope it is just a momentum
native_inelastic_unitID("DeltaE")
{
     Q_modes[modQ]="|Q|";
     Q_modes[Q3D] ="QxQyQz";    
     Q_modes[NoQ] ="";    // no Q dimension (does it have any interest&relevance to ISIS/SNS?) 
     dE_modes[ANY_Mode]  = "";
     dE_modes[Direct]    = "Direct";
     dE_modes[Indir]     = "Indirect";
     dE_modes[Elastic]   = "Elastic";
     // possible unit conversion modes
     ConvModes[ConvertNo]  ="CnvNo";
     ConvModes[ConvertFast]="CnvFast";
     ConvModes[ConvByTOF]  ="CnvByTOF";
     ConvModes[ConvFromTOF]="CnvFromTOF";
 
// NoQ --> any Analysis mode will do as it does not depend on it; we may want to convert unuts
    LOOP_ND<MAX_NDIM,NoQ,ANY_Mode,ConvertNo>::EXEC(this);
    LOOP_ND<MAX_NDIM,NoQ,ANY_Mode,ConvertFast>::EXEC(this);
    LOOP_ND<MAX_NDIM,NoQ,ANY_Mode,ConvFromTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,NoQ,ANY_Mode,ConvByTOF>::EXEC(this);
// MOD Q
    LOOP_ND<MAX_NDIM,modQ,Direct,ConvertNo>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Indir,ConvertNo>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Elastic,ConvertNo>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Direct,ConvertFast>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Indir,ConvertFast>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Elastic,ConvertFast>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Direct,ConvFromTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Indir,ConvFromTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Elastic,ConvFromTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Direct,ConvByTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Indir,ConvByTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,modQ,Elastic,ConvByTOF>::EXEC(this);

 // Q3D
    LOOP_ND<MAX_NDIM,Q3D,Direct,ConvertNo>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Indir,ConvertNo>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Elastic,ConvertNo>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Direct,ConvertFast>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Indir,ConvertFast>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Elastic,ConvertFast>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Direct,ConvFromTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Indir,ConvFromTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Elastic,ConvFromTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Direct,ConvByTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Indir,ConvByTOF>::EXEC(this);
    LOOP_ND<MAX_NDIM,Q3D,Elastic,ConvByTOF>::EXEC(this);


}

//

} // namespace Mantid
} // namespace MDAlgorithms


