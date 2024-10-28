// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FindCenterOfMassPosition2.h"
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
#include "MantidKernel/FloatingPointComparison.h"
#include "MantidKernel/PhysicalConstants.h"
namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindCenterOfMassPosition2)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void FindCenterOfMassPosition2::init() {
  const auto wsValidator = std::make_shared<CompositeValidator>();
  const auto positiveDouble = std::make_shared<BoundedValidator<double>>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator));
  declareProperty("Output", "",
                  "TableWorkspace will contain the center of mass position. "
                  "When empty, a CenterOfMass output parameter with two elements (x,y) is created.");

  declareProperty("CenterX", 0.0, "Initial estimate for the beam center in X in meters");
  declareProperty("CenterY", 0.0, "Initial estimate for the beam center in Y in meters");
  declareProperty("Tolerance", 0.00125,
                  "Tolerance on the center of mass position between each iteration in meters. "
                  "Suggested value is the size of a quarter of a pixel.");

  declareProperty("DirectBeam", true,
                  "When true, the calculation will include the pixels within BeamRadius from the beam center. "
                  "Since the process is iterative, the pixels masked by DirectBeam=False will move.");

  positiveDouble->setLower(0);
  declareProperty("BeamRadius", 0.0155, positiveDouble,
                  "Radius of the direct beam area, in meters, used the exclude the beam when calculating "
                  "the center of mass of the scattering pattern. "
                  "This is ignored when DirectBeam=True");
  declareProperty("IntegrationRadius", EMPTY_DBL(), positiveDouble,
                  "Integration radius, in meters, used to include when calculating "
                  "the center of mass of the scattering pattern.");
}

/** Iterates through spectrum in the input workspace finding the center of mass until
 *  we converge to within the tolerance specified in meters
 *
 *  @param inputWS  :: workspace to find the center of mass of
 *  @param centerX  :: save the real center of mass x coord here
 *  @param centerY  :: save the real center of mass y coord here
 *  @param progress :: object for reporting progress of the operation
 */
void FindCenterOfMassPosition2::findCenterOfMass(const API::MatrixWorkspace_sptr &inputWS, double &centerX,
                                                 double &centerY, Progress &progress) {
  const double tolerance = getProperty("Tolerance");
  const bool includeDirectBeam = getProperty("DirectBeam");
  const double beamRadius = getProperty("BeamRadius");
  // when the integration radius isn't set, put the integration radius to 100m which is effectively infinity
  const double integrationRadius = isDefault("IntegrationRadius") ? 100. : getProperty("IntegrationRadius");

  // Define box around center of mass so that only pixels in an area
  // _centered_ on the latest center position are considered. At each
  // iteration we will recompute the bounding box, and we will make
  // it as large as possible. The largest box is defined in:
  WorkspaceBoundingBox boundingBox(inputWS, integrationRadius, beamRadius, !includeDirectBeam, centerX, centerY);

  // Initialize book-keeping
  // distance between previous and current beam center
  double distanceFromPrevious = std::numeric_limits<double>::max();
  // previous distance between previous and current beam center
  double distanceFromPreviousPrevious = std::numeric_limits<double>::max();

  int totalLocalMinima = 0;
  int totalIterations = 0;
  constexpr int LOCAL_MINIMA_MAX{5};

  // Find center of mass and iterate until we converge
  // to within the tolerance specified in meters
  while (distanceFromPrevious > tolerance) {
    // Normalize output to find center of mass position
    // Compute the distance to the previous iteration
    distanceFromPrevious = boundingBox.distanceFromPrevious();

    // skip out early if the center found is within the ignore region
    if (boundingBox.centerOfMassWithinBeamCenter()) {
      g_log.error("Center of mass falls within the beam center area: stopping here");
      break;
    }

    // Recenter around new mass position
    boundingBox.prepareCenterCalculation();

    // Check to see if we have the same result
    // as the previous iteration
    if (Kernel::equals(distanceFromPrevious, distanceFromPreviousPrevious)) {
      totalLocalMinima++;
    } else {
      totalLocalMinima = 0;
    }

    // Quit if we found the exact same distance five times in a row.
    if (totalLocalMinima > LOCAL_MINIMA_MAX) {
      g_log.warning() << "Found the same or equivalent center of mass locations more than " << LOCAL_MINIMA_MAX
                      << " times in a row: stopping here\n";
      break;
    }

    // Quit if we haven't converged after the maximum number of iterations.
    if (++totalIterations > m_maxIteration) {
      g_log.warning() << "More than " << m_maxIteration << " iteration to find beam center: stopping here\n";
      break;
    }

    distanceFromPreviousPrevious = distanceFromPrevious;

    // Count histogram for normalization
    boundingBox.findNewCenterPosition();

    progress.report("Find Beam Center");
  }

  // get the final result
  centerX = boundingBox.getCenterX();
  centerY = boundingBox.getCenterY();
}

/** Package the algorithm outputs one of two ways depending on whether or
 *  not it was given an input EventWorkspace to start with
 *
 *  @param centerX  :: center of mass x coord to package
 *  @param centerY  :: center of mass y coord to package
 */
void FindCenterOfMassPosition2::storeOutputWorkspace(double centerX, double centerY) {
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
    row << "X (m)" << centerX;
    row = m_result->appendRow();
    row << "Y (m)" << centerY;

    setProperty("OutputWorkspace", m_result);
  } else {
    // Store the results using an ArrayProperty
    if (!existsProperty("CenterOfMass"))
      declareProperty(std::make_unique<ArrayProperty<double>>("CenterOfMass", std::make_shared<NullValidator>(),
                                                              Direction::Output));
    std::vector<double> center_of_mass;
    center_of_mass.emplace_back(centerX);
    center_of_mass.emplace_back(centerY);
    setProperty("CenterOfMass", center_of_mass);
  }

  g_log.information() << "Center of Mass found at x=" << centerX << " y=" << centerY << '\n';
}

void FindCenterOfMassPosition2::exec() {
  const MatrixWorkspace_sptr inputWSWvl = getProperty("InputWorkspace");
  double centerX = getProperty("CenterX");
  double centerY = getProperty("CenterY");

  MatrixWorkspace_sptr inputWS;

  // Sum up all the wavelength bins
  IAlgorithm_sptr childAlg = createChildAlgorithm("Integration", 0., 0.5); // first half is integrating
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWSWvl);
  childAlg->executeAsChildAlg();
  inputWS = childAlg->getProperty("OutputWorkspace");

  // Set up the progress reporting object
  Progress progress(this, 0.5, 1.0, m_maxIteration);

  findCenterOfMass(inputWS, centerX, centerY, progress);
  storeOutputWorkspace(centerX, centerY);
}

} // namespace Mantid::Algorithms
