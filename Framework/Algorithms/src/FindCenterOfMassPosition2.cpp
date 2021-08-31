// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FindCenterOfMassPosition2.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindCenterOfMassPosition2)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void FindCenterOfMassPosition2::init() {
  const auto wsValidator = std::make_shared<CompositeValidator>();
  const auto positiveDouble = std::make_shared<BoundedValidator<double>>();

  wsValidator->add<HistogramValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator));
  declareProperty("Output", "",
                  "If not empty, a table workspace of that "
                  "name will contain the center of mass position.");

  declareProperty("CenterX", 0.0, "Estimate for the beam center in X [m]. Default: 0");
  declareProperty("CenterY", 0.0, "Estimate for the beam center in Y [m]. Default: 0");
  declareProperty("Tolerance", 0.00125,
                  "Tolerance on the center of mass "
                  "position between each iteration [m]. "
                  "Default: 0.00125");

  declareProperty("DirectBeam", true,
                  "If true, a direct beam calculation will be performed. Otherwise, the "
                  "center of mass "
                  "of the scattering data will be computed by excluding the beam area.");

  positiveDouble->setLower(0);
  declareProperty("BeamRadius", 0.0155, positiveDouble,
                  "Radius of the beam area, in meters, used the exclude the "
                  "beam when calculating "
                  "the center of mass of the scattering pattern.");
}

API::MatrixWorkspace_sptr
FindCenterOfMassPosition2::sumUsingSpectra(DataObjects::EventWorkspace_const_sptr inputEventWS, const int numSpec,
                                           Progress &progress) {
  std::vector<double> y_values(numSpec);
  std::vector<double> e_values(numSpec);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < numSpec; i++) {
    double sum_i(0), err_i(0);
    progress.report("Integrating events");
    const EventList &el = inputEventWS->getSpectrum(i);
    el.integrate(0, 0, true, sum_i, err_i);
    y_values[i] = sum_i;
    e_values[i] = err_i;
  }

  IAlgorithm_sptr algo = createChildAlgorithm("CreateWorkspace", 0.7, 1.0);
  algo->setProperty<std::vector<double>>("DataX", std::vector<double>(2, 0.0));
  algo->setProperty<std::vector<double>>("DataY", y_values);
  algo->setProperty<std::vector<double>>("DataE", e_values);
  algo->setProperty<int>("NSpec", numSpec);
  algo->execute();

  MatrixWorkspace_sptr inputWS = algo->getProperty("OutputWorkspace");
  return inputWS;
}

double initBoundingBox(WorkspaceBoundingBox &boundingBox, const int numSpec, const double beamRadius,
                       const bool directBeam) {
  double total_count = 0;
  for (int i = 0; i < numSpec; i++) {
    if (!boundingBox.isValidWs(i))
      continue;

    boundingBox.updateMinMax(i);

    if (!boundingBox.isOutOfBoundsOfNonDirectBeam(beamRadius, i, directBeam))
      continue;
    else
      total_count += boundingBox.updatePositionAndReturnCount(i);
  }
  return total_count;
}

double updateBoundingBox(WorkspaceBoundingBox &boundingBox, WorkspaceBoundingBox previousBoundingBox, const int numSpec,
                         const double beamRadius, const bool directBeam) {
  double total_count = 0;
  for (int i = 0; i < numSpec; i++) {
    if (!boundingBox.isValidWs(i))
      continue;

    const V3D position = boundingBox.getWorkspace()->spectrumInfo().position(i);

    if (previousBoundingBox.containsPoint(position.X(), position.Y())) {
      if (!boundingBox.isOutOfBoundsOfNonDirectBeam(beamRadius, i, directBeam))
        continue;
      else
        total_count += boundingBox.updatePositionAndReturnCount(i);
    }
  }
  return total_count;
}

bool equals(double a, double b) { return fabs(a - b) < std::numeric_limits<double>::min(); }

void FindCenterOfMassPosition2::findCenterOfMass(API::MatrixWorkspace_sptr inputWS, double &center_x, double &center_y,
                                                 const int numSpec, Progress &progress) {
  const double tolerance = getProperty("Tolerance");
  const bool directBeam = getProperty("DirectBeam");
  const double beamRadius = getProperty("BeamRadius");

  // Define box around center of mass so that only pixels in an area
  // _centered_ on the latest center position are considered. At each
  // iteration we will recompute the bounding box, and we will make
  // it as large as possible. The largest box is defined in:
  WorkspaceBoundingBox boundingBox(inputWS, g_log);
  boundingBox.setCenter(center_x, center_y);

  // Starting values for the bounding box and the center
  WorkspaceBoundingBox previousBoundingBox(g_log);
  previousBoundingBox.setBounds(0., 0., 0., 0.);

  // Initialize book-keeping
  double distance = -1;
  double distanceCheck = 0;
  double total_count = initBoundingBox(boundingBox, numSpec, beamRadius, directBeam);

  int n_local_minima = 0;
  int n_iteration = 0;

  // Find center of mass and iterate until we converge
  // to within the tolerance specified in meters
  while (distance > tolerance || distance < 0) {
    // Normalize output to find center of mass position
    boundingBox.normalizePosition(total_count, total_count);
    // Compute the distance to the previous iteration
    distance = boundingBox.calculateDistance();
    // Recenter around new mass position
    double radius_x = boundingBox.calculateRadiusX();
    double radius_y = boundingBox.calculateRadiusY();

    if (!directBeam && (radius_x <= beamRadius || radius_y <= beamRadius)) {
      g_log.error() << "Center of mass falls within the beam center area: "
                       "stopping here\n";
      break;
    }

    boundingBox.setCenter(boundingBox.getX(), boundingBox.getY());
    previousBoundingBox.setBounds(boundingBox.getCenterX() - radius_x, boundingBox.getCenterX() + radius_x,
                                  boundingBox.getCenterY() - radius_y, boundingBox.getCenterY() + radius_y);

    // Check to see if we have the same result
    // as the previous iteration
    if (equals(distance, distanceCheck)) {
      n_local_minima++;
    } else {
      n_local_minima = 0;
    }

    // Quit if we found the exact same distance five times in a row.
    if (n_local_minima > 5) {
      g_log.warning() << "Found the same or equivalent center of mass locations "
                         "more than 5 times in a row: stopping here\n";
      break;
    }

    // Quit if we haven't converged after the maximum number of iterations.
    if (++n_iteration > m_maxIteration) {
      g_log.warning() << "More than " << m_maxIteration << " iteration to find beam center: stopping here\n";
      break;
    }

    distanceCheck = distance;

    // Count histogram for normalization
    boundingBox.setPosition(0, 0);
    total_count = updateBoundingBox(boundingBox, previousBoundingBox, numSpec, beamRadius, directBeam);

    progress.report("Find Beam Center");
  }
  center_x = boundingBox.getCenterX();
  center_y = boundingBox.getCenterY();
}

void FindCenterOfMassPosition2::storeOutputWorkspace(double center_x, double center_y) {
  std::string output = getProperty("Output");

  // If an output workspace name was given, create a TableWorkspace with the
  // results,
  // otherwise use an ArrayProperty
  if (!output.empty()) {
    // Store the result in a table workspace
    declareProperty(
        std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("OutputWorkspace", "", Direction::Output));

    // Set the name of the new workspace
    setPropertyValue("OutputWorkspace", output);

    Mantid::API::ITableWorkspace_sptr m_result = std::make_shared<TableWorkspace>();
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
      declareProperty(std::make_unique<ArrayProperty<double>>("CenterOfMass", std::make_shared<NullValidator>(),
                                                              Direction::Output));
    std::vector<double> center_of_mass;
    center_of_mass.emplace_back(center_x);
    center_of_mass.emplace_back(center_y);
    setProperty("CenterOfMass", center_of_mass);
  }

  g_log.information() << "Center of Mass found at x=" << center_x << " y=" << center_y << '\n';
}

void FindCenterOfMassPosition2::exec() {
  const MatrixWorkspace_sptr inputWSWvl = getProperty("InputWorkspace");
  double center_x = getProperty("CenterX");
  double center_y = getProperty("CenterY");

  MatrixWorkspace_sptr inputWS;

  // Get the number of monitors. We assume that all monitors are stored in the
  // first spectra
  const auto numSpec = static_cast<int>(inputWSWvl->getNumberHistograms());
  // Set up the progress reporting object
  Progress progress(this, 0.0, 1.0, m_maxIteration);
  EventWorkspace_const_sptr inputEventWS = std::dynamic_pointer_cast<const EventWorkspace>(inputWSWvl);

  if (inputEventWS) {
    inputWS = sumUsingSpectra(inputEventWS, numSpec, progress);
    WorkspaceFactory::Instance().initializeFromParent(*inputWSWvl, *inputWS, false);
  } else {
    // Sum up all the wavelength bins
    IAlgorithm_sptr childAlg = createChildAlgorithm("Integration");
    childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWSWvl);
    childAlg->executeAsChildAlg();
    inputWS = childAlg->getProperty("OutputWorkspace");
  }

  findCenterOfMass(inputWS, center_x, center_y, numSpec, progress);
  storeOutputWorkspace(center_x, center_y);
}

} // namespace Algorithms
} // namespace Mantid
