
#include "MantidMDAlgorithms/ConvertToMDParent.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
//
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidMDEvents/MDWSTransform.h"
//

#include "MantidMDEvents/ConvToMDSelector.h"


using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace Mantid::MDEvents::CnvrtToMD;

namespace Mantid
{
namespace MDAlgorithms
{

// logger for the algorithm workspaces  
Kernel::Logger& ConvertToMDParent::g_Log =Kernel::Logger::get("MD-Algorithms");
//
Mantid::Kernel::Logger & ConvertToMDParent::getLogger(){return g_Log;}
//
/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertToMDParent::category() const { return "MDAlgorithms";}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToMDParent::init()
{
      auto ws_valid = boost::make_shared<CompositeValidator>();
      //
      ws_valid->add<InstrumentValidator>();
      // the validator which checks if the workspace has axis and any units
      ws_valid->add<WorkspaceUnitValidator>("");


    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,ws_valid),
        "An input Matrix Workspace (2DMatrix or Event workspace) ");
   
     declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
                  "Name of the output [[MDEventWorkspace]].");

     declareProperty(new PropertyWithValue<bool>("OverwriteExisting", true, Direction::Input),
              "By default  (''\"1\"''), existing Output Workspace will be replaced. Select false (''\"0\"'') if you want to add new events to the workspace, which already exist. "
              "\nChoosing ''\"0\"''' can be very inefficient for file-based workspaces");

     std::vector<std::string> Q_modes = MDEvents::MDTransfFactory::Instance().getKeys();
     // something to do with different moments of thime when algorithm or test loads library. To avoid empty factory always do this. 
     if(Q_modes.empty()) Q_modes.assign(1,"ERROR IN LOADING Q-converters");
  
     /// this variable describes default possible ID-s for Q-dimensions   
     declareProperty("QDimensions",Q_modes[0],boost::make_shared<StringListValidator>(Q_modes),
"String, describing available analysis modes, registered with [[MD Transformation factory]].\n "
"There are 3 modes currently available and described in details on [[MD Transformation factory]] page. \n"
"The modes names are '''CopyToMD''', '''mod<nowiki>|Q|</nowiki>''' and '''Q3D'''",
                     Direction::InOut);
     /// temporary, untill dEMode is not properly defined on Workspace
     std::vector<std::string> dE_modes = Kernel::DeltaEMode().availableTypes();
     declareProperty("dEAnalysisMode",dE_modes[Kernel::DeltaEMode::Direct],boost::make_shared<StringListValidator>(dE_modes),
"You can analyse neutron energy transfer in '''Direct''', '''Indirect''' or '''Elastic''' mode. \n"
"The analysis mode has to correspond to experimental set up. Selecting inelastic mode increases \n"
"the number of the target workspace dimensions by one. See [[MD Transformation factory]] for further details.",
                     Direction::InOut);

    MDEvents::MDWSTransform QSclAndFrames;
    std::vector<std::string> TargFrames = QSclAndFrames.getTargetFrames();
    declareProperty("Q3DFrames", TargFrames[CnvrtToMD::AutoSelect],boost::make_shared<StringListValidator>(TargFrames),
      "What will be the Q-dimensions of the output workspace in Q3D case?\n"
      "   AutoSelect: Choose the target coordinate frame as the function of goniometer and UB matrix values set on the input workspace\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  Q (sample frame): Wave-vector change of the lattice in the frame of the sample (taking out goniometer rotation).\n"
      "  HKL: Use the sample's UB matrix to convert to crystal's HKL indices.\n"
      "See [[ MD_Transformation_factory#Q3D |MD Transformation factory]] for more details about this. "
       );


     std::vector<std::string> QScales = QSclAndFrames.getQScalings();
     declareProperty("QConversionScales",QScales[CnvrtToMD::NoScaling], boost::make_shared<StringListValidator>(QScales),
"This property to normalize three momentums obtained in '''Q3D''' mode. See [[MD Transformation factory]] "
"for description and available scaling modes.\n The value can be modified depending on the target coordinate "
"system, defined by the property '''OutputDimensions'''. "
   );


     setPropertySettings("Q3DFrames",new Kernel::VisibleWhenProperty("QDimensions",IS_EQUAL_TO,"Q3D"));
     setPropertySettings("QconversionScales",new Kernel::VisibleWhenProperty("QDimensions",IS_EQUAL_TO,"Q3D"));


     
    declareProperty(new ArrayProperty<std::string>("OtherDimensions",Direction::Input),
"List(comma separated) of additional to '''Q''' and '''DeltaE''' variables which form additional "
"(orthogonal) to '''Q''' dimensions in the target workspace (e.g. Temperature or Magnetic field).\n "
"These variables had to be logged during experiment and the names of these variables have to coincide "
"with the log names for the records of these variables in the source workspace."
                    );

    // this property is mainly for ChildAlgorithms to set-up as they have to identify if they use the same instrument. 
    declareProperty(new PropertyWithValue<std::string>("PreprocDetectorsWS","PreprocessedDetectorsWS",Direction::Input), 
"The name of the table workspace where the part of the detectors transformation into reciprocal space, "
"calculated by [[PreprocessDetectorsToMD]] algorithm stored. If the workspace is not found in analysis "
"data service, [[PreprocessDetectorsToMD]] used to calculate it. If found, the algorithm uses existing "
"workspace. The field is useful if one expects to analyse number of different experiments obtained on "
"the same instrument. <span style=\"color:#FF0000\"> Dangerous if one uses number of workspaces with "
"modified derived instrument one after another. </span> '''In this case this property has to be set to "
"<span style=\"color:#FF0000\">\"-\"</span> sting (without quotes) or empty (possible from script only) "
"to force the workspace recalculation each time the algorithm is invoked'''"
                    );

    declareProperty(new PropertyWithValue<bool>("UpdateMasks", false, Direction::Input),
"if PreprocessDetectorWS is used to build the workspace with preprocessed detectors at first algorithm "
"call and the input workspaces instruments are different by just different masked detectors, setting this "
"option to true forces [[PreprocessDetectorsToMD]] update only the detectors masks for all subsequent "
"calls to this algorithm. <span style=\"color:#FF0000\">This is temporary solution necessary until Mantid "
"masks spectra by 0 rather then by NaN</span> "
                    );

    // if one needs to use Lorentz corrections
    declareProperty(new PropertyWithValue<bool>("LorentzCorrection", false, Direction::Input),
"Correct the weights of events or signals and errors transformed into reciprocal space by multiplying them "
"by the Lorentz multiplier:\n <math>sin(\\theta)^2/\\lambda^4</math>. Currently works in Q3D Elastic case only "
"and is ignored in any other case."
                    );
    declareProperty(new PropertyWithValue<bool>("IgnoreZeroSignals", false, Direction::Input),
 "Enabling this property forces the algorithm to ignore bins with zero signal for an input matrix workspace. Input event workspaces are not affected. "
 "This violates the data normalization but may substantially accelerate calculations in situations when the normalization is not important (e.g. peak finding)."
      );
    declareProperty(new ArrayProperty<double>("MinValues"),
"It has to be N comma separated values, where N is the number of dimensions of the target workspace. Values "
"smaller then specified here will not be added to workspace.\n Number N is defined by properties 4,6 and 7 and "
"described on [[MD Transformation factory]] page. See also [[ConvertToMDHelper]]");

//TODO:    " If a minimal target workspace range is higher then the one specified here, the target workspace range will be used instead " );

   declareProperty(new ArrayProperty<double>("MaxValues"),
"A list of the same size and the same units as MinValues list. Values higher or equal to the specified by "
"this list will be ignored");
//TODO:    "If a maximal target workspace range is lower, then one of specified here, the target workspace range will be used instead" );
    
    declareProperty(new ArrayProperty<double>("Uproj"),
//"The functionality of this parameter set to non-default value is still not fully verified (see ticket #5982). "
"Defines the first projection vector of the target Q coordinate system in '''Q3D''' mode - Default (1,0,0)");

    declareProperty(new ArrayProperty<double>("Vproj"),
//"The functionality of this parameter set to non-default value is still not fully verified (see ticket #5982). "
"Defines the second projection vector of the target Q coordinate system in '''Q3D''' mode - Default (0,1,0).");

    declareProperty(new ArrayProperty<double>("Wproj"),
//"The functionality of this parameter set to non-default value is still not fully verified (see ticket #5982). "
"Defines the third projection vector of the target Q coordinate system in '''Q3D''' mode. - Default (0,0,1)");

   // Box controller properties. These are the defaults
    this->initBoxControllerProps("5" /*SplitInto*/, 1000 /*SplitThreshold*/, 20 /*MaxRecursionDepth*/);
    // additional box controller settings property. 
    auto mustBeMoreThen1 = boost::make_shared<BoundedValidator<int> >();
    mustBeMoreThen1->setLower(1);

    declareProperty(
      new PropertyWithValue<int>("MinRecursionDepth", 1,mustBeMoreThen1),
"Optional. If specified, then all the boxes will be split to this minimum recursion depth. 0 = no splitting, "
"1 = one level of splitting, etc. \n Be careful using this since it can quickly create a huge number of boxes = "
"(SplitInto ^ (MinRercursionDepth * NumDimensions)). \n But setting this property equal to MaxRecursionDepth "
"property is necessary if one wants to generate multiple file based workspaces in order to merge them later.");
    setPropertyGroup("MinRecursionDepth", getBoxSettingsGroupName());
 
}

} // namespace Mantid
} // namespace MDAlgorithms


