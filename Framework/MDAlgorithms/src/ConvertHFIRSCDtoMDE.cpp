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
#include "MantidAPI/Run.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
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

std::map<std::string, std::string> ConvertHFIRSCDtoMDE::validateInputs() {
  std::map<std::string, std::string> result;

  API::IMDHistoWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
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
  if (!inputWSmsg.str().empty())
    result["InputWorkspace"] = inputWSmsg.str();

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

  declareProperty(std::make_unique<WorkspaceProperty<API::IMDHistoWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(
      std::make_unique<PropertyWithValue<double>>(
          "Wavelength", DBL_MAX, std::make_shared<BoundedValidator<double>>(0.0, 100.0, true), Direction::Input),
      "Wavelength");
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
  declareProperty(std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertHFIRSCDtoMDE::exec() {
  double wavelength = this->getProperty("Wavelength");
  bool lorentz = getProperty("LorentzCorrection");

  API::IMDHistoWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  auto &expInfo = *(inputWS->getExperimentInfo(static_cast<uint16_t>(0)));
  std::string instrument = expInfo.getInstrument()->getName();

  std::vector<double> twotheta, azimuthal;
  if (instrument == "HB3A" && !expInfo.run().hasProperty("azimuthal")) { // HB3A Load MD
    const auto &di = expInfo.detectorInfo();
    for (size_t x = 0; x < 512; x++) {
      for (size_t y = 0; y < 512 * 3; y++) {
        size_t n = x + y * 512;
        if (!di.isMonitor(n)) {
          twotheta.push_back(di.twoTheta(n));
          azimuthal.push_back(di.azimuthal(n));
        }
      }
    }
  } else { // HB2C LoadWAND or HB3A HB3AAdjustSampleNorm
    azimuthal = (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(expInfo.getLog("azimuthal"))))();
    twotheta = (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(expInfo.getLog("twotheta"))))();
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
        inserter.insertMDEvent(signal * factor, signal * factor * factor, 0, goniometerIndex, 0, q_sample.data());
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
  if (outRun.hasProperty("wavelength")) {
    outRun.removeLogData("wavelength");
  }
  outRun.addLogData(new PropertyWithValue<double>("wavelength", wavelength));
  outRun.getProperty("wavelength")->setUnits("Angstrom");

  auto user_convention = Kernel::ConfigService::Instance().getString("Q.convention");
  auto ws_convention = outputWS->getConvention();
  if (user_convention != ws_convention) {
    auto convention_alg = createChildAlgorithm("ChangeQConvention");
    convention_alg->setProperty("InputWorkspace", outputWS);
    convention_alg->executeAsChildAlg();
  }
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::MDAlgorithms
