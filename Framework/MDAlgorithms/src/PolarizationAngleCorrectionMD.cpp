// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/PolarizationAngleCorrectionMD.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MDGeometry.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/BoundedValidator.h"
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

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PolarizationAngleCorrectionMD)

/// Algorithm's name for identification. @see Algorithm::name
const std::string PolarizationAngleCorrectionMD::name() const { return "PolarizationAngleCorrectionMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int PolarizationAngleCorrectionMD::version() const { return 1; }

/// Summary
const std::string PolarizationAngleCorrectionMD::summary() const {
  return "Apply polarization angle correction to MDEventWorkspace";
}

/// category
const std::string PolarizationAngleCorrectionMD::category() const { return "MDAlgorithms"; }

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Define input and output properties
 */
void PolarizationAngleCorrectionMD::init() {

  declareProperty(
      std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("InputWorkspace", "", Kernel::Direction::Input),
      "An input MDEventWorkspace.  Must be in Q_sample/Q_lab frame.  Must have an axis as DeltaE");

  auto anglerange = std::make_shared<BoundedValidator<double>>();
  anglerange->setLower(-180.);
  anglerange->setUpper(180.);
  declareProperty("PolarizationAngle", 0., anglerange,
                  "An in-plane polarization angle​, between -180 and 180 degrees");

  auto precisionrange = std::make_shared<BoundedValidator<double>>();
  precisionrange->setLower(0.);
  precisionrange->setUpper(1.);
  declareProperty(
      "Precision", 1., precisionrange,
      "Precision (between 0 and 1). Any event whose absolute value of cosine of 2 of its schaf angle less than this "
      "precision will be ignored.");

  declareProperty(
      std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The output MDEventWorkspace with polarization angle correction applied");
}

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Main execution body
 */
void PolarizationAngleCorrectionMD::exec() {
  // Get input workspace and other parameters
  API::IMDEventWorkspace_sptr input_ws = getProperty("InputWorkspace");
  mPolarizationAngle = getProperty("PolarizationAngle");
  mPolarizationAngle *= M_PI / 180.; // convert to arcs
  mPrecision = getProperty("Precision");

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

  // Apply polarization angle correction to MDEvents
  CALL_MDEVENT_FUNCTION(applyPolarizationAngleCorrection, output_ws);

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
std::map<std::string, std::string> PolarizationAngleCorrectionMD::validateInputs() {
  std::map<std::string, std::string> output;

  // Get input workspace
  API::IMDEventWorkspace_sptr input_ws = getProperty("InputWorkspace");

  // check input dimension
  std::string dim_error = checkInputMDDimension();
  if (dim_error != "") {
    output["InputWorkspace"] = dim_error;
  }

  return output;
}

//---------------------------------------------------------------------------------------------
/**
 * @brief Apply polarization angle correction to each MDEvent in MDEventWorkspace
 */
template <typename MDE, size_t nd>
void PolarizationAngleCorrectionMD::applyPolarizationAngleCorrection(
    typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws) {
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
        // Modify the event

        // Calculate Gamma
        double qx(0.), qz(0.);
        if (!mIsQSample) {
          // Q-lab: gamma = arctan2(Qx​,Qz​)
          qx = static_cast<double>(it->getCenter(mQxIndex));
          qz = static_cast<double>(it->getCenter(mQzIndex));
        } else {
          // Q-sample
          // Qlab = R * QSample
          std::vector<double> qsample = {it->getCenter(0), it->getCenter(1), it->getCenter(2)};
          std::vector<double> qlab = mRotationMatrixMap[it->getExpInfoIndex()] * qsample;
          qx = qlab[0];
          qz = qlab[2];
        }
        double gamma(std::atan2(qx, qz)); // unit = arc

        // The Scharpf angle \alphs = \gamma - P_A
        double alpha = gamma - mPolarizationAngle;
        // Calculate cosine 2*alpha
        double cosine2alpha = std::cos(2 * alpha);
        // If absolute value of consine 2*alpha is larger than Precision
        float factor(0.);
        if (fabs(cosine2alpha) > mPrecision) {
          factor = static_cast<float>(1. / cosine2alpha);
        }

        // calcalate and set intesity: I *= F
        auto intensity = it->getSignal() * factor;
        it->setSignal(intensity);

        // calculate and set error: Err2∗=F^2
        auto error2 = it->getErrorSquared() * factor * factor;
        // error2 *= factor * factor;
        it->setErrorSquared(error2);
      }
    }
    box->releaseEvents();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  return;
}

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Check input MDEventWorkspace dimension
 * validate dimensions: input workspace is in Q_sample or Q_lab frame, and the 4th dimension is DeltaE,
 * determine Qx and Qz indexes
 * check whether rotational matrix exists in each ExperimentInfo
 * @return
 */
std::string PolarizationAngleCorrectionMD::checkInputMDDimension() {
  std::string errormsg("");

  // Check Q-dimension and determine Qx and Qz index
  API::IMDEventWorkspace_sptr inputws = getProperty("InputWorkspace");
  size_t numdims = inputws->getNumDims();
  std::string qxname("Q_lab_x");
  std::string qzname("Q_lab_z");
  if (numdims < 4) {
    errormsg = "Input workspace must have at least 4 dimensions";
  } else {
    // Get and check the dimensions: Q3D or Q1D
    const Mantid::Kernel::SpecialCoordinateSystem coordsys = inputws->getSpecialCoordinateSystem();
    if (coordsys == Mantid::Kernel::SpecialCoordinateSystem::QLab) {
      mIsQSample = false;
    } else if (coordsys == Mantid::Kernel::SpecialCoordinateSystem::QSample) {
      // q3d
      mIsQSample = true;
      // reset Qx and Qz name
      qxname = "Q_sample_x";
      qzname = "Q_sample_z";
    } else {
      // not supported
      errormsg = "InputWorkspace is not in Q-Sample or Q-lab frame";
    }

    // determine Qx and Qz index
    for (size_t i = 0; i < numdims; ++i) {
      if (inputws->getDimension(i)->getName() == qxname)
        mQxIndex = i;
      else if (inputws->getDimension(i)->getName() == qzname)
        mQzIndex = i;
    }
    // verify and information
    if (mQxIndex != 0 || mQzIndex != 2)
      throw std::runtime_error("Qx, Qy and Qz are not in (Qx, Qy, Qz) order");
    else
      g_log.information() << "Found " << qxname << " at " << mQxIndex << ", " << qzname << " at " << mQzIndex << "\n";

    // Check DeltaE
    if (errormsg.size() > 0 && inputws->getDimension(3)->getName() != "DeltaE") {
      errormsg = "4-th dimension is " + inputws->getDimension(3)->getName() + ".  Must be DeltaE";
      return errormsg;
    }
  }

  // Check rotation matrix
  auto numexpinfo = inputws->getNumExperimentInfo();
  for (uint16_t i = 0; i < numexpinfo; ++i) {
    const Kernel::Matrix<double> &rotmatrix = inputws->getExperimentInfo(i)->run().getGoniometerMatrix();
    mRotationMatrixMap[i] = rotmatrix;
  }

  return errormsg;
}

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Check whether sample log Ei is validad or not
 * Temperature value can be specified by either property Temperature, or
 * it can be calcualted from sample temperture log in the MDWorkspace
 */
std::string PolarizationAngleCorrectionMD::checkEi(const API::IMDEventWorkspace_sptr &mdws) {
  // Get temperture sample log name
  std::string Estring("Ei");
  std::stringstream eiss;

  // the input property could be a valid float; if not must search the experiment info
  uint16_t numexpinfo = mdws->getNumExperimentInfo();

  for (uint16_t i = 0; i < numexpinfo; ++i) {

    // if user specified is not a valid float
    ExperimentInfo_const_sptr expinfo = mdws->getExperimentInfo(i);
    if (expinfo->run().hasProperty(Estring)) {
      std::string eistr = expinfo->run().getProperty(Estring)->value();
      try {
        double ei = boost::lexical_cast<double>(eistr);
        if (ei <= 0) {
          // Ei is not greater than 0 and is not allowed
          eiss << "Experiment Info Ei " << ei << " cannot be zero or less than zero.";
        }
      } catch (...) {
        // unable cast to double
        eiss << "Experiment Info Ei " << eistr << " cannot be cast to a double number";
      }
    } else {
      // does not have Ei
      eiss << "Experiment Info " << i << " does not have " << Estring;
    }
  }

  // return error string
  std::string ei_error = eiss.str();
  return ei_error;
}

} // namespace Mantid::MDAlgorithms
