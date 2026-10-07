// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvertHFIRSCDtoMDE.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitLabelTypes.h"

#include "Eigen/Dense"
#include "boost/math/constants/constants.hpp"

namespace Mantid::MDAlgorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertHFIRSCDtoMDE)

namespace {
const std::string WAVELENGTH_LOG("wavelength");

/// Identify a group member in messages and output names: its name, or its 1-based position if it has none
std::string memberName(const WorkspaceGroup &group, const size_t index) {
  const std::string name = group.getItem(index)->getName();
  return name.empty() ? std::to_string(index + 1) : name;
}

std::string join(const std::vector<std::string> &messages) {
  std::string joined;
  for (const auto &message : messages)
    joined += (joined.empty() ? "" : "; ") + message;
  return joined;
}
} // namespace

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConvertHFIRSCDtoMDE::name() const { return "ConvertHFIRSCDtoMDE"; }

/// Algorithm's version for identification. @see Algorithm::version
int ConvertHFIRSCDtoMDE::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertHFIRSCDtoMDE::category() const { return "MDAlgorithms\\Creation"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvertHFIRSCDtoMDE::summary() const {
  return "Convert from the detector vs scan index MDHistoWorkspace into a "
         "MDEventWorkspace with units in Q_sample.";
}

/** Check that a workspace can be converted.
 * @param inputWS :: detector-space workspace to check
 * @return description of the problems found, or an empty string if there are none
 */
std::string ConvertHFIRSCDtoMDE::validateInputWorkspace(const API::IMDHistoWorkspace_sptr &inputWS) const {
  std::stringstream inputWSmsg;
  if (inputWS->getNumDims() != 3) {
    inputWSmsg << "Incorrect number of dimensions";
  } else if (inputWS->getDimension(0)->getName() != "y" || inputWS->getDimension(1)->getName() != "x" ||
             inputWS->getDimension(2)->getName() != "scanIndex") {
    inputWSmsg << "Wrong dimensions";
  } else if (inputWS->getNumExperimentInfo() == 0) {
    inputWSmsg << "Missing experiment info";
  } else if (inputWS->getExperimentInfo(0)->getInstrument()->getName() != "HB3A" &&
             inputWS->getExperimentInfo(0)->getInstrument()->getName() != "WAND") {
    inputWSmsg << "This only works for DEMAND (HB3A) or WAND (HB2C)";
  } else if (inputWS->getDimension(2)->getNBins() != inputWS->getExperimentInfo(0)->run().getNumGoniometers()) {
    inputWSmsg << "goniometers not set correctly, did you run SetGoniometer "
                  "with Average=False";
  } else {
    std::string instrument = inputWS->getExperimentInfo(0)->getInstrument()->getName();
    const auto run = inputWS->getExperimentInfo(0)->run();
    size_t number_of_runs = inputWS->getDimension(2)->getNBins();
    std::vector<std::string> logs;
    if (instrument == "HB3A")
      logs = {"monitor", "time"};
    else
      logs = {"duration", "monitor_count"};
    for (auto log : logs) {
      if (run.hasProperty(log)) {
        if (static_cast<size_t>(run.getLogData(log)->size()) != number_of_runs)
          inputWSmsg << "Log " << log << " has incorrect length, ";
      } else {
        inputWSmsg << "Missing required log " << log << ", ";
      }
    }
  }
  return inputWSmsg.str();
}

std::map<std::string, std::string> ConvertHFIRSCDtoMDE::validateInputs() {
  std::map<std::string, std::string> result;

  const API::Workspace_sptr input = this->getProperty("InputWorkspace");
  const auto group = std::dynamic_pointer_cast<WorkspaceGroup>(input);
  const auto inputWorkspaces = inputWorkspaceList(input);
  const std::vector<double> wavelengths = this->getProperty("Wavelength");

  std::vector<std::string> inputMessages;
  std::vector<std::string> wavelengthMessages;
  if (group && inputWorkspaces.empty())
    inputMessages.emplace_back("The input group is empty");
  if (!group && wavelengths.size() > 1)
    wavelengthMessages.emplace_back("Only one value can be given when InputWorkspace is a single workspace");
  if (group && wavelengths.size() > 1 && wavelengths.size() != inputWorkspaces.size())
    wavelengthMessages.emplace_back("Give one value, or one value per group member: found " +
                                    std::to_string(wavelengths.size()) + " values for " +
                                    std::to_string(inputWorkspaces.size()) + " members");
  // Resolve the wavelength of each workspace only when the number of fallback values is valid
  const bool checkWavelengths = wavelengthMessages.empty();

  std::string instrument;      // instrument of the first valid member
  std::string instrumentOwner; // name of that member
  for (size_t i = 0; i < inputWorkspaces.size(); ++i) {
    const auto &inputWS = inputWorkspaces[i];
    const std::string prefix = group ? "Member '" + memberName(*group, i) + "': " : "";
    if (!inputWS) {
      inputMessages.emplace_back(
          prefix + (group ? "must be an MDHistoWorkspace" : "must be an MDHistoWorkspace or a WorkspaceGroup of them"));
      continue;
    }
    const std::string inputWSmsg = validateInputWorkspace(inputWS);
    if (!inputWSmsg.empty()) {
      inputMessages.emplace_back(prefix + inputWSmsg);
      continue;
    }
    const std::string memberInstrument = inputWS->getExperimentInfo(0)->getInstrument()->getName();
    if (instrument.empty()) {
      instrument = memberInstrument;
      instrumentOwner = group ? memberName(*group, i) : "";
    } else if (memberInstrument != instrument) {
      inputMessages.emplace_back(prefix + "instrument " + memberInstrument + " differs from instrument " + instrument +
                                 " of member '" + instrumentOwner + "'");
    }
    if (checkWavelengths) {
      try {
        resolveWavelength(*inputWS, fallbackWavelength(wavelengths, i));
      } catch (const std::runtime_error &err) {
        wavelengthMessages.emplace_back(prefix + err.what());
      }
    }
  }
  if (!inputMessages.empty())
    result["InputWorkspace"] = join(inputMessages);
  if (!wavelengthMessages.empty())
    result["Wavelength"] = join(wavelengthMessages);

  std::vector<double> minVals = this->getProperty("MinValues");
  std::vector<double> maxVals = this->getProperty("MaxValues");

  if (minVals.size() != 3 || maxVals.size() != 3) {
    std::stringstream msg;
    msg << "Must provide 3 values, 1 for every dimension";
    result["MinValues"] = msg.str();
    result["MaxValues"] = msg.str();
  } else {
    std::stringstream msg;

    size_t rank = minVals.size();
    for (size_t i = 0; i < rank; ++i) {
      if (minVals[i] >= maxVals[i]) {
        if (msg.str().empty())
          msg << "max not bigger than min ";
        else
          msg << ", ";
        msg << "at index=" << (i + 1) << " (" << minVals[i] << ">=" << maxVals[i] << ")";
      }
    }

    if (!msg.str().empty()) {
      result["MinValues"] = msg.str();
      result["MaxValues"] = msg.str();
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertHFIRSCDtoMDE::init() {

  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("InputWorkspace", "", Direction::Input),
                  "A detector-space MDHistoWorkspace, or a WorkspaceGroup of them from the same instrument.");
  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "Wavelength", std::make_shared<ArrayBoundedValidator<double>>(0.0, 100.0, true)),
                  "Fallback incident wavelength, in Angstrom. As in HB3AAdjustSampleNorm, the 'wavelength' sample log "
                  "of each input workspace is used when present, and this value only when the log is missing. Give "
                  "one value for all inputs, or, for a WorkspaceGroup, one value per member in group order.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("LorentzCorrection", false, Direction::Input),
                  "Correct the weights of events or signals and errors transformed into "
                  "reciprocal space by multiplying them "
                  "by the Lorentz multiplier:\n :math:`sin(2\\theta)cos(\\phi)/\\lambda^3`");
  declareProperty(std::make_unique<ArrayProperty<double>>("MinValues", "-10,-10,-10"),
                  "It has to be 3 comma separated values, one for each dimension in "
                  "q_sample."
                  "Values smaller then specified here will not be added to "
                  "workspace.");
  declareProperty(std::make_unique<ArrayProperty<double>>("MaxValues", "10,10,10"),
                  "A list of the same size and the same units as MinValues "
                  "list. Values higher or equal to the specified by "
                  "this list will be ignored");
  // Box controller properties. These are the defaults
  this->initBoxControllerProps("5" /*SplitInto*/, 1000 /*SplitThreshold*/, 20 /*MaxRecursionDepth*/);
  declareProperty(std::make_unique<PropertyWithValue<double>>("ObliquityParallaxCoefficient", 1.0, Direction::Input),
                  "Geometrical correction for shift in vertical beam position due to wide beam.");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
      "An MDEventWorkspace in Q-sample. For a WorkspaceGroup input, a WorkspaceGroup whose members are named "
      "<OutputWorkspace>_<input member name>, as in HB3AAdjustSampleNorm.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertHFIRSCDtoMDE::exec() {
  const API::Workspace_sptr input = this->getProperty("InputWorkspace");
  const auto group = std::dynamic_pointer_cast<WorkspaceGroup>(input);
  const auto inputWorkspaces = inputWorkspaceList(input);
  const std::vector<double> wavelengths = this->getProperty("Wavelength");

  if (!group) {
    const double wavelength = resolveWavelength(*inputWorkspaces.front(), fallbackWavelength(wavelengths, 0));
    setProperty("OutputWorkspace", convertWorkspace(inputWorkspaces.front(), wavelength));
    return;
  }

  // One output per member, named as in HB3AAdjustSampleNorm. Each member is declared as an output property so that
  // it is stored in the AnalysisDataService under its name, before the group.
  const std::string outputName = getPropertyValue("OutputWorkspace");
  auto outputGroup = std::make_shared<WorkspaceGroup>();
  Progress progress(this, 0.0, 1.0, inputWorkspaces.size());
  for (size_t i = 0; i < inputWorkspaces.size(); ++i) {
    const double wavelength = resolveWavelength(*inputWorkspaces[i], fallbackWavelength(wavelengths, i));
    auto outputWS = convertWorkspace(inputWorkspaces[i], wavelength);
    const std::string propertyName = "OutputWorkspace_" + std::to_string(i + 1);
    if (existsProperty(propertyName))
      removeProperty(propertyName);
    declareProperty(std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>(
                        propertyName, outputName + "_" + memberName(*group, i), Direction::Output),
                    "Output for member " + std::to_string(i + 1) + " of the input group.");
    setProperty(propertyName, outputWS);
    outputGroup->addWorkspace(outputWS);
    progress.report("Converted " + memberName(*group, i));
  }
  setProperty("OutputWorkspace", std::static_pointer_cast<API::Workspace>(outputGroup));
}

/** The input workspaces to convert: the members of a group in order, or the single input workspace.
 * @param input :: value of the InputWorkspace property
 * @return one entry per workspace, null for an entry that is not an MDHistoWorkspace
 */
std::vector<API::IMDHistoWorkspace_sptr> ConvertHFIRSCDtoMDE::inputWorkspaceList(const API::Workspace_sptr &input) {
  std::vector<API::IMDHistoWorkspace_sptr> inputWorkspaces;
  if (const auto group = std::dynamic_pointer_cast<WorkspaceGroup>(input)) {
    for (size_t i = 0; i < group->size(); ++i)
      inputWorkspaces.emplace_back(std::dynamic_pointer_cast<API::IMDHistoWorkspace>(group->getItem(i)));
  } else if (input) {
    inputWorkspaces.emplace_back(std::dynamic_pointer_cast<API::IMDHistoWorkspace>(input));
  }
  return inputWorkspaces;
}

/** The fallback wavelength given in the Wavelength property for an input workspace, if any.
 * @param wavelengths :: values of the Wavelength property
 * @param index :: position of the input workspace in the group, 0 for a single input
 * @return the value for this workspace, or no value if the property is empty
 */
std::optional<double> ConvertHFIRSCDtoMDE::fallbackWavelength(const std::vector<double> &wavelengths,
                                                              const size_t index) {
  if (wavelengths.empty())
    return std::nullopt;
  if (wavelengths.size() == 1)
    return wavelengths.front();
  return wavelengths.at(index);
}

/** Find the wavelength for a workspace, following HB3AAdjustSampleNorm: the
 * 'wavelength' sample log when present, otherwise the fallback value.
 * @param inputWS :: detector-space workspace to convert
 * @param fallback :: value to use when the sample log is missing
 * @return the wavelength, in Angstrom
 * @throws std::runtime_error if the sample log is not a positive number, or if
 * neither the sample log nor the fallback is available
 */
double ConvertHFIRSCDtoMDE::resolveWavelength(const API::IMDHistoWorkspace &inputWS,
                                              const std::optional<double> &fallback) const {
  const auto &run = inputWS.getExperimentInfo(static_cast<uint16_t>(0))->run();
  if (run.hasProperty(WAVELENGTH_LOG)) {
    double wavelength;
    try {
      // HB3A files store the log as a string, which is converted here
      wavelength = run.getLogAsSingleValue(WAVELENGTH_LOG);
    } catch (const std::invalid_argument &) {
      throw std::runtime_error("The '" + WAVELENGTH_LOG + "' sample log of the input workspace cannot be converted " +
                               "to a number: '" + run.getProperty(WAVELENGTH_LOG)->value() + "'");
    }
    if (!(wavelength > 0.0) || !std::isfinite(wavelength))
      throw std::runtime_error("The '" + WAVELENGTH_LOG + "' sample log of the input workspace must be a positive " +
                               "number, found " + std::to_string(wavelength));
    g_log.information() << "Using wavelength " << wavelength << " Angstrom from the '" << WAVELENGTH_LOG
                        << "' sample log\n";
    return wavelength;
  }
  if (!fallback)
    throw std::runtime_error("No wavelength available: the input workspace has no '" + WAVELENGTH_LOG +
                             "' sample log and the Wavelength property is not set");
  g_log.information() << "Using wavelength " << *fallback << " Angstrom from the Wavelength property\n";
  return *fallback;
}

/** Convert one detector-space workspace into a Q-sample MDEventWorkspace.
 * @param inputWS :: detector-space workspace to convert
 * @param wavelength :: incident wavelength, in Angstrom
 * @return the converted workspace
 */
API::IMDEventWorkspace_sptr ConvertHFIRSCDtoMDE::convertWorkspace(const API::IMDHistoWorkspace_sptr &inputWS,
                                                                  const double wavelength) {
  bool lorentz = getProperty("LorentzCorrection");

  auto &expInfo = *(inputWS->getExperimentInfo(static_cast<uint16_t>(0)));
  std::string instrument = expInfo.getInstrument()->getName();

  std::vector<double> twotheta, azimuthal;
  std::vector<int> detectorID; // detector ID for each (twotheta, azimuthal), most useful when detector grouping done
  if (instrument == "HB3A" && !expInfo.run().hasProperty("azimuthal")) { // HB3A Load MD
    const auto &di = expInfo.detectorInfo();
    for (size_t x = 0; x < 512; x++) {
      for (size_t y = 0; y < 512 * 3; y++) {
        size_t n = x + y * 512;
        if (!di.isMonitor(n)) {
          twotheta.push_back(di.twoTheta(n));
          azimuthal.push_back(di.azimuthal(n));
          detectorID.push_back(static_cast<int>(1 + n)); // detector ID's start at 1 in HB3A
        }
      }
    }
  } else { // HB2C LoadWAND or HB3A HB3AAdjustSampleNorm
    azimuthal = (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(expInfo.getLog("azimuthal"))))();
    twotheta = (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(expInfo.getLog("twotheta"))))();
    if (expInfo.run().hasProperty("detectorID")) {
      detectorID = (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<int>> *>(expInfo.getLog("detectorID"))))();
    } else {
      // backwards compatibility if sample-log is not present
      detectorID.assign(azimuthal.size(), 0);
    }
  }

  auto outputWS = DataObjects::MDEventFactory::CreateMDWorkspace(3, "MDEvent");
  Mantid::Geometry::QSample frame;
  std::vector<double> minVals = this->getProperty("MinValues");
  std::vector<double> maxVals = this->getProperty("MaxValues");
  outputWS->addDimension(std::make_shared<Geometry::MDHistoDimension>(
      "Q_sample_x", "Q_sample_x", frame, static_cast<coord_t>(minVals[0]), static_cast<coord_t>(maxVals[0]), 1));

  outputWS->addDimension(std::make_shared<Geometry::MDHistoDimension>(
      "Q_sample_y", "Q_sample_y", frame, static_cast<coord_t>(minVals[1]), static_cast<coord_t>(maxVals[1]), 1));

  outputWS->addDimension(std::make_shared<Geometry::MDHistoDimension>(
      "Q_sample_z", "Q_sample_z", frame, static_cast<coord_t>(minVals[2]), static_cast<coord_t>(maxVals[2]), 1));
  outputWS->setCoordinateSystem(Mantid::Kernel::QSample);
  outputWS->initialize();

  BoxController_sptr bc = outputWS->getBoxController();
  this->setBoxController(bc);
  outputWS->splitBox();

  auto mdws_mdevt_3 = std::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3>>(outputWS);
  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> inserter(mdws_mdevt_3);

  double cop = this->getProperty("ObliquityParallaxCoefficient");
  float coeff = static_cast<float>(cop);

  float k = boost::math::float_constants::two_pi / static_cast<float>(wavelength);
  float inv_wl_cube = static_cast<float>(1 / (wavelength * wavelength * wavelength));
  // check convention to determine the sign of k
  std::string convention = Kernel::ConfigService::Instance().getString("Q.convention");
  if (convention == "Crystallography") {
    k *= -1.f;
  }
  std::vector<Eigen::Vector3f> q_lab_pre;
  std::vector<float> lorentz_pre;
  q_lab_pre.reserve(azimuthal.size());
  lorentz_pre.reserve(azimuthal.size());
  for (size_t m = 0; m < azimuthal.size(); ++m) {
    auto twotheta_f = static_cast<float>(twotheta[m]);
    auto azimuthal_f = static_cast<float>(azimuthal[m]);
    q_lab_pre.push_back({-std::sin(twotheta_f) * std::cos(azimuthal_f) * k,
                         -std::sin(twotheta_f) * std::sin(azimuthal_f) * k * coeff, (1.f - std::cos(twotheta_f)) * k});
    lorentz_pre.push_back(std::abs(std::sin(twotheta_f) * std::cos(azimuthal_f)) * inv_wl_cube);
  }
  float factor = 1;
  const auto run = inputWS->getExperimentInfo(0)->run();
  for (size_t n = 0; n < inputWS->getDimension(2)->getNBins(); n++) {
    auto gon = run.getGoniometerMatrix(n);
    Eigen::Matrix3f goniometer;
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        goniometer(i, j) = static_cast<float>(gon[i][j]);
    goniometer = goniometer.inverse().eval();
    auto goniometerIndex = static_cast<uint16_t>(n);
    for (size_t m = 0; m < azimuthal.size(); m++) {
      size_t idx = n * azimuthal.size() + m;
      coord_t signal = static_cast<coord_t>(inputWS->getSignalAt(idx));
      if (signal > 0.f && std::isfinite(signal)) {
        Eigen::Vector3f q_sample = goniometer * q_lab_pre[m];
        if (lorentz) {
          factor = lorentz_pre[m];
        }
        inserter.insertMDEvent(signal * factor, signal * factor * factor, 0, goniometerIndex, detectorID[m],
                               q_sample.data());
      }
    }
  }

  auto *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts);
  outputWS->splitAllIfNeeded(ts);
  tp.joinAll();

  outputWS->refreshCache();
  outputWS->copyExperimentInfos(*inputWS);
  auto &outRun = outputWS->getExperimentInfo(0)->mutableRun();
  if (outRun.hasProperty(WAVELENGTH_LOG)) {
    outRun.removeLogData(WAVELENGTH_LOG);
  }
  outRun.addLogData(new PropertyWithValue<double>(WAVELENGTH_LOG, wavelength));
  outRun.getProperty(WAVELENGTH_LOG)->setUnits("Angstrom");

  auto user_convention = Kernel::ConfigService::Instance().getString("Q.convention");
  auto ws_convention = outputWS->getConvention();
  if (user_convention != ws_convention) {
    auto convention_alg = createChildAlgorithm("ChangeQConvention");
    convention_alg->setProperty("InputWorkspace", outputWS);
    convention_alg->executeAsChildAlg();
  }
  return outputWS;
}

} // namespace Mantid::MDAlgorithms
