// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/BaseConvertToDiffractionMDWorkspace.h"

#include "MantidAPI/IMDEventWorkspace.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

#include "MantidMDAlgorithms/ConvertToMDMinMaxLocal.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDWSTransform.h"

#include <algorithm>
#include <limits>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDAlgorithms {

/**Small class to diable propery on interface */
class DisabledProperty : public EnabledWhenProperty {
public:
  DisabledProperty() : EnabledWhenProperty("NonExistingProperty", IS_DEFAULT) {}
  bool checkCriterion(const IPropertyManager * /*algo*/) const override {
    return false;
  }
};

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void BaseConvertToDiffractionMDWorkspace::init() {

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output MDEventWorkspace. If the workspace "
                  "already exists, then the events will be added to it.");
  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("Append", false, Direction::Input),
      "Append events to the output workspace. The workspace is replaced if "
      "unchecked.");

  // Disabled for this version
  declareProperty(std::make_unique<PropertyWithValue<bool>>("ClearInputWorkspace",
                                                       false, Direction::Input),
                  "Clearing the events from the input workspace during "
                  "conversion (to save memory) is not supported by algorithm "
                  "v2");
  // disable property on interface
  this->setPropertySettings("ClearInputWorkspace",
                            std::make_unique<DisabledProperty>());

  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("OneEventPerBin", true,
                                           Direction::Input),
      "Use the histogram representation (event for event workspaces).\n"
      "One MDEvent will be created for each histogram bin (even empty ones).\n"
      "Warning! This can use significantly more memory!");

  frameOptions.emplace_back("Q (sample frame)");
  frameOptions.emplace_back("Q (lab frame)");
  frameOptions.emplace_back("HKL");
  declareProperty(
      "OutputDimensions", "Q (lab frame)",
      boost::make_shared<StringListValidator>(frameOptions),
      "What will be the dimensions of the output workspace?\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  Q (sample frame): Wave-vector change of the lattice in the frame of "
      "the sample (taking out goniometer rotation).\n"
      "  HKL: Use the sample's UB matrix to convert to crystal's HKL indices.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("LorentzCorrection",
                                                       false, Direction::Input),
                  "Correct the weights of events by multiplying by the Lorentz "
                  "formula: sin(theta)^2 / lambda^4");

  // Box controller properties. These are the defaults
  this->initBoxControllerProps("2" /*SplitInto*/, 1500 /*SplitThreshold*/,
                               20 /*MaxRecursionDepth*/);

  declareProperty(
      std::make_unique<PropertyWithValue<int>>("MinRecursionDepth", 1),
      "Optional. If specified, then all the boxes will be split to this "
      "minimum recursion depth. 1 = one level of splitting, etc.\n"
      "Be careful using this since it can quickly create a huge number of "
      "boxes = (SplitInto ^ (MinRercursionDepth * NumDimensions)).\n"
      "But setting this property equal to MaxRecursionDepth property is "
      "necessary if one wants to generate multiple file based workspaces in "
      "order to merge them later\n");
  setPropertyGroup("MinRecursionDepth", getBoxSettingsGroupName());
}

/** method to convert the value of the target frame specified for the
 *ConvertToDiffractionMDWorksapce into the properties names of the ConvertToMD
 * @param  TargFrame -- the string, describing target transformation frame in
 *the form accepted by convertToDiffractionWorksapce
 * @param  TargFrameName -- the string describing target transformation frame
 *in the form acepted by convertToMD
 * @param  ScalingName    -- default coordinate scaling name accepted by
 *convertToMD;
 *
 *@return TargFrameName and ScalingName
 */
void BaseConvertToDiffractionMDWorkspace::convertFramePropertyNames(
    const std::string &TargFrame, std::string &TargFrameName,
    std::string &ScalingName) {
  // ----------------- Handle the type of output
  // -------------------------------------

  MDAlgorithms::MDWSTransform QSclAndFrames;

  if (TargFrame == frameOptions[0]) // "Q (sample frame)"
  {
    TargFrameName =
        QSclAndFrames.getTargetFrame(MDAlgorithms::CnvrtToMD::SampleFrame);
    ScalingName = QSclAndFrames.getQScaling(
        MDAlgorithms::CnvrtToMD::NoScaling); //< momentums in A^-1
  } else if (TargFrame == frameOptions[1])   //     "Q (lab frame)"
  {
    TargFrameName =
        QSclAndFrames.getTargetFrame(MDAlgorithms::CnvrtToMD::LabFrame);
    ScalingName = QSclAndFrames.getQScaling(
        MDAlgorithms::CnvrtToMD::NoScaling); //< momentums in A^-1
  } else if (TargFrame == frameOptions[2])   // "HKL"
  {
    TargFrameName =
        QSclAndFrames.getTargetFrame(MDAlgorithms::CnvrtToMD::HKLFrame);
    ScalingName = QSclAndFrames.getQScaling(
        MDAlgorithms::CnvrtToMD::HKLScale); //< momentums in A^-1
  } else {
    throw std::invalid_argument(
        "BaseConvertToDiffractionMDWorkspace::Unknown target frame: " +
        TargFrame);
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.   */
void BaseConvertToDiffractionMDWorkspace::exec() {

  Mantid::API::Algorithm_sptr Convert =
      createChildAlgorithm("ConvertToMD", 0., 1.);
  Convert->initialize();

  Convert->setRethrows(true);
  Convert->initialize();

  Convert->setProperty<MatrixWorkspace_sptr>(
      "InputWorkspace", this->getProperty("InputWorkspace"));
  Convert->setProperty("OutputWorkspace",
                       this->getPropertyValue("OutputWorkspace"));
  Convert->setProperty("OverwriteExisting", !this->getProperty("Append"));

  if (!MDTransfFactory::Instance().exists("Q3D")) {
    throw std::runtime_error(" ConvertToMD Q3D plugin used to transform into "
                             "DiffractionWorkspaced has not been registered "
                             "with the MDTransformation factory");
  }
  Convert->setPropertyValue("QDimensions", "Q3D");

  std::vector<std::string> dE_modes = Kernel::DeltaEMode::availableTypes();
  Convert->setPropertyValue("dEAnalysisMode",
                            dE_modes[Kernel::DeltaEMode::Elastic]);

  std::string TargetFrame, Scaling;
  this->convertFramePropertyNames(this->getPropertyValue("OutputDimensions"),
                                  TargetFrame, Scaling);
  Convert->setProperty("Q3DFrames", TargetFrame);
  Convert->setProperty("QConversionScales", Scaling);

  Convert->setProperty("OtherDimensions", "");
  Convert->setProperty("PreprocDetectorsWS", "-");

  bool lorCorr = this->getProperty("LorentzCorrection");
  Convert->setProperty("LorentzCorrection", lorCorr);

  bool ignoreZeros = !this->getProperty("OneEventPerBin");
  Convert->setProperty("IgnoreZeroSignals", ignoreZeros);
  // set extents
  std::vector<double> extents = this->getProperty("Extents");
  std::vector<double> minVal, maxVal;
  convertExtents(extents, minVal, maxVal);
  Convert->setProperty("MinValues", minVal);
  Convert->setProperty("MaxValues", maxVal);

  // Box controller properties. Has defaults
  Convert->setProperty("SplitInto", this->getPropertyValue("SplitInto"));
  Convert->setProperty("SplitThreshold",
                       this->getPropertyValue("SplitThreshold"));
  Convert->setProperty("MaxRecursionDepth",
                       this->getPropertyValue("MaxRecursionDepth"));
  std::string depth = this->getPropertyValue("MinRecursionDepth");
  if (depth == "0")
    depth = "1"; // ConvertToMD does not understand 0 depth
  Convert->setProperty("MinRecursionDepth", depth);

  Convert->executeAsChildAlg();

  IMDEventWorkspace_sptr iOut = Convert->getProperty("OutputWorkspace");
  this->setProperty("OutputWorkspace", iOut);
}

} // namespace MDAlgorithms
} // namespace Mantid
