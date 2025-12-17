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
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid::MDAlgorithms {

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
                  "SampleLog variable name that contains the temperature or a number");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  mustBePositive->setLowerExclusive(true);
  declareProperty(std::make_unique<PropertyWithValue<double>>("RescaleToTemperature", EMPTY_DBL(), mustBePositive,
                                                              Direction::Input),
                  "The temperature to which to rescale the intensity");

  declareProperty(
      std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The output MDEventWorkspace with detailed balance applied");
}

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Main execution body
 */
void ApplyDetailedBalanceMD::exec() {
  // Get input workspace
  API::IMDEventWorkspace_sptr input_ws = getProperty("InputWorkspace");

  // Get temperature to convert to
  mFinalTemperature = getProperty("RescaleToTemperature");
  if (mFinalTemperature > EMPTY_DBL() / 2) {
    mFinalTemperature = -10.; // set to an unphysical value
  }

  // Process input workspace and create output workspace
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

  // check input dimension
  std::string dim_error = checkInputMDDimension();
  if (dim_error != "") {
    output["InputWorkspace"] = dim_error;
  }

  std::string kerror = getTemperature(input_ws);
  if (kerror != "")
    output["Temperature"] = kerror;

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
  // Get Box from MDEventWorkspace
  MDBoxBase<MDE, nd> *box1 = ws->getBox();
  std::vector<API::IMDNode *> boxes;
  box1->getBoxes(boxes, 1000, true);
  auto numBoxes = int(boxes.size());

  // Add the boxes in parallel. They should be spread out enough on each
  // core to avoid stepping on each other.

  PRAGMA_OMP( parallel for if (!ws->isFileBacked()))
  for (int i = 0; i < numBoxes; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
    if (box && !box->getIsMasked()) {
      // get the MEEvents from box
      std::vector<MDE> &events = box->getEvents();
      // Add events, with bounds checking
      for (auto it = events.begin(); it != events.end(); ++it) {
        // Create the event
        // do calculattion
        float temperature(static_cast<float>(mExpinfoTemperatureMean[it->getExpInfoIndex()]));
        float factor;

        // delta_e = it->getCenter(mDeltaEIndex);
        // factor = pi * (1 - exp(-deltaE/(kb*T)))
        // using expm1=e^x-1 function
        if (mFinalTemperature < 0) { // convert to chi''
          factor =
              -static_cast<float>(M_PI) * std::expm1f(-static_cast<float>(it->getCenter(mDeltaEIndex)) *
                                                      static_cast<float>(PhysicalConstants::meVtoKelvin / temperature));
        } else {
          float de = static_cast<float>(it->getCenter(mDeltaEIndex));
          if (std::fabs(de) < 1e-10) {
            factor = static_cast<float>(mFinalTemperature) / temperature;
          } else {
            factor = std::expm1f(-de * static_cast<float>(PhysicalConstants::meVtoKelvin / temperature)) /
                     std::expm1f(-de * static_cast<float>(PhysicalConstants::meVtoKelvin / mFinalTemperature));
          }
        }

        // calcalate and set intesity
        auto intensity = it->getSignal() * factor;
        it->setSignal(intensity);

        // calculate and set error
        auto error2 = it->getErrorSquared() * factor * factor;
        // error2 *= factor * factor;
        it->setErrorSquared(error2);
      }
    }
    if (box) {
      box->releaseEvents();
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  return;
}

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Retrieve sample temperature
 * Temperature value can be specified by either property Temperature, or
 * it can be calcualted from sample temperture log in the MDWorkspace
 */
std::string ApplyDetailedBalanceMD::getTemperature(const API::IMDEventWorkspace_sptr &mdws) {
  // Get temperture sample log name
  std::string Tstring = getProperty("Temperature");
  std::string temperature_error("");

  // Try to convert Tstring to a float
  float temperature;
  try {
    temperature = boost::lexical_cast<float>(Tstring);
  } catch (...) {
    // set to a unphysical value
    temperature = -10;
  }

  // the input property could be a valid float; if not must search the experiment info
  mExpinfoTemperatureMean.clear();
  uint16_t numexpinfo = mdws->getNumExperimentInfo();

  for (uint16_t i = 0; i < numexpinfo; ++i) {
    if (temperature < 0) {
      // if user specified is not a valid float
      ExperimentInfo_const_sptr expinfo = mdws->getExperimentInfo(i);
      if (expinfo->run().hasProperty(Tstring)) {
        auto log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(expinfo->run().getProperty(Tstring));
        if (!log) {
          // wrong type of sample log: must be TimeSeriesProperty<double>
          std::stringstream errss;
          errss << "ExperimentInfo" << i << " has " << Tstring << ", which is not a valid double-valuesd log";
          temperature_error += errss.str() + "\n";
        } else {
          mExpinfoTemperatureMean[i] = log->getStatistics().mean;
        }
      } else {
        // specified sample log does not exist
        std::stringstream errss;
        errss << "ExperimentInfo " << i << " does not have tempertaure log " << Tstring;
        temperature_error += errss.str() + "\n";
      }
    } else {
      // set user specified temperature to map
      mExpinfoTemperatureMean[i] = temperature;
    }
  }

  return temperature_error;
}

} // namespace Mantid::MDAlgorithms
