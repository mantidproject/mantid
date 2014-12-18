//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindCenterOfMassPosition2.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <iostream>
#include <vector>
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindCenterOfMassPosition2)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void FindCenterOfMassPosition2::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input, wsValidator));
  declareProperty("Output", "",
                  "If not empty, a table workspace of that "
                  "name will contain the center of mass position.");

  declareProperty("CenterX", 0.0,
                  "Estimate for the beam center in X [m]. Default: 0");
  declareProperty("CenterY", 0.0,
                  "Estimate for the beam center in Y [m]. Default: 0");
  declareProperty("Tolerance", 0.00125, "Tolerance on the center of mass "
                                        "position between each iteration [m]. "
                                        "Default: 0.00125");

  declareProperty(
      "DirectBeam", true,
      "If true, a direct beam calculation will be performed. Otherwise, the "
      "center of mass "
      "of the scattering data will be computed by excluding the beam area.");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty("BeamRadius", 0.0155, positiveDouble,
                  "Radius of the beam area, in meters, used the exclude the "
                  "beam when calculating "
                  "the center of mass of the scattering pattern.");
}

void FindCenterOfMassPosition2::exec() {
  MatrixWorkspace_sptr inputWSWvl = getProperty("InputWorkspace");
  MatrixWorkspace_sptr inputWS;

  // Option to exclude beam area
  bool direct_beam = getProperty("DirectBeam");

  // TODO: Need an input for the X bin to use, assume 0 for now
  int specID = 0;
  // Initial center location
  double center_x = getProperty("CenterX");
  double center_y = getProperty("CenterY");
  const double tolerance = getProperty("Tolerance");
  // Iteration cutoff
  int max_iteration = 200;
  // Radius of the beam area, in pixels
  double beam_radius = getProperty("BeamRadius");

  // Get the number of monitors. We assume that all monitors are stored in the
  // first spectra
  const int numSpec = static_cast<int>(inputWSWvl->getNumberHistograms());

  // Set up the progress reporting object
  Progress progress(this, 0.0, 1.0, max_iteration);

  EventWorkspace_const_sptr inputEventWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWSWvl);
  if (inputEventWS) {
    std::vector<double> y_values(numSpec);
    std::vector<double> e_values(numSpec);

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < numSpec; i++) {
      double sum_i(0), err_i(0);
      progress.report("Integrating events");
      const EventList &el = inputEventWS->getEventList(i);
      el.integrate(0, 0, true, sum_i, err_i);
      y_values[i] = sum_i;
      e_values[i] = err_i;
    }

    IAlgorithm_sptr algo = createChildAlgorithm("CreateWorkspace", 0.7, 1.0);
    algo->setProperty<std::vector<double>>("DataX",
                                           std::vector<double>(2, 0.0));
    algo->setProperty<std::vector<double>>("DataY", y_values);
    algo->setProperty<std::vector<double>>("DataE", e_values);
    algo->setProperty<int>("NSpec", numSpec);
    algo->execute();

    inputWS = algo->getProperty("OutputWorkspace");
    WorkspaceFactory::Instance().initializeFromParent(inputWSWvl, inputWS,
                                                      false);
  } else {
    // Sum up all the wavelength bins
    IAlgorithm_sptr childAlg = createChildAlgorithm("Integration");
    childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWSWvl);
    childAlg->executeAsChildAlg();
    inputWS = childAlg->getProperty("OutputWorkspace");
  }

  // Define box around center of mass so that only pixels in an area
  // _centered_ on the latest center position are considered. At each
  // iteration we will recompute the bounding box, and we will make
  // it as large as possible. The largest box is defined as:
  double xmin0 = 0;
  double xmax0 = 0;
  double ymin0 = 0;
  double ymax0 = 0;

  // Starting values for the bounding box and the center
  double xmin = xmin0;
  double xmax = xmax0;
  double ymin = ymin0;
  double ymax = ymax0;

  // Initialize book-keeping
  double distance = -1;
  double distance_check = 0;
  int n_local_minima = 0;
  int n_iteration = 0;

  // Find center of mass and iterate until we converge
  // to within a quarter of a pixel
  bool first_run = true;
  while (distance > tolerance || distance < 0) {
    // Count histogram for normalization
    double total_count = 0;
    double position_x = 0;
    double position_y = 0;

    for (int i = 0; i < numSpec; i++) {
      // Get the pixel relating to this spectrum
      IDetector_const_sptr det;
      try {
        det = inputWS->getDetector(i);
      } catch (Exception::NotFoundError &) {
        g_log.warning() << "Spectrum index " << i
                        << " has no detector assigned to it - discarding"
                        << std::endl;
        continue;
      }
      // If this detector is masked, skip to the next one
      if (det->isMasked())
        continue;
      // If this detector is a monitor, skip to the next one
      if (det->isMonitor())
        continue;

      // Get the current spectrum
      const MantidVec &YIn = inputWS->readY(i);
      const V3D pos = det->getPos();
      double x = pos.X();
      double y = pos.Y();

      if (first_run) {
        xmin0 = std::min(x, xmin0);
        xmax0 = std::max(x, xmax0);
        ymin0 = std::min(y, ymin0);
        ymax0 = std::max(y, ymax0);
      }

      if (first_run || (x >= xmin && x <= xmax && y >= ymin && y <= ymax)) {
        if (!direct_beam) {
          double dx = x - center_x;
          double dy = y - center_y;
          if (dx * dx + dy * dy < beam_radius * beam_radius)
            continue;
        }
        position_x += YIn[specID] * x;
        position_y += YIn[specID] * y;
        total_count += YIn[specID];
      }
    }

    // Normalize output to find center of mass position
    position_x /= total_count;
    position_y /= total_count;

    // Compute the distance to the previous iteration
    distance = sqrt((center_x - position_x) * (center_x - position_x) +
                    (center_y - position_y) * (center_y - position_y));

    // Modify the bounding box around the detector region used to
    // compute the center of mass so that it is centered around
    // the new center of mass position.
    double radius_x = std::min((position_x - xmin0), (xmax0 - position_x));
    double radius_y = std::min((position_y - ymin0), (ymax0 - position_y));

    if (!direct_beam && (radius_x <= beam_radius || radius_y <= beam_radius)) {
      g_log.error()
          << "Center of mass falls within the beam center area: stopping here"
          << std::endl;
      break;
    }

    center_x = position_x;
    center_y = position_y;

    xmin = center_x - radius_x;
    xmax = center_x + radius_x;
    ymin = center_y - radius_y;
    ymax = center_y + radius_y;

    // Sanity check to avoid getting stuck
    if (distance == distance_check) {
      n_local_minima++;
    } else {
      n_local_minima = 0;
    }

    // Quit if we found the exact same distance five times in a row.
    if (n_local_minima > 5) {
      g_log.warning()
          << "Found the same or equivalent center of mass locations "
             "more than 5 times in a row: stopping here" << std::endl;
      break;
    }

    // Quit if we haven't converged after the maximum number of iterations.
    if (++n_iteration > max_iteration) {
      g_log.warning() << "More than " << max_iteration
                      << " iteration to find beam center: stopping here"
                      << std::endl;
      break;
    }

    distance_check = distance;
    first_run = false;

    progress.report("Find Beam Center");
  }

  std::string output = getProperty("Output");

  // If an output workspace name was given, create a TableWorkspace with the
  // results,
  // otherwise use an ArrayProperty
  if (!output.empty()) {
    // Store the result in a table workspace
    declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
        "OutputWorkspace", "", Direction::Output));

    // Set the name of the new workspace
    setPropertyValue("OutputWorkspace", output);

    Mantid::API::ITableWorkspace_sptr m_result =
        Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    m_result->addColumn("str", "Name");
    m_result->addColumn("double", "Value");

    Mantid::API::TableRow row = m_result->appendRow();
    row << "X (m)" << center_x;
    row = m_result->appendRow();
    row << "Y (m)" << center_y;

    setProperty("OutputWorkspace", m_result);
  } else {
    // Store the results using an ArrayProperty
    if (!existsProperty("CenterOfMass"))
      declareProperty(new ArrayProperty<double>(
          "CenterOfMass", boost::make_shared<NullValidator>(),
          Direction::Output));
    std::vector<double> center_of_mass;
    center_of_mass.push_back(center_x);
    center_of_mass.push_back(center_y);
    setProperty("CenterOfMass", center_of_mass);
  }

  g_log.information() << "Center of Mass found at x=" << center_x
                      << " y=" << center_y << std::endl;
}

} // namespace Algorithms
} // namespace Mantid
