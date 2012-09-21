/*WIKI* 
== Summary ==

Transforms a workspace into MDEvent workspace with dimensions defined by user. 
   
Gateway for set of subalgorithms, combined together to convert input 2D matrix workspace or Event workspace with any units along X-axis into  multidimensional event workspace. 

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
#include "MantidDataObjects/TableWorkspace.h" 
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
        "An input Matrix Workspace (2DMatrix or Event workspace) ");
   
     declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
                  "Name of the output MDEventWorkspace");

     declareProperty(new PropertyWithValue<bool>("OverwriteExisting", true, Direction::Input),
              "By default (""1""), existing Output Workspace will be replaced. Select false (""0"") if you want to add new events to the workspace, which already exist.\n"
              "Choosing ""0"" can be very inefficient for file-based workspaces");

     std::vector<std::string> Q_modes = MDEvents::MDTransfFactory::Instance().getKeys();
     // something to do with different moments of thime when algorithm or test loads library. To avoid empty factory always do this. 
     if(Q_modes.empty()) Q_modes.assign(1,"ERROR IN LOADING Q-converters");
  
     /// this variable describes default possible ID-s for Q-dimensions   
     declareProperty("QDimensions",Q_modes[0],boost::make_shared<StringListValidator>(Q_modes),
          "String, describing available analysis modes, registered with [[MD Transformation factory]].\n"
          "The modes names are ""CopyToMD"", ""mod|Q|"" and ""Q3D""",Direction::InOut);
     /// temporary, untill dEMode is not properly defined on Workspace
     MDEvents::MDTransfDEHelper AlldEModes;
     std::vector<std::string> dE_modes = AlldEModes.getEmodes();
     declareProperty("dEAnalysisMode",dE_modes[CnvrtToMD::Direct],boost::make_shared<StringListValidator>(dE_modes),
       "You can analyse neutron energy transfer in ""Direct"", ""Indirect"" or ""Elastic"" mode. \n"
       " The analysis mode has to correspond to experimental set up. Selecting inelastic mode increases the number of the target workspace dimensions by one.\n"
       " See [[MD Transformation factory]] for further details.",Direction::InOut);

     MDEvents::MDWSTransform QScl;
     std::vector<std::string> QScales = QScl.getQScalings();
     declareProperty("QConversionScales",QScales[CnvrtToMD::NoScaling], boost::make_shared<StringListValidator>(QScales),
       "This property to normalize three momentums obtained in ""Q3D"" mode.\n"
       " See [[MD Transformation factory]] for description and available scaling modes.");
     
    declareProperty(new ArrayProperty<std::string>("OtherDimensions",Direction::Input),
        " List(comma separated) of additional to Q and DeltaE variables which form additional (orthogonal) to Q dimensions"
        " in the target workspace (e.g. Temperature or Magnetic field).\n"
        " These variables had to be logged during experiment and the names of these variables "
        " have to coincide with the log names for the records of these variables in the source workspace");

    // this property is mainly for subalgorithms to set-up as they have to identify if they use the same instrument. 
    declareProperty(new PropertyWithValue<std::string>("PreprocDetectorsWS","PreprocessedDetectorsWS",Direction::Input), 
      "The name of the table workspace where the part of the detectors transformation into reciprocal space, calculated by [[PreprocessDetectorsToMD]] algorithm stored.\n"
      "If the workspace is not found in analysis data service, [[PreprocessDetectorsToMD]] used to calculate it. If found, the algorithm will use the workspace from DS\n"
      "Useful if one expects to analyse number of different experiments obtained on the same instrument.\n"
      "<span style=""color:#FF0000""> Dangerous if one uses number of workspaces with modified derived instrument one after another. </span>\n"
      "In this case this property has to be set empty and the workspace will be recalculated inernaly each time the algorithm is invoked"); 
    // if one needs to use Lorentz corrections
    declareProperty(new PropertyWithValue<bool>("LorentzCorrection", false, Direction::Input), 
        "Correct the weights of events or signals and errors transformed into reciprocal space by multiplying them by the Lorentz multiplier: sin(theta)^2/lambda^4.\n"
        "Currently works in Q3D Elastic case only.");     

    declareProperty(new ArrayProperty<double>("MinValues"),
        "It has to be N comma separated values, where N is the number of dimensions of the target workspace.\n"
        "Values smaller then specified here will not be added to workspace.\n"
        "Number N is defined by properties 4,6 and 7 and described on [[MD Transformation factory]] page.\n");
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
  // initiate class which would deal with any dimension workspaces requested by algorithm parameters
  if(!m_OutWSWrapper) m_OutWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());

  // -------- get Input workspace
   m_InWS2D = getProperty("InputWorkspace");
   
   // get the output workspace
   API::IMDEventWorkspace_sptr spws = getProperty("OutputWorkspace");
  
  // Collect and Analyze the requests to the job, specified by the input parameters:
    //a) Q selector:
    std::string QModReq                    = getProperty("QDimensions");
    //b) the energy exchange mode
    std::string dEModReq                   = getProperty("dEAnalysisMode");
    //c) other dim property;
    std::vector<std::string> otherDimNames = getProperty("OtherDimensions");
    //d) part of the procedure, specifying the target dimensions units. Currently only Q3D target units can be converted to different flavours of hkl
    std::string convertTo_                 = getProperty("QConversionScales");

    // Build the target ws description as function of the input & output ws and the parameters, supplied to the algorithm 
    MDEvents::MDWSDescription targWSDescr;
    // get workspace parameters and build target workspace descritpion, report if there is need to build new target MD workspace
    bool createNewTargetWs = buildTargetWSDescription(spws,QModReq,dEModReq,otherDimNames,convertTo_,targWSDescr);

     // create and initate new workspace or set up existing workspace as a target. 
    if(createNewTargetWs)  // create new
       spws = this->createNewMDWorkspace(targWSDescr);
    else // setup existing MD workspace as workspace target.
       m_OutWSWrapper->setMDWS(spws);
 
    // preprocess detectors;
    targWSDescr.m_PreprDetTable = this->preprocessDetectorsPositions(m_InWS2D);

 
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
 * Copy over the metadata from the input matrix workspace to output MDEventWorkspace
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
/** handle the input parameters and build target workspace description as function of input parameters */ 
bool ConvertToMD::buildTargetWSDescription(API::IMDEventWorkspace_sptr spws,const std::string &QModReq,const std::string &dEModReq,const std::vector<std::string> &otherDimNames,
                                           const std::string &convertTo_,MDEvents::MDWSDescription &targWSDescr)
{
  // ------- Is there need to creeate new ouptutworpaced?  
    bool createNewTargetWs =doWeNeedNewTargetWorkspace(spws );
 

   // set the min and max values for the dimensions from the input porperties
    std::vector<double> dimMin = getProperty("MinValues");
    std::vector<double> dimMax = getProperty("MaxValues");
    // verify that the number min/max values is equivalent to the number of dimensions defined by properties and min is less max
    targWSDescr.setMinMax(dimMin,dimMax);   
    targWSDescr.buildFromMatrixWS(m_InWS2D,QModReq,dEModReq,otherDimNames);

    bool LorentzCorrections = getProperty("LorentzCorrection");
    targWSDescr.setLorentsCorr(LorentzCorrections);

  // instanciate class, responsible for defining Mslice-type projection
    MDEvents::MDWSTransform MsliceProj;
    if(createNewTargetWs)
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
         targWSDescr.m_RotMatrix = MsliceProj.getTransfMatrix(targWSDescr,convertTo_);           
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
        targWSDescr.m_RotMatrix = MsliceProj.getTransfMatrix(targWSDescr,convertTo_);
    
    }
    return createNewTargetWs;
}


/**Create new MD workspace and set up its box controller using algorithm's box controllers properties 
* @param NewMDWSDescription -- the constructed MD workspace description;
*/
API::IMDEventWorkspace_sptr ConvertToMD::createNewMDWorkspace(const MDEvents::MDWSDescription &targWSDescr)
{
   // create new md workspace and set internal shared pointer of m_OutWSWrapper to this workspace
    API::IMDEventWorkspace_sptr spws = m_OutWSWrapper->createEmptyMDWS(targWSDescr);
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

    return spws;

}

/**Check if the target workspace new or exists and we need to create new workspace
 *@param spws -- shared pointer to target MD workspace, which can be undefined if the workspace does not exist
 *@param      -- Algorithnm property "OverwriteExisting" which specifies if one needs to owerwrite existing workspace
 *
 *@returns true if one needs to create new workspace and false otherwise
*/
bool ConvertToMD::doWeNeedNewTargetWorkspace(API::IMDEventWorkspace_sptr spws)
{

  bool createNewWs(false);
  if(!spws)
  {
    createNewWs = true;
  }
  else
  { 
      bool shouldOverwrite = getProperty("OverwriteExisting");
      if (shouldOverwrite )
      {
          createNewWs=true;
      }else{
          createNewWs=false;
      }
  }
  return createNewWs;
}
/**The method responsible for preprocessing detectors positions into reciprocal space  
  *@param InWS2D -- input Matrix workspace with defined instrument 
  *
  *@returns TableWorkspace_const_sptr the pointer to the table workspace which contains positions of the preprocessed detectorsl
  *         Depenting on the algorithm parameters, this worksapce is also stored in the analysis data service. 
 */
DataObjects::TableWorkspace_const_sptr ConvertToMD::preprocessDetectorsPositions( Mantid::API::MatrixWorkspace_const_sptr InWS2D)
{

    DataObjects::TableWorkspace_sptr TargTableWS;
    bool storeInDataService(true);
    std::string OutWSName = std::string(getProperty("PreprocDetectorsWS"));
    if(OutWSName.empty()) // TargTableWS is recalculated each time;
    {
      storeInDataService = false;
      OutWSName = "ServiceTableWS";  // TODO: should be hidden?
    }
    else
    {
      storeInDataService = true;
      if(API::AnalysisDataService::Instance().doesExist(OutWSName))      // that is it, workspace exists and we may try to use it
      {
        TargTableWS = API::AnalysisDataService::Instance().retrieveWS<DataObjects::TableWorkspace>(OutWSName);
        // get number of all histohrams (may be masked or invalid)
        size_t nHist = InWS2D->getNumberHistograms();
        size_t nDetMap=TargTableWS->rowCount();
        if(nHist==nDetMap)
        {
          // let's take at least some precaution to ensure that instrument have not changed
          std::string currentWSInstrumentName = InWS2D->getInstrument()->getName();
          std::string oldInstrName            = std::string(TargTableWS->getProperty("InstrumentName"));

          if(oldInstrName==currentWSInstrumentName) return TargTableWS;
        }
      }

    }
    // if input workspace does not exist in analysis data service, we have to add it there to work with algorithm /sucs...
    std::string InWSName = InWS2D->getName();
    if(!API::AnalysisDataService::Instance().doesExist(InWSName))
    {
       if(InWSName.empty())InWSName = "ImputMatrixWS";
       // wery bad, but what can we do otherwise... -> pool out the class pointer which is not const 
       API::AnalysisDataService::Instance().addOrReplace(InWSName,m_InWS2D);
    }

    Mantid::API::Algorithm_sptr childAlg = createSubAlgorithm("PreprocessDetectorsToMD",0.,1.);
    if(!childAlg)throw(std::runtime_error("Can not create child subalgorithm to preprocess detectors"));
    childAlg->setProperty("InputWorkspace",InWSName);
    childAlg->setProperty("OutputWorkspace",OutWSName);

    childAlg->execute();
    if(!childAlg->isExecuted())throw(std::runtime_error("Can not properly execute child subalgorithm to preprocess detectors"));

    TargTableWS = childAlg->getProperty("OutputWorkspace");
    if(!TargTableWS)throw(std::runtime_error("Can not retrieve results of child subalgorithm to preprocess detectors work"));

    if(storeInDataService)
      API::AnalysisDataService::Instance().addOrReplace(OutWSName,TargTableWS);
    else
      TargTableWS->setName(OutWSName);

    return TargTableWS;

}


} // namespace Mantid
} // namespace MDAlgorithms


