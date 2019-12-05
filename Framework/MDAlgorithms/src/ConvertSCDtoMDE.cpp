// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/ConvertSCDtoMDE.h"
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
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitLabelTypes.h"

#include "Eigen/Dense"
#include "boost/math/constants/constants.hpp"

namespace Mantid {
namespace MDAlgorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertSCDtoMDE)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConvertSCDtoMDE::name() const { return "ConvertSCDtoMDE"; }

/// Algorithm's version for identification. @see Algorithm::version
int ConvertSCDtoMDE::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertSCDtoMDE::category() const {
  return "MDAlgorithm\\Creation";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvertSCDtoMDE::summary() const {
  return "Convert from the detector vs scan index MDHistoWorkspace into a "
         "MDEventWorkspace with units in Q_sample.";
}

std::map<std::string, std::string> ConvertSCDtoMDE::validateInputs() {
  std::map<std::string, std::string> result;

  std::vector<double> minVals = this->getProperty("MinValues");
  std::vector<double> maxVals = this->getProperty("MaxValues");

  if (minVals.size() != maxVals.size()) {
    std::stringstream msg;
    msg << "Rank of MinValues != MaxValues (" << minVals.size()
        << "!=" << maxVals.size() << ")";
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
        msg << "at index=" << (i + 1) << " (" << minVals[i]
            << ">=" << maxVals[i] << ")";
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
void ConvertSCDtoMDE::init() {

  // Box controller properties. These are the defaults
  this->initBoxControllerProps("5" /*SplitInto*/, 1000 /*SplitThreshold*/,
                               20 /*MaxRecursionDepth*/);
  declareProperty(std::make_unique<WorkspaceProperty<API::IMDHistoWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "wavelength", Mantid::EMPTY_DBL(), Direction::Input),
                  "wavelength");
  declareProperty(std::make_unique<ArrayProperty<double>>("MinValues"),
                  "It has to be N comma separated values, where N is the "
                  "number of dimensions of the target workspace. Values "
                  "smaller then specified here will not be added to "
                  "workspace.\n Number N is defined by properties 4,6 and 7 "
                  "and "
                  "described on *MD Transformation factory* page. See also "
                  ":ref:`algm-ConvertToMDMinMaxLocal`");
  declareProperty(std::make_unique<ArrayProperty<double>>("MaxValues"),
                  "A list of the same size and the same units as MinValues "
                  "list. Values higher or equal to the specified by "
                  "this list will be ignored");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertSCDtoMDE::exec() {
  double wavelength = this->getProperty("wavelength");
  if (wavelength == Mantid::EMPTY_DBL()) {
    throw std::invalid_argument("wavelength not entered!");
  }

  API::IMDHistoWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  auto &expInfo = *(inputWS->getExperimentInfo(static_cast<uint16_t>(0)));
  std::string instrument = expInfo.getInstrument()->getName();
  g_log.notice() << instrument << "\n";
  std::vector<double> twotheta, azimuthal;
  std::vector<double> s1, omega, chi, phi;
  if (instrument == "HB3A") {
    auto omegaLog = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
        expInfo.run().getLogData("omega"));
    omega = omegaLog->valuesAsVector();
    auto chiLog = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
        expInfo.run().getLogData("chi"));
    chi = chiLog->valuesAsVector();
    auto phiLog = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
        expInfo.run().getLogData("phi"));
    phi = phiLog->valuesAsVector();
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
  } else { // HB2C
    s1 = (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(
        expInfo.getLog("s1"))))();
    azimuthal =
        (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(
            expInfo.getLog("azimuthal"))))();
    twotheta =
        (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(
            expInfo.getLog("twotheta"))))();
  }

  auto outputWS = DataObjects::MDEventFactory::CreateMDWorkspace(3, "MDEvent");
  Mantid::Geometry::QSample frame;
  std::vector<double> minVals = this->getProperty("MinValues");
  std::vector<double> maxVals = this->getProperty("MaxValues");
  outputWS->addDimension(boost::make_shared<Geometry::MDHistoDimension>(
      "Q_sample_x", "Q_sample_x", frame, static_cast<coord_t>(minVals[0]),
      static_cast<coord_t>(maxVals[0]), 1));

  outputWS->addDimension(boost::make_shared<Geometry::MDHistoDimension>(
      "Q_sample_y", "Q_sample_y", frame, static_cast<coord_t>(minVals[1]),
      static_cast<coord_t>(maxVals[1]), 1));

  outputWS->addDimension(boost::make_shared<Geometry::MDHistoDimension>(
      "Q_sample_z", "Q_sample_z", frame, static_cast<coord_t>(minVals[2]),
      static_cast<coord_t>(maxVals[2]), 1));
  outputWS->setCoordinateSystem(Mantid::Kernel::QSample);
  outputWS->initialize();

  BoxController_sptr bc = outputWS->getBoxController();
  this->setBoxController(bc);
  outputWS->splitBox();

  auto mdws_mdevt_3 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3>>(outputWS);
  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> inserter(mdws_mdevt_3);

  float k =
      boost::math::float_constants::two_pi / static_cast<float>(wavelength);
  std::vector<Eigen::Vector3f> q_lab_pre;
  q_lab_pre.reserve(azimuthal.size());
  for (size_t m = 0; m < azimuthal.size(); ++m) {
    auto twotheta_f = static_cast<float>(twotheta[m]);
    auto azimuthal_f = static_cast<float>(azimuthal[m]);
    q_lab_pre.push_back({-sin(twotheta_f) * cos(azimuthal_f) * k,
                         -sin(twotheta_f) * sin(azimuthal_f) * k,
                         (1.f - cos(twotheta_f)) * k});
  }

  for (size_t n = 0; n < inputWS->getDimension(2)->getNBins(); n++) {
    Eigen::Matrix3f goniometer;
    if (instrument == "HB3A") {
      float omega_radian =
          static_cast<float>(omega[n]) * boost::math::float_constants::degree;
      float chi_radian =
          static_cast<float>(chi[n]) * boost::math::float_constants::degree;
      float phi_radian =
          static_cast<float>(phi[n]) * boost::math::float_constants::degree;
      Eigen::Matrix3f r1;
      r1 << cos(omega_radian), 0, -sin(omega_radian), 0, 1, 0,
          sin(omega_radian), 0, cos(omega_radian); // omega 0,1,0,-1
      Eigen::Matrix3f r2;
      r2 << cos(chi_radian), sin(chi_radian), 0, -sin(chi_radian),
          cos(chi_radian), 0, 0, 0, 1; // chi 0,0,1,-1
      Eigen::Matrix3f r3;
      r3 << cos(phi_radian), 0, -sin(phi_radian), 0, 1, 0, sin(phi_radian), 0,
          cos(phi_radian); // phi 0,1,0,-1
      goniometer = r1 * r2 * r3;
    } else { // HB2C
      float s1_radian =
          static_cast<float>(s1[n]) * boost::math::float_constants::degree;
      goniometer << cos(s1_radian), 0, sin(s1_radian), 0, 1, 0, -sin(s1_radian),
          0, cos(s1_radian); // s1 0,1,0,1
    }
    goniometer = goniometer.inverse().eval();
    for (size_t m = 0; m < azimuthal.size(); m++) {
      size_t idx = n * azimuthal.size() + m;
      coord_t signal = static_cast<coord_t>(inputWS->getSignalAt(idx));
      if (signal > 0.f) {
        Eigen::Vector3f q_sample = goniometer * q_lab_pre[m];
        inserter.insertMDEvent(signal, signal, 0, 0, q_sample.data());
      }
    }
  }

  auto *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts);
  outputWS->splitAllIfNeeded(ts);
  tp.joinAll();

  outputWS->refreshCache();
  outputWS->copyExperimentInfos(*inputWS);

  auto user_convention =
      Kernel::ConfigService::Instance().getString("Q.convention");
  auto ws_convention = outputWS->getConvention();
  if (user_convention != ws_convention) {
    auto convention_alg = createChildAlgorithm("ChangeQConvention");
    convention_alg->setProperty("InputWorkspace", outputWS);
    convention_alg->executeAsChildAlg();
  }
  setProperty("OutputWorkspace", outputWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
