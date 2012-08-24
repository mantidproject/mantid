/*WIKI* 
== Summary ==

Transforms a workspace into MDEvent workspace with dimensions defined by user. 
   
Gateway for set of subalgorithms, combined together to convert input 2D matrix workspace or event workspace with any units along X-axis into  multidimensional event workspace. 

Depending on the user input and the data, find in the input workspace, the algorithms transform the input workspace into 1 to 4 dimensional MDEvent workspace and adds to this workspace additional dimensions, which are described by the workspace properties and requested by user.

*WIKI*/

#include "MantidMDAlgorithms/ConvertToMD.h"

#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/ArrayLengthValidator.h"
//
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidMDEvents/MDWSTransform.h"
//
#include "MantidDataObjects/Workspace2D.h"

#include <algorithm>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidMDEvents/ConvToMDSelector.h"
#include "MantidMDEvents/MDTransfDEHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;
using namespace Mantid::MDEvents::CnvrtToMD;
namespace Mantid
{
namespace MDAlgorithms
{

// logger for the algorithm workspaces  
Kernel::Logger& ConvertToMD::g_Log =Kernel::Logger::get("MD-Algorithms");
// the variable describes the locations of the preprocessed detectors, which can be stored and reused if the algorithm runs more then once;
MDEvents::ConvToMDPreprocDet ConvertToMD::g_DetLoc;
//
Mantid::Kernel::Logger & 
ConvertToMD::getLogger(){return g_Log;}
//
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToMD)


// Sets documentation strings for this algorithm
void ConvertToMD::initDocs()
{
    this->setWikiSummary("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions.");
    this->setOptionalMessage("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions.");
}
//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToMD::~ConvertToMD()
{
    // if the algorithm has gone, then the preprocessed detectors should probably too
    g_DetLoc.clearAll();
}
//
//const double rad2deg = 180.0 / M_PI;
//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void 
ConvertToMD::init()
{
      auto ws_valid = boost::make_shared<CompositeValidator>();
      //
      ws_valid->add<InstrumentValidator>();
      // the validator which checks if the workspace has axis and any units
      ws_valid->add<WorkspaceUnitValidator>("");


    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,ws_valid),
        "An input Matrix Workspace (Matrix 2D or Event) with units along X-axis and defined instrument with defined sample");
   
     declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
                  "Name of the output MDEventWorkspace");
     declareProperty(new PropertyWithValue<bool>("OverwriteExisting", true, Direction::Input),
              "By default, existing Output Workspace will be replaced. Select false if you want to add new events to the workspace, which already exist.\n"
              " Can be very inefficient for file-based workspaces");

     std::vector<std::string> Q_modes = MDEvents::MDTransfFactory::Instance().getKeys();
     // something to do with different moments of thime when algorithm or test loads library. To avoid empty factory always do this. 
     if(Q_modes.empty()) Q_modes.assign(1,"ERROR IN LOADING Q-converters");
  
     /// this variable describes default possible ID-s for Q-dimensions   
     declareProperty("QDimensions",Q_modes[0],boost::make_shared<StringListValidator>(Q_modes),
         "You can to transfer source workspace into target MD workspace directly by supplying string ""CopyToMD""\n"
         " (No Q analysis, or Q conversion is performed),\n"
         "into mod(Q) (1 dimension) providing ""|Q|"" string or into 3 dimensions in Q space ""Q3D"". \n"
         " First mode used for copying data from input workspace into multidimensional target workspace, second -- mainly for powder analysis\n"
         "(though crystal as powder is also analysed in this mode) and the third -- for crystal analysis.\n",Direction::InOut); 

     MDEvents::MDWSTransform QScl;
     std::vector<std::string> QScales = QScl.getQScalings();
     declareProperty("QConversionScales",QScales[CnvrtToMD::NoScaling], boost::make_shared<StringListValidator>(QScales),
        "This property to normalize three momentums obtained in Q3D mode. Possible values are:\n"
        "  No Scaling,        -- momentums in Momentum or MomentumTransfer units  A^-1\n"
        "  Q in lattice units -- single scale, where all momentums are divided by the minimal reciprocal lattice vector 2*Pi/Max(a_latt)\n"
        "                        where a_lat is the maximal lattice parameter\n"
        "  Orthogonal HKL     -- three Q components are divided by 2pi/a,2pi/b and 2pi/c lattice vectors.\n"
        "  HKL                 -- converted to HKL (multiplied by B-matrix which is equivalent to Orthogonal HKL for rectilinear lattices.\n" 
        "This parameter is currently ignored in ""mod|Q|"" and ""CopyToMD"" modes and if a reciprocal lattice is not defined in the input workspace.");
     /// temporary
     MDEvents::MDTransfDEHelper AlldEModes;
     std::vector<std::string> dE_modes = AlldEModes.getEmodes();
     declareProperty("dEAnalysisMode",dE_modes[CnvrtToMD::Direct],boost::make_shared<StringListValidator>(dE_modes),
        "You can analyse neutron energy transfer in direct, indirect or elastic mode. The analysis mode has to correspond to experimental set up.\n"
        " Selecting inelastic mode increases the number of the target workspace dimensions by one. (by DeltaE -- the energy transfer)\n"
        """NoDE"" choice corresponds to ""CopyToMD"" analysis mode and is selected automatically if the QDimensions is set to ""CopyToMD""",Direction::InOut);                
     
    declareProperty(new ArrayProperty<std::string>("OtherDimensions",Direction::Input),
        " List(comma separated) of additional to Q and DeltaE variables which form additional (orthogonal) to Q dimensions"
        " in the target workspace (e.g. Temperature or Magnetic field).\n"
        " These variables had to be logged during experiment and the names of these variables "
        " have to coincide with the log names for the records of these variables in the source workspace");

    // this property is mainly for subalgorithms to set-up as they have to identify if they use the same instrument. 
    declareProperty(new PropertyWithValue<bool>("UsePreprocessedDetectors", true, Direction::Input), 
        "Store the part of the detectors transformation into reciprocal space to save/reuse it later.\n"
        " Useful if one expects to analyse number of different experiments obtained on the same instrument.\n"
        "<span style=""color:#FF0000""> Dangerous if one uses number of workspaces with modified derived instrument one after another. </span>"
        " In this case switch has to be set to false, as first instrument would be used for all workspaces othewise and no check for its validity is performed."); 

    declareProperty(new ArrayProperty<double>("MinValues"),
        "It has to be N comma separated values, where N is defined as: \n"
        "a) 1+delta(dE)+N_OtherDimensions if the first dimension (QDimensions property) is equal to |Q| or \n"
        "b) 3+delta(dE)+N_OtherDimensions if the first (3) dimensions (QDimensions property) equal  Q3D or \n\n"
        "c) (1 or 2)+N_OtherDimesnions if QDimesnins property is emtpty. \n"     
         " where delta(dE)==1 in direct and indirect modes and 0 otherwise\n"
         " In case c) the target workspace dimensions are defined by the [[units]] of the input workspace axis.\n\n"
         " This property contains minimal values for all dimensions.\n"
         " Momentum values expected to be in [A^-1] and energy transfer (if any) expressed in [meV]\n"
         " In case b), the target dimensions for Q3D are either momentums if QinHKL is false or are momentums divided by correspondent lattice parameters if QinHKL is true\n"
         " All other values are in the [[units]] they are expressed in their log files\n"
         " Values lower then the specified one will be ignored and not transferred into the target MD workspace\n");
//TODO:    " If a minimal target workspace range is higher then the one specified here, the target workspace range will be used instead " );

   declareProperty(new ArrayProperty<double>("MaxValues"),
         " A list of the same size and the same units as MinValues list"
         " Values higher or equal to the specified by this list will be ignored\n");
//TODO:    "If a maximal target workspace range is lower, then one of specified here, the target workspace range will be used instead" );
    
    declareProperty(new ArrayProperty<double>("Uproj"),
     "Optional: First base vector (in hkl) defining a new coordinate system for neutron scattering;\n"
     "Default (1,0,0).\n");
    declareProperty(new ArrayProperty<double>("Vproj"),
    "Optional:  Second base vector (in hkl) defining a new coordinate system for neutron scattering;\n"
    "Default (0,1,0).\n");
    declareProperty(new ArrayProperty<double>("Wproj"),
    "Optional:  Third base vector (in hkl) defining a new coordinate system for neutron scattering;\n"
    "Default (0,0,1).\n");
   // Box controller properties. These are the defaults
    this->initBoxControllerProps("5" /*SplitInto*/, 1000 /*SplitThreshold*/, 20 /*MaxRecursionDepth*/);
    // additional box controller settings property. 
    auto mustBeMoreThen1 = boost::make_shared<BoundedValidator<int> >();
    mustBeMoreThen1->setLower(1);

    declareProperty(
      new PropertyWithValue<int>("MinRecursionDepth", 1,mustBeMoreThen1),
      "Optional. If specified, then all the boxes will be split to this minimum recursion depth. 1 = one level of splitting, etc.\n"
      "Be careful using this since it can quickly create a huge number of boxes = (SplitInto ^ (MinRercursionDepth * NumDimensions)).\n"
      "But setting this property equal to MaxRecursionDepth property is necessary if one wants to generate multiple file based workspaces in order to merge them later\n");
    setPropertyGroup("MinRecursionDepth", getBoxSettingsGroupName());

 
}

 //----------------------------------------------------------------------------------------------
/* Execute the algorithm.   */
void ConvertToMD::exec()
{
  // initiate class which would deal with any dimension workspaces, handling 
  if(!m_OutWSWrapper)
  {
    m_OutWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
  }
  // -------- Input workspace
   m_InWS2D = getProperty("InputWorkspace");

  // ------- Is there any output workspace?
  // shared pointer to target workspace
  API::IMDEventWorkspace_sptr spws = getProperty("OutputWorkspace");
  bool create_new_ws(false);
  if(!spws)
  {
    create_new_ws = true;
  }
  else
  { 
      bool should_overwrite = getProperty("OverwriteExisting");
      if (should_overwrite)
      {
          create_new_ws=true;
      }else{
          create_new_ws=false;
      }
  }
 
  // Collate and Analyze the requests to the job, specified by the input parameters
  // what dimension names requested by the user by:
    //a) Q selector:
    std::string Q_mod_req                    = getProperty("QDimensions");
    //b) the energy exchange mode
    std::string dE_mod_req                   = getProperty("dEAnalysisMode");
    //c) other dim property;
    std::vector<std::string> other_dim_names = getProperty("OtherDimensions");
    //d) part of the procedure, specifying the target dimensions units. Currently only Q3D target units can be converted to different flavours of hkl
    std::string convert_to_                  = getProperty("QConversionScales");

// Build the target ws description as function of the input ws and the parameters, supplied to the algorithm 
    MDEvents::MDWSDescription targWSDescr;
   // set the min and max values for the dimensions from the input porperties
    std::vector<double> dimMin = getProperty("MinValues");
    std::vector<double> dimMax = getProperty("MaxValues");
    // verify that the number min/max values is equivalent to the number of dimensions defined by properties and min is less the
    targWSDescr.setMinMax(dimMin,dimMax);   
    targWSDescr.buildFromMatrixWS(m_InWS2D,Q_mod_req,dE_mod_req,other_dim_names);


  // instanciate class, responsible for defining Mslice-type projection
    MDEvents::MDWSTransform MsliceProj;
    if(create_new_ws)
    {
        //identify if u,v are present among input parameters and use defaults if not
        std::vector<double> ut = getProperty("UProj");
        std::vector<double> vt = getProperty("VProj");
        std::vector<double> wt = getProperty("WProj");
        try
        {
            MsliceProj.setUVvectors(ut,vt,wt);
        }
        catch(std::invalid_argument &)
        {
            g_log.error() << "The projections are coplanar. Will use defaults [1,0,0],[0,1,0] and [0,0,1]" << std::endl;
        }
       // otherwise input uv are ignored -> later it can be modified to set ub matrix if no given, but this may overcomplicate things. 


        // check if we are working in powder mode
        // set up target coordinate system and identify/set the (multi) dimension's names to use
         targWSDescr.m_RotMatrix = MsliceProj.getTransfMatrix(targWSDescr,convert_to_);     
      
    }
    else // user input is mainly ignored and everything is in old workspac
    {  

        // dimensions are already build, so build MDWS description from existing workspace
        MDEvents::MDWSDescription oldWSDescr;
        oldWSDescr.buildFromMDWS(spws);

        // some conversion parameters can not be defined by the target workspace. They have to be retrieved from the input workspace 
        // and derived from input parameters. 
        oldWSDescr.setUpMissingParameters(targWSDescr);      
        // check inconsistencies
        oldWSDescr.checkWSCorresponsMDWorkspace(targWSDescr);
        // reset new ws description name
        targWSDescr =oldWSDescr;
       // set up target coordinate system
        targWSDescr.m_RotMatrix = MsliceProj.getTransfMatrix(targWSDescr,convert_to_);
    
    }

    // Check what to do with detectors:  
    if(targWSDescr.isDetInfoLost())
    { // in NoQ mode one may not have DetPositions any more. Neither this information is needed for anything except data conversion interface. 
         g_DetLoc.buildFakeDetectorsPositions(m_InWS2D);
    }
    else  // preprocess or not the detectors positions
    {
          bool reuse_preprocecced_detectors = getProperty("UsePreprocessedDetectors");
          if(!(reuse_preprocecced_detectors&&g_DetLoc.isDefined(m_InWS2D))){
            // amount of work:
            const size_t nHist = m_InWS2D->getNumberHistograms();
            m_Progress.reset(new API::Progress(this,0.0,1.0,nHist));
            g_log.information()<<" preprocessing detectors\n";
            g_DetLoc.processDetectorsPositions(m_InWS2D,g_Log,m_Progress.get());  
            if(g_DetLoc.nDetectors()==0)
            {
                g_log.error()<<" no valid detectors identified associated with spectra, nothing to do\n";
                throw(std::invalid_argument("no valid detectors indentified associated with any spectra"));
            }
          }
    }
    targWSDescr.setDetectors(g_DetLoc);

 // create and initate new workspace
  if(create_new_ws)  
  {
    spws = m_OutWSWrapper->createEmptyMDWS(targWSDescr);
    if(!spws)
    {
        g_log.error()<<"can not create target event workspace with :"<<targWSDescr.nDimensions()<<" dimensions\n";
        throw(std::invalid_argument("can not create target workspace"));
    }
    // Build up the box controller
    Mantid::API::BoxController_sptr bc = m_OutWSWrapper->pWorkspace()->getBoxController();
    // Build up the box controller, using the properties in BoxControllerSettingsAlgorithm
    this->setBoxController(bc);
    // split boxes;
    spws->splitBox();
  // Do we split more due to MinRecursionDepth?
    int minDepth = this->getProperty("MinRecursionDepth");
    int maxDepth = this->getProperty("MaxRecursionDepth");
    if (minDepth>maxDepth) throw std::invalid_argument("MinRecursionDepth must be >= MaxRecursionDepth ");
    spws->setMinRecursionDepth(size_t(minDepth));  
  }else{
      m_OutWSWrapper->setMDWS(spws);
  }

  //DO THE JOB:

  // get pointer to appropriate  algorithm, (will throw if logic is wrong and subalgorithm is not found among existing)
  ConvToMDSelector AlgoSelector;
  m_Convertor  = AlgoSelector.convSelector(m_InWS2D,m_Convertor);

  // initate conversion and estimate amout of job to do
  size_t n_steps = m_Convertor->initialize(targWSDescr,m_OutWSWrapper);
  // progress reporter
  m_Progress.reset(new API::Progress(this,0.0,1.0,n_steps)); 

  g_log.information()<<" conversion started\n";
  m_Convertor->runConversion(m_Progress.get());
  copyMetaData(spws);

  //JOB COMPLETED:
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(spws));

  // free the algorithm from the responsibility for the target workspace to allow it to be deleted if necessary
  m_OutWSWrapper->releaseWorkspace();
  // free up the sp to the input workspace, which would be deleted if nobody needs it any more;
  m_InWS2D.reset();
  return;
}

/**
 * Copy over the metadata from the input matrix workspace
 * @param mdEventWS :: The output MDEventWorkspace
 */
void ConvertToMD::copyMetaData(API::IMDEventWorkspace_sptr mdEventWS) const
{
  const MantidVec & binBoundaries = m_InWS2D->readX(0);
  auto mapping = m_InWS2D->spectraMap().createIDGroupsMap();

  uint16_t nexpts = mdEventWS->getNumExperimentInfo();
  for(uint16_t i = 0; i < nexpts; ++i)
  {
    ExperimentInfo_sptr expt = mdEventWS->getExperimentInfo(i);
    expt->mutableRun().storeHistogramBinBoundaries(binBoundaries);
    expt->cacheDetectorGroupings(*mapping);
  }
}

/** Constructor */
ConvertToMD::ConvertToMD()
{}


} // namespace Mantid
} // namespace MDAlgorithms


