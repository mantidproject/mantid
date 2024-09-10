// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/SofTwoThetaTOF.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Strings.h"

#include <filesystem>

using Mantid::Kernel::Strings::randomString;

namespace {
/// Constants for the algorithm's property names.
namespace Prop {
std::string const ANGLE_STEP{"AngleStep"};
std::string const FILENAME{"GroupingFilename"};
std::string const INPUT_WS{"InputWorkspace"};
std::string const OUTPUT_WS{"OutputWorkspace"};
} // namespace Prop

/// A temporary file resource lifetime manager
struct RemoveFileAtScopeExit {
  std::string name;
  ~RemoveFileAtScopeExit() {
    if (!name.empty()) {
      auto const path = std::filesystem::path(name);
      if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
      }
    }
  }
};

/// A holder of minimum and maximum values.
struct MinMax {
  double min{std::numeric_limits<double>::max()};
  double max{std::numeric_limits<double>::lowest()};
};

/**
 * Return the minimum and maximum X over all histograms
 * @param ws a workspace
 * @return the minimum and maximum X
 */
MinMax minMaxX(Mantid::API::MatrixWorkspace const &ws) {
  auto const nHisto = ws.getNumberHistograms();
  MinMax mm;
  for (size_t i = 0; i < nHisto; ++i) {
    auto const &x = ws.x(i);
    mm.min = std::min(mm.min, x.front());
    mm.max = std::max(mm.max, x.back());
  }
  return mm;
}

/**
 * Return the width of the first bin in the first histogram.
 * @param ws a workspace
 * @return the bin width
 */
double binWidth(Mantid::API::MatrixWorkspace const &ws) {
  auto const &x = ws.x(0);
  return x[1] - x[0];
}

/**
 * Append .xml to filename if needed.
 * @param filename a filename
 * @return a suitable name for an XML file
 */
std::string ensureXMLExtension(std::string const &filename) {
  std::string name = filename;
  auto const path = std::filesystem::path(filename);
  if (path.extension() != ".xml") {
    name += ".xml";
  }
  return name;
}
} // namespace

namespace Mantid::WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofTwoThetaTOF)

/// Algorithms name for identification. @see Algorithm::name
const std::string SofTwoThetaTOF::name() const { return "SofTwoThetaTOF"; }

/// Algorithm's version for identification. @see Algorithm::version
int SofTwoThetaTOF::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SofTwoThetaTOF::category() const { return "Inelastic;ILL\\Direct"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SofTwoThetaTOF::summary() const {
  return "Calculates the intensity as a function of scattering angle and time "
         "of flight.";
}

/** Initialize the algorithm's properties.
 */
void SofTwoThetaTOF::init() {
  auto histogrammedTOF = std::make_shared<Kernel::CompositeValidator>();
  histogrammedTOF->add(std::make_shared<API::WorkspaceUnitValidator>("TOF"));
  histogrammedTOF->add(std::make_shared<API::HistogramValidator>());
  histogrammedTOF->add(std::make_shared<API::InstrumentValidator>());
  declareProperty(
      std::make_unique<API::WorkspaceProperty<>>(Prop::INPUT_WS, "", Kernel::Direction::Input, histogrammedTOF),
      "A workspace to be converted.");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(Prop::OUTPUT_WS, "", Kernel::Direction::Output),
                  "A workspace with (2theta, TOF) units.");
  auto positiveDouble = std::make_shared<Kernel::BoundedValidator<double>>();
  positiveDouble->setLowerExclusive(0.);
  declareProperty(Prop::ANGLE_STEP, EMPTY_DBL(), positiveDouble, "The angle step for detector grouping, in degrees.");
  declareProperty(std::make_unique<API::FileProperty>(Prop::FILENAME, "", API::FileProperty::OptionalSave,
                                                      std::vector<std::string>{".xml"}),
                  "A grouping file that will be created; a corresponding .par "
                  "file wille be created as well.");
}

/** Execute the algorithm.
 */
void SofTwoThetaTOF::exec() {
  API::MatrixWorkspace_sptr in = getProperty(Prop::INPUT_WS);
  auto ragged = convertToConstantL2(in);
  auto equalBinning = rebinToNonRagged(ragged);
  auto ws = maskEmptyBins(equalBinning, ragged);
  ws = groupByTwoTheta(ws, clarifyAngleStep(*in));
  ws = convertToTwoTheta(ws);
  setProperty(Prop::OUTPUT_WS, ws);
}

double SofTwoThetaTOF::clarifyAngleStep(API::MatrixWorkspace const &ws) {
  if (!isDefault(Prop::ANGLE_STEP)) {
    return getProperty(Prop::ANGLE_STEP);
  } else {
    auto instrument = ws.getInstrument();
    if (!instrument) {
      throw std::invalid_argument("Missing " + Prop::ANGLE_STEP);
    }
    auto const &paramMap = ws.constInstrumentParameters();
    auto const paramValues = paramMap.getDouble(instrument->getName(), "natural-angle-step");
    if (paramValues.empty()) {
      throw std::invalid_argument("Missing " + Prop::ANGLE_STEP +
                                  " or 'natural-angle-step' not defined in "
                                  "instrument parameters file.");
    }
    return paramValues.front();
  }
}

API::MatrixWorkspace_sptr SofTwoThetaTOF::convertToConstantL2(API::MatrixWorkspace_sptr &ws) {
  auto toConstantL2 = createChildAlgorithm("ConvertToConstantL2", 0., 0.2);
  toConstantL2->setProperty("InputWorkspace", ws);
  toConstantL2->setPropertyValue("OutputWorkspace", "out");
  toConstantL2->execute();
  return toConstantL2->getProperty("OutputWorkspace");
}

API::MatrixWorkspace_sptr SofTwoThetaTOF::convertToTwoTheta(API::MatrixWorkspace_sptr &ws) {
  auto convertAxis = createChildAlgorithm("ConvertSpectrumAxis", 0.9, 1.0);
  convertAxis->setProperty("InputWorkspace", ws);
  convertAxis->setProperty("OutputWorkspace", "out");
  convertAxis->setProperty("Target", "Theta");
  convertAxis->execute();
  return convertAxis->getProperty("OutputWorkspace");
}

API::MatrixWorkspace_sptr SofTwoThetaTOF::groupByTwoTheta(API::MatrixWorkspace_sptr &ws, double const twoThetaStep) {
  const std::string GROUP_WS("softwothetatof_groupWS");
  auto generateGrouping = createChildAlgorithm("GenerateGroupingPowder", 0.2, 0.5);
  generateGrouping->setProperty("InputWorkspace", ws);
  generateGrouping->setProperty("AngleStep", twoThetaStep);
  generateGrouping->setProperty("GroupingWorkspace", GROUP_WS);
  std::string filename;
  RemoveFileAtScopeExit deleteThisLater;
  if (isDefault(Prop::FILENAME)) {
    const std::string shortname = "detector-grouping-" + randomString(4) + "-" + randomString(4) + "-" +
                                  randomString(4) + "-" + randomString(4) + ".xml";
    const auto tempPath = std::filesystem::temp_directory_path() / shortname;
    filename = tempPath.string();
    generateGrouping->setProperty("GenerateParFile", false);
    // Make sure the file gets deleted at scope exit.
    deleteThisLater.name = filename;
  } else {
    filename = static_cast<std::string>(getProperty(Prop::FILENAME));
    filename = ensureXMLExtension(filename);
  }
  generateGrouping->setProperty("GroupingFilename", filename);
  generateGrouping->execute();

  auto groupDetectors = createChildAlgorithm("GroupDetectors", 0.7, 0.9);
  groupDetectors->setProperty("InputWorkspace", ws);
  groupDetectors->setProperty("OutputWorkspace", "out");
  // TODO make this algo work with the GroupingWorkspace from GenerateGroupingPowder
  // Mantid::DataObjects::GroupingWorkspace_sptr gws = generateGrouping->getProperty("GroupingWorkspace");
  // groupDetectors->setProperty("CopyGroupingFromWorkspace", gws);
  groupDetectors->setProperty("MapFile", filename);
  groupDetectors->setProperty("Behaviour", "Average");
  groupDetectors->execute();
  return groupDetectors->getProperty("OutputWorkspace");
}

API::MatrixWorkspace_sptr SofTwoThetaTOF::maskEmptyBins(API::MatrixWorkspace_sptr &maskable,
                                                        API::MatrixWorkspace_sptr &comparison) {
  auto maskNonOverlapping = createChildAlgorithm("MaskNonOverlappingBins", 0.6, 0.7);
  maskNonOverlapping->setProperty("InputWorkspace", maskable);
  maskNonOverlapping->setProperty("OutputWorkspace", "out");
  maskNonOverlapping->setProperty("ComparisonWorkspace", comparison);
  maskNonOverlapping->setProperty("MaskPartiallyOverlapping", true);
  maskNonOverlapping->setProperty("RaggedInputs", "Ragged");
  maskNonOverlapping->setProperty("CheckSortedX", false);
  maskNonOverlapping->execute();
  return maskNonOverlapping->getProperty("OutputWorkspace");
}

API::MatrixWorkspace_sptr SofTwoThetaTOF::rebinToNonRagged(API::MatrixWorkspace_sptr &ws) {
  auto const xRange = minMaxX(*ws);
  std::vector<double> const rebinParams{xRange.min, binWidth(*ws), xRange.max};
  auto rebin = createChildAlgorithm("Rebin", 0.5, 0.6);
  rebin->setProperty("InputWorkspace", ws);
  rebin->setProperty("OutputWorkspace", "out");
  rebin->setProperty("Params", rebinParams);
  rebin->setProperty("FullBinsOnly", true);
  rebin->execute();
  return rebin->getProperty("OutputWorkspace");
}

} // namespace Mantid::WorkflowAlgorithms
