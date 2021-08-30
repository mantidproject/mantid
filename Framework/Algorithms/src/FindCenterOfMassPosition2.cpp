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

struct BoundingBox {
  API::MatrixWorkspace_sptr workspace;
  double position_x = 0;
  double position_y = 0;
  double *center_x;
  double *center_y;
  double xmin = 0;
  double xmax = 0;
  double ymin = 0;
  double ymax = 0;
}

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

// TODO: Break this down further, still feel like this is too large?
void FindCenterOfMassPosition2::findCenterOfMass(API::MatrixWorkspace_sptr inputWS, double &center_x, double &center_y,
                                                 const int numSpec, Progress &progress) {
  const double tolerance = getProperty("Tolerance");
  const bool direct_beam = getProperty("DirectBeam");
  const double beam_radius = getProperty("BeamRadius");
  
  const auto &spectrumInfo = inputWS->spectrumInfo();
  const int indexFirstValidWs = findFirstValidWs(spectrumInfo, numSpec);
  // Define box around center of mass so that only pixels in an area
  // _centered_ on the latest center position are considered. At each
  // iteration we will recompute the bounding box, and we will make
  // it as large as possible. The largest box is defined as:
  WorkspaceBoundingBox boundingBox(inputWs);
  boundingBox.setCenter(center_x, center_y);

  // Starting values for the bounding box and the center
  //NOTE only ever used for checks and set at the end
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
  // to within the tolerance specified in meters
  bool first_run = true;
  while (distance > tolerance || distance < 0) {
    // Count histogram for normalization
    double total_count = 0;
    boundingBox.position_x = 0;
    boundingBox.position_y = 0;


    //NOOOOOOOOOOOOTE:  MOVE THIS TO THE END AND DO THE FIRST RUN OUT OF THE LOOP SO WE DONT NEED A FIRST RUN FLAG

    for (int i = indexFirstValidWs; i < numSpec; i++) {
      if(!isValidWs(spectrumInfo, i))
        continue;
      // Get the current spectrum
      auto &YIn = inputWS->y(i);

      const V3D pos = spectrumInfo.position(i);
      double x = pos.X();
      double y = pos.Y();

      if(first_run) {
        updateMinMax(boundingBox, i);
      }

      // TODO: Separated dense conditonal out into legible function or variable
      if (first_run || (x >= xmin && x <= xmax && y >= ymin && y <= ymax)) {
        if (isOutOfBoundsOfNonDirectBeam(boundingBox, beam_radius, i, direct_beam))
          continue;
        else
          updatePositionAndCount(boundingBox, total_count, i);
      }
    }

    // Normalize output to find center of mass position
    boundingBox.position_x /= total_count;
    boundingBox.position_y /= total_count;

    // Compute the distance to the previous iteration
    distance =
        sqrt((boundingBox.center_x - boundingBox.position_x) * (boundingBox.center_x - boundingBox.position_x) + (boundingBox.center_y - boundingBox.position_y) * (boundingBox.center_y - boundingBox.position_y));

    // Modify the bounding box around the detector region used to
    // compute the center of mass so that it is centered around
    // the new center of mass position.
    double radius_x = std::min((boundingBox.position_x - boundingBox.xmin0), (boundingBox.xmax0 - boundingBox.position_x));
    double radius_y = std::min((boundingBox.position_y - boundingBox.ymin0), (boundingBox.ymax0 - boundingBox.position_y));

    // TODO: This apparently shouldnt be an issue, might break the scattering pattern???
    if (!direct_beam && (radius_x <= beam_radius || radius_y <= beam_radius)) {
      g_log.error() << "Center of mass falls within the beam center area: "
                       "stopping here\n";
      break;
    }

    // TODO: Is this fine or not, do I need a conditional or do you not actually care if this is applied regardless of
    // method?
    boundingBox.center_x = boundingBox.position_x;
    boundingBox.center_y = boundingBox.position_y;

    xmin = boundingBox.center_x - radius_x;
    xmax = boundingBox.center_x + radius_x;
    ymin = boundingBox.center_y - radius_y;
    ymax = boundingBox.center_y + radius_y;

    // Check to see if we have the same result
    // as the previous iteration

    // TODO: double compare should not be done with ==
    if (distance == distance_check) {
      n_local_minima++;
    } else {
      n_local_minima = 0;
    }

    // PERSONAL NOTE:  This doesnt smell right, there has to be a better way to tell that you are stuck than to just
    // quit if you hit the same thing some arbitrary number of times, masks the issue really

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

    distance_check = distance;
    first_run = false;

    progress.report("Find Beam Center");
  }
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
  } else {/diffraction/-/boards?scope=all&assignee_username=wqp
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
