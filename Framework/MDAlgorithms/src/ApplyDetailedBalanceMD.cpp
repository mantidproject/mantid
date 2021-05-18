// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ApplyDetailedBalanceMD.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
// #include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MDGeometry.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include <math.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyDetailedBalanceMD)

/// Algorithm's name for identification. @see Algorithm::name
const std::string ApplyDetailedBalanceMD::name() const { return "ApplyDetailedBalanceMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int ApplyDetailedBalanceMD::version() const { return 1; }

/// Summary
const std::string ApplyDetailedBalanceMD::summary() const { return "Apply detailed balance to MDEventWorkspace"; }

/// category
const std::string ApplyDetailedBalanceMD::category() const { return "MDAlgorithms"; }

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Define input and output properties
 */
void ApplyDetailedBalanceMD::init() {

  declareProperty(
      std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("InputWorkspace", "", Kernel::Direction::Input),
      "An input MDEventWorkspace.  Must be in Q_sample/Q_lab frame.  Must have an axis as DeltaE");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("Temperature", "", Direction::Input),
                  "SampleLog variable name that contains the temperature");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The output MDEventWorkspace with detailed balance applied");
}

/**
 * @brief Main execution body
 */
void ApplyDetailedBalanceMD::exec() {
  // Process inputs
  // Check temperature log
  if (mTemperature < 0 && mExpinfoTemperatureMean.size() == 0) {
    throw std::runtime_error("Temperature log is not set up correct.");
  }
  if (mDeltaEIndex != 1 && mDeltaEIndex != 3)
    throw std::runtime_error("Workspace dimension is not checked.");

  // Process input workspace and create output workspace
  API::IMDEventWorkspace_sptr input_ws = getProperty("InputWorkspace");
  std::string output_ws_name = getPropertyValue("OutputWorkspace");

  API::IMDEventWorkspace_sptr output_ws(0);
  if (input_ws->getName() == output_ws_name) {
    // Calcualte in-place
    output_ws = input_ws;
  } else {
    // Clone input workace to output workspace
    output_ws = input_ws->clone();
  }

  // Apply detailed balance to MDEvents
  CALL_MDEVENT_FUNCTION(applyDetailedBalance, output_ws);

  // refresh cache for MDBoxes: set correct Box signal
  output_ws->refreshCache();

  // Clear masking (box flags) from the output workspace
  output_ws->clearMDMasking();

  // Set output
  setProperty("OutputWorkspace", output_ws);
}

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Validate inputs
 * Input MDEventWorkspace dimensions:
 * - in Q_sample or Q_lab frame, the 4th dimension is DeltaE, or
 * - the first dimension is |Q| and second is DeltaE
 * validate that run objects have Ei defined (number greater than 0)
 * first input is an MD event workspace as validated above
second input is Temperature - must be either a float>0, or a string pointing to a log name.
if temperature points to a log name, it must be present in each experiment info, and the average must be greater than 0
 */
std::map<std::string, std::string> ApplyDetailedBalanceMD::validateInputs() {
  std::map<std::string, std::string> output;

  // Get input workspace
  API::IMDEventWorkspace_sptr input_ws = getProperty("InputWorkspace");
  if (input_ws == nullptr) {
    output["InputWorkspace"] = "Input workspace is Null";
  } else {
    // check input dimension
    std::string dim_error = checkInputMDDimension();
    if (dim_error != "") {
      output["InputWorkspace"] = dim_error;
    }

    std::string kerror = getTemperature(input_ws);
    if (kerror != "")
      output["Temperature"] = kerror;
  }

  return output;
}

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Check input MDEventWorkspace dimension
 * validate dimensions: input workspace is in Q_sample or Q_lab frame, and the 4th dimension is DeltaE, or the first
 * dimension is |Q| and second is DeltaE
 * @return
 */
std::string ApplyDetailedBalanceMD::checkInputMDDimension() {
  std::string errormsg("");

  API::IMDEventWorkspace_sptr inputws = getProperty("InputWorkspace");
  size_t numdims = inputws->getNumDims();

  // Get and check the dimensions: Q3D or Q1D
  const Mantid::Kernel::SpecialCoordinateSystem coordsys = inputws->getSpecialCoordinateSystem();
  size_t qdim(0);
  std::string qdimstr("Not Q3D or |Q|");
  if (coordsys == Mantid::Kernel::SpecialCoordinateSystem::QLab ||
      coordsys == Mantid::Kernel::SpecialCoordinateSystem::QSample) {
    // q3d
    qdim = 3;
    qdimstr = "Q3D";
  } else {
    // search Q1D: at any place
    for (size_t i = 0; i < numdims; ++i) {
      if (inputws->getDimension(i)->getName() == "|Q|") {
        qdim = 1;
        qdimstr = "|Q|";
        break;
      }
    }
  }

  // Check DeltaE
  if (qdim == 1 && inputws->getDimension(1)->getName() == "DeltaE") {
    // 2nd dimension is DeltaE
    mDeltaEIndex = 1;
  } else if (qdim == 3 && inputws->getDimension(3)->getName() == "DeltaE") {
    // 4th dimension is DeltaE
    mDeltaEIndex = 3;
  } else {
    // Error
    g_log.error() << "Coordiate system = " << coordsys << " does not meet requirement: \n";
    for (size_t i = 0; i < numdims; ++i) {
      g_log.error() << i << "-th dim: " << inputws->getDimension(i)->getName() << "\n";
    }
    errormsg += "Q Dimension (" + qdimstr +
                ") is neither Q3D nor |Q|.  Or DeltaE is found in proper place (2nd or 4th dimension).";
  }

  return errormsg;
}

//---------------------------------------------------------------------------------------------
/**
 * @brief Apply detailed balance to each MDEvent in MDEventWorkspace
 */
template <typename MDE, size_t nd>
void ApplyDetailedBalanceMD::applyDetailedBalance(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws) {
  // toy around
  g_log.notice() << "[DEVELOP] " << ws->getName() << "\n";
  uint64_t numevents = ws->getNEvents();
  g_log.notice() << "[DEVELOP] number of events = " << numevents << "\n";

  // Get Box from MDEventWorkspace
  MDBoxBase<MDE, nd> *box1 = ws->getBox();
  std::vector<API::IMDNode *> boxes;
  box1->getBoxes(boxes, 1000, true);
  auto numBoxes = int(boxes.size());

  g_log.notice() << "[DEVELOP]  Number of boxex = " << numBoxes << "\n";

  // Add the boxes in parallel. They should be spread out enough on each
  // core to avoid stepping on each other.
  // cppcheck-suppress syntaxError
  PRAGMA_OMP( parallel for if (!ws->isFileBacked()))
  for (int i = 0; i < numBoxes; ++i) {
    PARALLEL_START_INTERUPT_REGION
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
    if (box && !box->getIsMasked()) {
      // Copy the events from WS2 and add them into WS1
      std::vector<MDE> &events = box->getEvents();
      // Add events, with bounds checking
      for (auto it = events.begin(); it != events.end(); ++it) {
        // Create the event
        // MDE newEvent(it->getSignal(), it->getErrorSquared(), it->getCenter());
        // std::cout << it->getSignal() << "\n";
        // do calculattion
        float temperatue(mTemperature);
        if (temperatue < 0) {
          const uint16_t exp_info_index(it->getExpInfoIndex());
          double ot = mExpinfoTemperatureMean[exp_info_index];
          temperatue = static_cast<float>(ot);
        }
        float delta_e = it->getCenter(mDeltaEIndex);
        double one_over_kb_t(PhysicalConstants::meVtoKelvin / temperatue);
        float x = static_cast<float>(one_over_kb_t);
        float pi = static_cast<float>(M_PI);
        float factor = pi * (static_cast<float>(1.) - exp(-delta_e * x));

        //        g_log.notice() << "[DEVELOP] signal = " << it->getSignal() << ", factor = " << factor
        //                       << ", DeltaE = " << delta_e << "\n";

        // calcalate and set intesity
        auto intensity = it->getSignal();
        intensity *= factor;
        it->setSignal(intensity);

        // calculate and set error
        auto error2 = it->getErrorSquared();
        error2 *= factor * factor;
        it->setErrorSquared(error2);
      }
    }
    box->releaseEvents();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  //  g_log.notice() << "[DEVELOP] traverse  events = " << count << "\n";

  //  for (int i = 0; i < numBoxes; ++i) {
  //    // FIXME PARALLEL_START_INTERUPT_REGION
  //    auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
  //    if (box && !box->getIsMasked()) {
  //      // Copy the events from WS2 and add them into WS1
  //      std::vector<MDE> &events = box->getEvents();
  //      // Add events, with bounds checking
  //      for (auto it = events.begin(); it != events.end(); ++it) {
  //        // Create the event
  //        // MDE newEvent(it->getSignal(), it->getErrorSquared(), it->getCenter());
  //        // std::cout << it->getSignal() << "\n";
  //        count += 1;
  //        // get the experiment info
  //        const uint16_t exp_index(it->getExpInfoIndex());
  //        // get the coordiate
  //        auto deltae = it->getCenter(mDeltaEIndex);

  //        g_log.notice() << "[DEBUG] signal = " << it->getSignal()
  //                       << " DeltaE = " << deltae
  //                       << "Q = " << it->getCenter(0) << "\n";
  //      }
  //    }
  //    // FIXME PARALLEL_END_INTERUPT_REGION
  //  }

  return;
}

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Retrieve sample temperature
 * Temperature value can be specified by either property Temperature, or
 * it can be calcualted from sample temperture log in the MDWorkspace
 */
std::string ApplyDetailedBalanceMD::getTemperature(API::IMDEventWorkspace_sptr mdws) {
  // Get temperture sample log name
  std::string Tstring = getProperty("Temperature");
  std::string temperature_error("");

  try {
    // Check whether it is a sample log
    bool all_have_temp(true);
    mExpinfoTemperatureMean.clear();
    uint16_t numexpinfo = mdws->getNumExperimentInfo();

    for (uint16_t i = 0; i < numexpinfo; ++i) {
      ExperimentInfo_const_sptr expinfo = mdws->getExperimentInfo(i);
      bool has_temp = expinfo->run().hasProperty(Tstring);
      if (has_temp) {
        auto log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(expinfo->run().getProperty(Tstring));
        if (!log) {
          g_log.error() << Tstring << " is not a valid double-valued log"
                        << "\n";
          throw std::invalid_argument(Tstring + " is not a double-valued log.");
        }
        mExpinfoTemperatureMean[i] = log->getStatistics().mean;
      } else {
        g_log.error() << "Spectrum info " << i << " does not have tempertaure log " << Tstring << "\n";
        all_have_temp = false;
      }
    }

    // If at least 1 experiment info does not have temperature log
    if (!all_have_temp) {
      // convert from string to double
      mTemperature = boost::lexical_cast<float>(Tstring);
      // reset map
      mExpinfoTemperatureMean.clear();
    }
  } catch (...) {
    Tstring += " is not a valid log, nor is it a number";
    temperature_error += Tstring;
  }

  return temperature_error;
}

void ApplyDetailedBalanceMD::showAnyEvents(const std::string &mdwsname) {
  // Compare
  // showMDEvents(outputname, 3);
  // showMDEvents(gold_detail_balanced_singe_name, 3);
  // Get workspace
  API::IMDEventWorkspace_sptr ws =
      std::dynamic_pointer_cast<API::IMDEventWorkspace>(API::AnalysisDataService::Instance().retrieve(mdwsname));
  if (ws == nullptr) {
    throw std::runtime_error("It is not an MDEventWorkspace");
  }
  // Work with MDEvents
  CALL_MDEVENT_FUNCTION(showMDEvents, ws); // , gold_detail_balanced_singe_name, 3);
}

template <typename MDE, size_t nd>
void ApplyDetailedBalanceMD::showMDEvents(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws) {

  // Print out summary
  size_t num_events = ws->getNEvents();
  g_log.notice() << "[Verify] " << ws->getName() << ": MD events = " << num_events << "\n";
  for (size_t i = 0; i < ws->getNumDims(); ++i)
    g_log.error() << "[Verify] " << i << "-th dim: " << ws->getDimension(i)->getName() << "\n";

  // Get Box from MDEventWorkspace
  MDBoxBase<MDE, nd> *box1 = ws->getBox();
  std::vector<API::IMDNode *> boxes;
  box1->getBoxes(boxes, 1000, true);
  auto numBoxes = int(boxes.size());

  g_log.notice() << "[Verify] Number of boxes = " << numBoxes << "\n";

  for (int i = 0; i < numBoxes; ++i) {
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
    if (box && !box->getIsMasked()) {
      std::vector<MDE> &events = box->getEvents();
      for (auto it = events.begin(); it != events.end(); ++it) {
        // get the coordiate
        auto deltae = it->getCenter(mDeltaEIndex);

        g_log.notice() << "[Verify] Signal = " << it->getSignal() << " DeltaE = " << deltae
                       << " Coordinate = " << it->getCenter(0) << ", " << it->getCenter(1) << ", " << it->getCenter(2)
                       << "\n";
      }
    }
    // FIXME PARALLEL_END_INTERUPT_REGION
  }
}

} // namespace MDAlgorithms
} // namespace Mantid
