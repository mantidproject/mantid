#include "MantidAlgorithms/FindCenterOfMassPosition.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindCenterOfMassPosition)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void FindCenterOfMassPosition::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  declareProperty(make_unique<WorkspaceProperty<>>(
      "InputWorkspace", "", Direction::Input, wsValidator));
  declareProperty("Output", "",
                  "If not empty, a table workspace of that "
                  "name will contain the center of mass position.");

  auto positiveInt = boost::make_shared<BoundedValidator<int>>();
  positiveInt->setLower(0);
  declareProperty("NPixelX", 192, positiveInt,
                  "Number of detector pixels in the X direction.");

  positiveInt->setLower(0);
  declareProperty("NPixelY", 192, positiveInt,
                  "Number of detector pixels in the Y direction.");

  declareProperty(
      "DirectBeam", true,
      "If true, a direct beam calculation will be performed. Otherwise, the "
      "center of mass "
      "of the scattering data will be computed by excluding the beam area.");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty("BeamRadius", 20.0, positiveDouble,
                  "Radius of the beam area, in pixels, used the exclude the "
                  "beam when calculating "
                  "the center of mass of the scattering pattern.");
}

void FindCenterOfMassPosition::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Option to exclude beam area
  bool direct_beam = getProperty("DirectBeam");

  // TODO: Need an input for the X bin to use, assume 0 for now
  int specID = 0;
  // Need input for detector dimensions
  int n_pixel_x = getProperty("NPixelX");
  int n_pixel_y = getProperty("NPixelY");
  // Iteration cutoff
  int max_iteration = 200;
  // Radius of the beam area, in pixels
  double beam_radius = getProperty("BeamRadius");

  // Set up the progress reporting object
  Progress progress(this, 0.0, 1.0, max_iteration);

  // Define box around center of mass so that only pixels in an area
  // _centered_ on the latest center position are considered. At each
  // iteration we will recompute the bounding box, and we will make
  // it as large as possible. The largest box is defined as:
  double xmin0 = 1.0;
  double xmax0 = n_pixel_x - 2.0;
  double ymin0 = 1.0;
  double ymax0 = n_pixel_y - 2.0;

  // Starting values for the bounding box and the center
  double xmin = xmin0;
  double xmax = xmax0;
  double ymin = ymin0;
  double ymax = ymax0;
  double center_x = n_pixel_x / 2.0;
  double center_y = n_pixel_y / 2.0;

  // Initialize book-keeping
  double distance = -1;
  double distance_check = 0;
  int n_local_minima = 0;
  int n_iteration = 0;

  // Get the number of monitors. We assume that all monitors are stored in the
  // first spectra
  int n_monitors =
      static_cast<int>(inputWS->getInstrument()->getMonitors().size());
  const int numSpec = static_cast<int>(inputWS->getNumberHistograms());

  // Find center of mass and iterate until we converge
  // to within a quarter of a pixel
  while (distance > 0.25 || distance < 0) {
    // Count histogram for normalization
    double total_count = 0;
    double position_x = 0;
    double position_y = 0;

    const auto &spectrumInfo = inputWS->spectrumInfo();
    for (int i = 0; i < numSpec; i++) {
      if (!spectrumInfo.hasDetectors(i)) {
        g_log.warning() << "Workspace index " << i
                        << " has no detector assigned to it - discarding\n";
        continue;
      }
      // Skip if we have a monitor or if the detector is masked.
      if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
        continue;

      // Get the current spectrum
      const MantidVec &YIn = inputWS->readY(i);
      double y = static_cast<double>((i - n_monitors) % n_pixel_x);
      double x = floor(static_cast<double>(i - n_monitors) / n_pixel_y);

      if (x >= xmin && x <= xmax && y >= ymin && y <= ymax) {
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
      g_log.error() << "Center of mass falls within the beam center area: "
                       "stopping here\n";
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
             "more than 5 times in a row: stopping here\n";
      break;
    }

    // Quit if we haven't converged after the maximum number of iterations.
    if (++n_iteration > max_iteration) {
      g_log.warning() << "More than " << max_iteration
                      << " iteration to find beam center: stopping here\n";
      break;
    }

    distance_check = distance;

    progress.report();
  }

  std::string output = getProperty("Output");

  // If an output workspace name was given, create a TableWorkspace with the
  // results,
  // otherwise use an ArrayProperty
  if (!output.empty()) {
    // Store the result in a table workspace
    declareProperty(make_unique<WorkspaceProperty<API::ITableWorkspace>>(
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
    declareProperty(make_unique<ArrayProperty<double>>(
        "CenterOfMass", boost::make_shared<NullValidator>(),
        Direction::Output));
    std::vector<double> center_of_mass;
    center_of_mass.push_back(center_x);
    center_of_mass.push_back(center_y);
    setProperty("CenterOfMass", center_of_mass);
  }

  g_log.information() << "Center of Mass found at x=" << center_x
                      << " y=" << center_y << '\n';
}

} // namespace Algorithms
} // namespace Mantid
