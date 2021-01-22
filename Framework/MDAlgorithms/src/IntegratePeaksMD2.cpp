// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/IntegratePeaksMD2.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidMDAlgorithms/GSLFunctions.h"
#include "MantidMDAlgorithms/MDBoxMaskFunction.h"

#include <cmath>
#include <fstream>
#include <gsl/gsl_integration.h>

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksMD2)

using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/** Initialize the algorithm's properties.
 */
void IntegratePeaksMD2::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input MDEventWorkspace.");

  auto radiiValidator = std::make_shared<ArrayBoundedValidator<double>>();
  radiiValidator->setLower(0.0);
  radiiValidator->setLowerExclusive(true);
  declareProperty(
      std::make_unique<ArrayProperty<double>>("PeakRadius",
                                              std::vector<double>({1.0}),
                                              radiiValidator, Direction::Input),
      "Fixed radius around each peak position in which to integrate, or the "
      "semi-axis lengths (a,b,c) describing an ellipsoid shape used for "
      "integration (in the same units as the workspace).");

  radiiValidator->setLowerExclusive(false);
  declareProperty(
      std::make_unique<ArrayProperty<double>>("BackgroundInnerRadius",
                                              std::vector<double>({0.0}),
                                              radiiValidator, Direction::Input),
      "Inner radius, or three values for semi-axis lengths (a,b,c) of the "
      "ellipsoid shape, used to evaluate the background of the peak.\n"
      "If smaller than PeakRadius, then we assume BackgroundInnerRadius = "
      "PeakRadius.");

  declareProperty(
      std::make_unique<ArrayProperty<double>>("BackgroundOuterRadius",
                                              std::vector<double>({0.0}),
                                              radiiValidator, Direction::Input),
      "Outer radius, or three values for semi-axis lengths (a,b,c) of the "
      "ellipsoid shape, to use to evaluate the background of the peak.\n"
      "The signal density around the peak (BackgroundInnerRadius < r < "
      "BackgroundOuterRadius) is used to estimate the background under the "
      "peak.\n"
      "If smaller than PeakRadius, no background measurement is done.");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::Input),
                  "A PeaksWorkspace containing the peaks to integrate.");

  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "",
                                                          Direction::Output),
      "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
      "with the peaks' integrated intensities.");

  declareProperty("ReplaceIntensity", true,
                  "Always replace intensity in PeaksWorkspacem (default).\n"
                  "If false, then do not replace intensity if calculated value "
                  "is 0 (used for SNSSingleCrystalReduction)");

  declareProperty(
      "IntegrateIfOnEdge", true,
      "Only warning if all of peak outer radius is not on detector (default).\n"
      "If false, do not integrate if the outer radius is not on a detector.");

  declareProperty("AdaptiveQBackground", false,
                  "Default is false.   If true, "
                  "BackgroundOuterRadius + AdaptiveQMultiplier * **|Q|** and "
                  "BackgroundInnerRadius + AdaptiveQMultiplier * **|Q|**");

  declareProperty("Ellipsoid", false, "Default is sphere.");
  declareProperty("FixQAxis", false, "Default is sphere.");

  declareProperty("Cylinder", false,
                  "Default is sphere.  Use next five parameters for cylinder.");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>("CylinderLength", 0.0,
                                                  Direction::Input),
      "Length of cylinder in which to integrate (in the same units as the "
      "workspace).");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "PercentBackground", 0.0, Direction::Input),
                  "Percent of CylinderLength that is background (20 is 20%)");

  std::vector<std::string> peakNames =
      FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
  peakNames.emplace_back("NoFit");
  declareProperty("ProfileFunction", "Gaussian",
                  std::make_shared<StringListValidator>(peakNames),
                  "Fitting function for profile that is used only with "
                  "Cylinder integration.");

  std::vector<std::string> integrationOptions(2);
  integrationOptions[0] = "Sum";
  integrationOptions[1] = "GaussianQuadrature";
  auto integrationvalidator =
      std::make_shared<StringListValidator>(integrationOptions);
  declareProperty("IntegrationOption", "GaussianQuadrature",
                  integrationvalidator,
                  "Integration method for calculating intensity "
                  "used only with Cylinder integration.");

  declareProperty(
      std::make_unique<FileProperty>("ProfilesFile", "",
                                     FileProperty::OptionalSave,
                                     std::vector<std::string>(1, "profiles")),
      "Save (Optionally) as Isaw peaks file with profiles included");

  declareProperty("AdaptiveQMultiplier", 0.0,
                  "PeakRadius + AdaptiveQMultiplier * **|Q|** "
                  "so each peak has a "
                  "different integration radius.  Q includes the 2*pi factor.");

  declareProperty(
      "CorrectIfOnEdge", false,
      "Only warning if all of peak outer radius is not on detector (default).\n"
      "If false, correct for volume off edge for both background and "
      "intensity.");

  declareProperty("UseOnePercentBackgroundCorrection", true,
                  "If this options is enabled, then the the top 1% of the "
                  "background will be removed"
                  "before the background subtraction.");
}

std::map<std::string, std::string> IntegratePeaksMD2::validateInputs() {
  std::map<std::string, std::string> result;

  std::vector<double> PeakRadius = getProperty("PeakRadius");
  std::vector<double> BackgroundInnerRadius =
      getProperty("BackgroundInnerRadius");
  std::vector<double> BackgroundOuterRadius =
      getProperty("BackgroundOuterRadius");
  bool ellipsoid = getProperty("Ellipsoid");

  if (PeakRadius.size() != 1 && PeakRadius.size() != 3) {
    std::stringstream errmsg;
    errmsg << "Only one or three values should be specified";
    result["PeakRadius"] = errmsg.str();
  }

  if (!ellipsoid && PeakRadius.size() != 1) {
    std::stringstream errmsg;
    errmsg << "One value must be specified when Ellipsoid is false";
    result["PeakRadius"] = errmsg.str();
  }

  if (BackgroundInnerRadius.size() != 1 && BackgroundInnerRadius.size() != 3) {
    std::stringstream errmsg;
    errmsg << "Only one or three values should be specified";
    result["BackgroundInnerRadius"] = errmsg.str();
  }

  if (!ellipsoid && BackgroundInnerRadius.size() != 1) {
    std::stringstream errmsg;
    errmsg << "One value must be specified when Ellipsoid is false";
    result["BackgroundInnerRadius"] = errmsg.str();
  }

  if (BackgroundOuterRadius.size() != 1 && BackgroundOuterRadius.size() != 3) {
    std::stringstream errmsg;
    errmsg << "Only one or three values should be specified";
    result["BackgroundOuterRadius"] = errmsg.str();
  }

  if (!ellipsoid && BackgroundOuterRadius.size() != 1) {
    std::stringstream errmsg;
    errmsg << "One value must be specified when Ellipsoid is false";
    result["BackgroundOuterRadius"] = errmsg.str();
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Integrate the peaks of the workspace using parameters saved in the algorithm
 * class
 * @param ws ::  MDEventWorkspace to integrate
 */
template <typename MDE, size_t nd>
void IntegratePeaksMD2::integrate(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  if (nd != 3)
    throw std::invalid_argument("For now, we expect the input MDEventWorkspace "
                                "to have 3 dimensions only.");

  /// Peak workspace to integrate
  Mantid::DataObjects::PeaksWorkspace_sptr inPeakWS =
      getProperty("PeaksWorkspace");

  /// Output peaks workspace, create if needed
  Mantid::DataObjects::PeaksWorkspace_sptr peakWS =
      getProperty("OutputWorkspace");
  if (peakWS != inPeakWS)
    peakWS = inPeakWS->clone();
  // This only fails in the unit tests which say that MaskBTP is not registered
  try {
    runMaskDetectors(inPeakWS, "Tube", "edges");
    runMaskDetectors(inPeakWS, "Pixel", "edges");
  } catch (...) {
    g_log.error("Can't execute MaskBTP algorithm for this instrument to set "
                "edge for IntegrateIfOnEdge option");
  }

  calculateE1(inPeakWS->detectorInfo()); // fill E1Vec for use in detectorQ
  Mantid::Kernel::SpecialCoordinateSystem CoordinatesToUse =
      ws->getSpecialCoordinateSystem();

  /// Radius to use around peaks
  std::vector<double> PeakRadius = getProperty("PeakRadius");
  /// Background (end) radius
  std::vector<double> BackgroundOuterRadius =
      getProperty("BackgroundOuterRadius");
  /// Start radius of the background
  std::vector<double> BackgroundInnerRadius =
      getProperty("BackgroundInnerRadius");
  /// One percent background correction
  bool useOnePercentBackgroundCorrection =
      getProperty("UseOnePercentBackgroundCorrection");

  bool manualEllip = false;
  if (PeakRadius.size() > 1) {
    manualEllip = true;
  }

  double minInnerRadius = PeakRadius[0];
  for (size_t r = 0; r < BackgroundInnerRadius.size(); r++) {
    if (manualEllip) {
      minInnerRadius = PeakRadius[r];
    }
    if (BackgroundInnerRadius[r] < minInnerRadius)
      BackgroundInnerRadius[r] = minInnerRadius;
  }
  // Ellipsoid
  bool isEllipse = getProperty("Ellipsoid");
  bool qAxisIsFixed = getProperty("FixQAxis");
  /// Cylinder Length to use around peaks for cylinder
  double cylinderLength = getProperty("CylinderLength");
  Workspace2D_sptr wsProfile2D, wsFit2D, wsDiff2D;
  size_t numSteps = 0;
  bool cylinderBool = getProperty("Cylinder");
  bool adaptiveQBackground = getProperty("AdaptiveQBackground");
  double adaptiveQMultiplier = getProperty("AdaptiveQMultiplier");
  double adaptiveQBackgroundMultiplier = 0.0;
  if (adaptiveQBackground)
    adaptiveQBackgroundMultiplier = adaptiveQMultiplier;
  std::vector<double> PeakRadiusVector(peakWS->getNumberPeaks(), PeakRadius[0]);
  std::vector<double> BackgroundInnerRadiusVector(peakWS->getNumberPeaks(),
                                                  BackgroundInnerRadius[0]);
  std::vector<double> BackgroundOuterRadiusVector(peakWS->getNumberPeaks(),
                                                  BackgroundOuterRadius[0]);
  if (cylinderBool) {
    numSteps = 100;
    size_t histogramNumber = peakWS->getNumberPeaks();
    Workspace_sptr wsProfile = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, numSteps, numSteps);
    wsProfile2D = std::dynamic_pointer_cast<Workspace2D>(wsProfile);
    AnalysisDataService::Instance().addOrReplace("ProfilesData", wsProfile2D);
    Workspace_sptr wsFit = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, numSteps, numSteps);
    wsFit2D = std::dynamic_pointer_cast<Workspace2D>(wsFit);
    AnalysisDataService::Instance().addOrReplace("ProfilesFit", wsFit2D);
    Workspace_sptr wsDiff = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, numSteps, numSteps);
    wsDiff2D = std::dynamic_pointer_cast<Workspace2D>(wsDiff);
    AnalysisDataService::Instance().addOrReplace("ProfilesFitDiff", wsDiff2D);
    auto newAxis1 = std::make_unique<TextAxis>(peakWS->getNumberPeaks());
    auto newAxis2 = std::make_unique<TextAxis>(peakWS->getNumberPeaks());
    auto newAxis3 = std::make_unique<TextAxis>(peakWS->getNumberPeaks());
    auto newAxis1Raw = newAxis1.get();
    auto newAxis2Raw = newAxis2.get();
    auto newAxis3Raw = newAxis3.get();
    wsProfile2D->replaceAxis(1, std::move(newAxis1));
    wsFit2D->replaceAxis(1, std::move(newAxis2));
    wsDiff2D->replaceAxis(1, std::move(newAxis3));
    for (int i = 0; i < peakWS->getNumberPeaks(); ++i) {
      // Get a direct ref to that peak.
      IPeak &p = peakWS->getPeak(i);
      std::ostringstream label;
      label << Utils::round(p.getH()) << "_" << Utils::round(p.getK()) << "_"
            << Utils::round(p.getL()) << "_" << p.getRunNumber();
      newAxis1Raw->setLabel(i, label.str());
      newAxis2Raw->setLabel(i, label.str());
      newAxis3Raw->setLabel(i, label.str());
    }
  }
  double percentBackground = getProperty("PercentBackground");
  size_t peakMin = 0;
  size_t peakMax = numSteps;
  double ratio = 0.0;
  if (cylinderBool) {
    peakMin = static_cast<size_t>(static_cast<double>(numSteps) *
                                  percentBackground / 100.);
    peakMax = numSteps - peakMin - 1;
    size_t numPeakCh = peakMax - peakMin + 1; // number of peak channels
    size_t numBkgCh = numSteps - numPeakCh;   // number of background channels
    ratio = static_cast<double>(numPeakCh) / static_cast<double>(numBkgCh);
  }
  /// Replace intensity with 0
  bool replaceIntensity = getProperty("ReplaceIntensity");
  bool integrateEdge = getProperty("IntegrateIfOnEdge");
  bool correctEdge = getProperty("CorrectIfOnEdge");

  std::string profileFunction = getProperty("ProfileFunction");
  std::string integrationOption = getProperty("IntegrationOption");
  std::ofstream out;
  if (cylinderBool && profileFunction != "NoFit") {
    std::string outFile = getProperty("InputWorkspace");
    outFile.append(profileFunction);
    outFile.append(".dat");
    std::string save_path =
        ConfigService::Instance().getString("defaultsave.directory");
    outFile = save_path + outFile;
    out.open(outFile.c_str(), std::ofstream::out);
  }
  // volume of Background sphere with inner volume subtracted
  double volumeBkg = 4.0 / 3.0 * M_PI *
                     (std::pow(BackgroundOuterRadius[0], 3) -
                      std::pow(BackgroundOuterRadius[0], 3));
  // volume of PeakRadius sphere
  double volumeRadius = 4.0 / 3.0 * M_PI * std::pow(PeakRadius[0], 3);
  //
  // If the following OMP pragma is included, this algorithm seg faults
  // sporadically when processing multiple TOPAZ runs in a script, on
  // Scientific Linux 6.2.  Typically, it seg faults after 2 to 6 runs are
  // processed, though occasionally it will process all 8 requested in the
  // script without crashing.  Since the lower level codes already use OpenMP,
  // parallelizing at this level is only marginally useful, giving about a
  // 5-10% speedup.  Perhaps is should just be removed permanantly, but for
  // now it is commented out to avoid the seg faults.  Refs #5533
  // PRAGMA_OMP(parallel for schedule(dynamic, 10) )
  // Initialize progress reporting
  int nPeaks = peakWS->getNumberPeaks();
  Progress progress(this, 0., 1., nPeaks);
  for (int i = 0; i < nPeaks; ++i) {
    if (this->getCancel())
      break; // User cancellation
    progress.report();

    // Get a direct ref to that peak.
    IPeak &p = peakWS->getPeak(i);

    // Get the peak center as a position in the dimensions of the workspace
    V3D pos;
    if (CoordinatesToUse == Mantid::Kernel::QLab) //"Q (lab frame)"
      pos = p.getQLabFrame();
    else if (CoordinatesToUse == Mantid::Kernel::QSample) //"Q (sample frame)"
      pos = p.getQSampleFrame();
    else if (CoordinatesToUse == Mantid::Kernel::HKL) //"HKL"
      pos = p.getHKL();

    // Do not integrate if sphere is off edge of detector

    double edge = detectorQ(p.getQLabFrame(),
                            std::max(BackgroundOuterRadius[0], PeakRadius[0]));
    if (edge < std::max(BackgroundOuterRadius[0], PeakRadius[0])) {
      g_log.warning() << "Warning: sphere/cylinder for integration is off edge "
                         "of detector for peak "
                      << i << "; radius of edge =  " << edge << '\n';
      if (!integrateEdge) {
        if (replaceIntensity) {
          p.setIntensity(0.0);
          p.setSigmaIntensity(0.0);
        }
        continue;
      }
    }

    // Build the sphere transformation
    bool dimensionsUsed[nd];
    coord_t center[nd];
    for (size_t d = 0; d < nd; ++d) {
      dimensionsUsed[d] = true; // Use all dimensions
      center[d] = static_cast<coord_t>(pos[d]);
    }
    signal_t signal = 0;
    signal_t errorSquared = 0;
    signal_t bgSignal = 0;
    signal_t bgErrorSquared = 0;
    double background_total = 0.0;
    if (!cylinderBool) {
      // modulus of Q
      coord_t lenQpeak = 0.0;
      if (adaptiveQMultiplier != 0.0) {
        lenQpeak = 0.0;
        for (size_t d = 0; d < nd; ++d) {
          lenQpeak += center[d] * center[d];
        }
        lenQpeak = std::sqrt(lenQpeak);
      }
      double adaptiveRadius =
          adaptiveQMultiplier * lenQpeak +
          *std::max_element(PeakRadius.begin(), PeakRadius.end());
      if (adaptiveRadius <= 0.0) {
        g_log.error() << "Error: Radius for integration sphere of peak " << i
                      << " is negative =  " << adaptiveRadius << '\n';
        adaptiveRadius = 0.;
        p.setIntensity(0.0);
        p.setSigmaIntensity(0.0);
        PeakRadiusVector[i] = 0.0;
        BackgroundInnerRadiusVector[i] = 0.0;
        BackgroundOuterRadiusVector[i] = 0.0;
        continue;
      }
      PeakRadiusVector[i] = adaptiveRadius;
      BackgroundInnerRadiusVector[i] =
          adaptiveQBackgroundMultiplier * lenQpeak +
          *std::max_element(BackgroundInnerRadius.begin(),
                            BackgroundInnerRadius.end());
      BackgroundOuterRadiusVector[i] =
          adaptiveQBackgroundMultiplier * lenQpeak +
          *std::max_element(BackgroundOuterRadius.begin(),
                            BackgroundOuterRadius.end());
      // define the radius squared for a sphere intially
      CoordTransformDistance getRadiusSq(nd, center, dimensionsUsed);
      // set spherical shape
      if (auto *shapeablePeak = dynamic_cast<Peak *>(&p)) {
        PeakShape *sphereShape = new PeakShapeSpherical(
            PeakRadiusVector[i], BackgroundInnerRadiusVector[i],
            BackgroundOuterRadiusVector[i], CoordinatesToUse, this->name(),
            this->version());
        shapeablePeak->setPeakShape(sphereShape);
      }
      const double scaleFactor = pow(PeakRadiusVector[i], 3) /
                                 (pow(BackgroundOuterRadiusVector[i], 3) -
                                  pow(BackgroundInnerRadiusVector[i], 3));
      // Integrate spherical background shell if specified
      if (BackgroundOuterRadius[0] > PeakRadius[0]) {
        // Get the total signal inside background shell
        ws->getBox()->integrateSphere(
            getRadiusSq,
            static_cast<coord_t>(pow(BackgroundOuterRadiusVector[i], 2)),
            bgSignal, bgErrorSquared,
            static_cast<coord_t>(pow(BackgroundInnerRadiusVector[i], 2)),
            useOnePercentBackgroundCorrection);
        // correct bg signal by Vpeak/Vshell (same for sphere and ellipse)
        bgSignal *= scaleFactor;
        bgErrorSquared *= scaleFactor * scaleFactor;
      }
      // if ellipsoid find covariance and centroid in spherical region
      // using one-pass algorithm from https://doi.org/10.1145/359146.359153
      if (isEllipse) {
        // flat bg to subtract
        const auto bgDensity =
            bgSignal / (4 * M_PI * pow(PeakRadiusVector[i], 3) / 3);
        std::vector<V3D> eigenvects;
        std::vector<double> eigenvals;
        if (PeakRadius.size() == 1) {
          findEllipsoid<MDE, nd>(
              ws, getRadiusSq, pos,
              static_cast<coord_t>(pow(PeakRadiusVector[i], 2)), qAxisIsFixed,
              bgDensity, eigenvects, eigenvals);
        } else {
          // Use the manually specified radii instead of finding them via
          // findEllipsoid
          eigenvals = PeakRadius;
          eigenvects.push_back(V3D(1.0, 0.0, 0.0));
          eigenvects.push_back(V3D(0.0, 1.0, 0.0));
          eigenvects.push_back(V3D(0.0, 0.0, 1.0));
        }

        // transform ellispoid onto sphere of radius = R
        getRadiusSq =
            CoordTransformDistance(nd, center, dimensionsUsed, 1, /* outD */
                                   eigenvects, eigenvals);
        // Integrate ellipsoid background shell if specified
        if (BackgroundOuterRadius[0] > PeakRadius[0]) {
          // Get the total signal inside "BackgroundOuterRadius"
          bgSignal = 0;
          bgErrorSquared = 0;
          ws->getBox()->integrateSphere(
              getRadiusSq,
              static_cast<coord_t>(pow(BackgroundOuterRadiusVector[i], 2)),
              bgSignal, bgErrorSquared,
              static_cast<coord_t>(pow(BackgroundInnerRadiusVector[i], 2)),
              useOnePercentBackgroundCorrection);
          // correct bg signal by Vpeak/Vshell (same as previously
          // calculated for sphere)
          bgSignal *= scaleFactor;
          bgErrorSquared *= scaleFactor * scaleFactor;
        }
        // set peak shape
        if (auto *shapeablePeak = dynamic_cast<Peak *>(&p)) {
          // get radii in same proprtion as eigenvalues
          auto max_stdev =
              pow(*std::max_element(eigenvals.begin(), eigenvals.end()), 0.5);
          std::vector<double> peakRadii(3, 0.0);
          std::vector<double> backgroundInnerRadii(3, 0.0);
          std::vector<double> backgroundOuterRadii(3, 0.0);
          for (size_t irad = 0; irad < peakRadii.size(); irad++) {
            auto scale = pow(eigenvals[irad], 0.5) / max_stdev;
            if (manualEllip) {
              peakRadii[irad] = PeakRadius[irad];
            } else {
              peakRadii[irad] = PeakRadiusVector[i] * scale;
            }

            if (BackgroundInnerRadius.size() == 1) {
              backgroundInnerRadii[irad] =
                  BackgroundInnerRadiusVector[i] * scale;
            } else {
              backgroundInnerRadii[irad] = BackgroundInnerRadius[irad];
            }
            if (BackgroundOuterRadius.size() == 1) {
              backgroundOuterRadii[irad] =
                  BackgroundOuterRadiusVector[i] * scale;
            } else {
              backgroundOuterRadii[irad] = BackgroundOuterRadius[irad];
            }
          }
          PeakShape *ellipsoidShape = new PeakShapeEllipsoid(
              eigenvects, peakRadii, backgroundInnerRadii, backgroundOuterRadii,
              CoordinatesToUse, this->name(), this->version());
          shapeablePeak->setPeakShape(ellipsoidShape);
        }
      }
      // spherical integration of signal
      ws->getBox()->integrateSphere(
          getRadiusSq, static_cast<coord_t>(adaptiveRadius * adaptiveRadius),
          signal, errorSquared, 0.0 /* innerRadiusSquared */,
          useOnePercentBackgroundCorrection);
    } else {
      CoordTransformDistance cylinder(nd, center, dimensionsUsed, 2);

      // Perform the integration into whatever box is contained within.
      Counts signal_fit(numSteps);
      signal_fit = 0;

      ws->getBox()->integrateCylinder(
          cylinder, static_cast<coord_t>(PeakRadius[0]),
          static_cast<coord_t>(cylinderLength), signal, errorSquared,
          signal_fit.mutableRawData());

      // Integrate around the background radius
      if (BackgroundOuterRadius[0] > PeakRadius[0]) {
        // Get the total signal inside "BackgroundOuterRadius"
        signal_fit = 0;

        ws->getBox()->integrateCylinder(
            cylinder, static_cast<coord_t>(BackgroundOuterRadius[0]),
            static_cast<coord_t>(cylinderLength), bgSignal, bgErrorSquared,
            signal_fit.mutableRawData());

        Points points(signal_fit.size(), LinearGenerator(0, 1));
        wsProfile2D->setHistogram(i, points, signal_fit);

        // Evaluate the signal inside "BackgroundInnerRadius"
        signal_t interiorSignal = 0;
        signal_t interiorErrorSquared = 0;

        // Integrate this 3rd radius, if needed
        if (BackgroundInnerRadius[0] != PeakRadius[0]) {
          ws->getBox()->integrateCylinder(
              cylinder, static_cast<coord_t>(BackgroundInnerRadius[0]),
              static_cast<coord_t>(cylinderLength), interiorSignal,
              interiorErrorSquared, signal_fit.mutableRawData());
        } else {
          // PeakRadius == BackgroundInnerRadius, so use the previous
          // value
          interiorSignal = signal;
          interiorErrorSquared = errorSquared;
        }
        // Subtract the peak part to get the intensity in the shell
        // (BackgroundInnerRadius < r < BackgroundOuterRadius)
        bgSignal -= interiorSignal;
        // We can subtract the error (instead of adding) because the two
        // values are 100% dependent; this is the same as integrating a
        // shell.
        bgErrorSquared -= interiorErrorSquared;
        // Relative volume of peak vs the BackgroundOuterRadius cylinder
        const double radiusRatio = (PeakRadius[0] / BackgroundOuterRadius[0]);
        const double peakVolume = radiusRatio * radiusRatio * cylinderLength;

        // Relative volume of the interior of the shell vs overall
        // background
        const double interiorRatio =
            (BackgroundInnerRadius[0] / BackgroundOuterRadius[0]);
        // Volume of the bg shell, relative to the volume of the
        // BackgroundOuterRadius cylinder
        const double bgVolume =
            1.0 - interiorRatio * interiorRatio * cylinderLength;

        // Finally, you will multiply the bg intensity by this to get the
        // estimated background under the peak volume
        const double scaleFactor = peakVolume / bgVolume;
        bgSignal *= scaleFactor;
        bgErrorSquared *= scaleFactor * scaleFactor;
      } else {
        Points points(signal_fit.size(), LinearGenerator(0, 1));
        wsProfile2D->setHistogram(i, points, signal_fit);
      }

      if (profileFunction == "NoFit") {
        signal = 0.;
        for (size_t j = 0; j < numSteps; j++) {
          if (j < peakMin || j > peakMax)
            background_total = background_total + wsProfile2D->mutableY(i)[j];
          else
            signal = signal + wsProfile2D->mutableY(i)[j];
        }
        errorSquared = std::fabs(signal);
      } else {

        IAlgorithm_sptr fitAlgorithm =
            createChildAlgorithm("Fit", -1, -1, false);
        // fitAlgorithm->setProperty("CreateOutput", true);
        // fitAlgorithm->setProperty("Output", "FitPeaks1D");
        std::string myFunc =
            std::string("name=LinearBackground;name=") + profileFunction;
        auto maxPeak = std::max_element(signal_fit.begin(), signal_fit.end());

        std::ostringstream strs;
        strs << maxPeak[0];
        std::string strMax = strs.str();
        if (profileFunction == "Gaussian") {
          myFunc += ", PeakCentre=50, Height=" + strMax;
          fitAlgorithm->setProperty("Constraints", "40<f1.PeakCentre<60");
        } else if (profileFunction == "BackToBackExponential" ||
                   profileFunction == "IkedaCarpenterPV") {
          myFunc += ", X0=50, I=" + strMax;
          fitAlgorithm->setProperty("Constraints", "40<f1.X0<60");
        }
        fitAlgorithm->setProperty("CalcErrors", true);
        fitAlgorithm->setProperty("Function", myFunc);
        fitAlgorithm->setProperty("InputWorkspace", wsProfile2D);
        fitAlgorithm->setProperty("WorkspaceIndex", static_cast<int>(i));
        try {
          fitAlgorithm->executeAsChildAlg();
        } catch (...) {
          g_log.error("Can't execute Fit algorithm");
          continue;
        }

        IFunction_sptr ifun = fitAlgorithm->getProperty("Function");
        if (i == 0) {
          out << std::setw(20) << "spectrum"
              << " ";
          for (size_t j = 0; j < ifun->nParams(); ++j)
            out << std::setw(20) << ifun->parameterName(j) << " ";
          out << std::setw(20) << "chi2"
              << " ";
          out << "\n";
        }
        out << std::setw(20) << i << " ";
        for (size_t j = 0; j < ifun->nParams(); ++j) {
          out << std::setw(20) << std::fixed << std::setprecision(10)
              << ifun->getParameter(j) << " ";
        }
        double chi2 = fitAlgorithm->getProperty("OutputChi2overDoF");
        out << std::setw(20) << std::fixed << std::setprecision(10) << chi2
            << "\n";

        std::shared_ptr<const CompositeFunction> fun =
            std::dynamic_pointer_cast<const CompositeFunction>(ifun);

        const auto &x = wsProfile2D->x(i);
        wsFit2D->setSharedX(i, wsProfile2D->sharedX(i));
        wsDiff2D->setSharedX(i, wsProfile2D->sharedX(i));

        FunctionDomain1DVector domain(x.rawData());
        FunctionValues yy(domain);
        fun->function(domain, yy);
        auto funcValues = yy.toVector();

        wsFit2D->mutableY(i) = std::move(funcValues);
        wsDiff2D->setSharedY(i, wsProfile2D->sharedY(i));
        wsDiff2D->mutableY(i) -= wsFit2D->y(i);

        // Calculate intensity
        signal = 0.0;
        if (integrationOption == "Sum") {
          for (size_t j = peakMin; j <= peakMax; j++)
            if (std::isfinite(yy[j]))
              signal += yy[j];
        } else {
          gsl_integration_workspace *w = gsl_integration_workspace_alloc(1000);

          double error;

          gsl_function F;
          F.function = &Mantid::MDAlgorithms::f_eval2;
          F.params = &fun;

          gsl_integration_qags(&F, x[peakMin], x[peakMax], 0, 1e-7, 1000, w,
                               &signal, &error);

          gsl_integration_workspace_free(w);
        }
        errorSquared = std::fabs(signal);
        // Get background counts
        for (size_t j = 0; j < numSteps; j++) {
          double background =
              ifun->getParameter(0) + ifun->getParameter(1) * x[j];
          if (j < peakMin || j > peakMax)
            background_total = background_total + background;
        }
      }
    }
    checkOverlap(
        i, peakWS, CoordinatesToUse,
        2.0 * std::max(PeakRadiusVector[i], BackgroundOuterRadiusVector[i]));
    // Save it back in the peak object.
    if (signal != 0. || replaceIntensity) {
      double edgeMultiplier = 1.0;
      double peakMultiplier = 1.0;
      if (correctEdge) {
        if (edge < BackgroundOuterRadius[0]) {
          double e1 = BackgroundOuterRadius[0] - edge;
          // volume of cap of sphere with h = edge
          double f1 =
              M_PI * std::pow(e1, 2) / 3 * (3 * BackgroundOuterRadius[0] - e1);
          edgeMultiplier = volumeBkg / (volumeBkg - f1);
        }
        if (edge < PeakRadius[0]) {
          double sigma = PeakRadius[0] / 3.0;
          // assume gaussian peak
          double e1 = std::exp(-std::pow(edge, 2) / (2 * sigma * sigma)) *
                      PeakRadius[0];
          // volume of cap of sphere with h = edge
          double f1 = M_PI * std::pow(e1, 2) / 3 * (3 * PeakRadius[0] - e1);
          peakMultiplier = volumeRadius / (volumeRadius - f1);
        }
      }

      p.setIntensity(peakMultiplier * signal -
                     edgeMultiplier * (ratio * background_total + bgSignal));
      p.setSigmaIntensity(
          sqrt(peakMultiplier * errorSquared +
               edgeMultiplier * (ratio * ratio * std::fabs(background_total) +
                                 bgErrorSquared)));
    }

    g_log.information() << "Peak " << i << " at " << pos << ": signal "
                        << signal << " (sig^2 " << errorSquared
                        << "), with background "
                        << bgSignal + ratio * background_total << " (sig^2 "
                        << bgErrorSquared +
                               ratio * ratio * std::fabs(background_total)
                        << ") subtracted.\n";
  }
  // This flag is used by the PeaksWorkspace to evaluate whether it has
  // been integrated.
  peakWS->mutableRun().addProperty("PeaksIntegrated", 1, true);
  // These flags are specific to the algorithm.
  peakWS->mutableRun().addProperty("PeakRadius", PeakRadiusVector, true);
  peakWS->mutableRun().addProperty("BackgroundInnerRadius",
                                   BackgroundInnerRadiusVector, true);
  peakWS->mutableRun().addProperty("BackgroundOuterRadius",
                                   BackgroundOuterRadiusVector, true);

  // save profiles in peaks file
  const std::string outfile = getProperty("ProfilesFile");
  if (outfile.length() > 0) {
    IAlgorithm_sptr alg;
    try {
      alg = createChildAlgorithm("SaveIsawPeaks", -1, -1, false);
    } catch (Exception::NotFoundError &) {
      g_log.error("Can't locate SaveIsawPeaks algorithm");
      throw;
    }
    alg->setProperty("InputWorkspace", peakWS);
    alg->setProperty("ProfileWorkspace", wsProfile2D);
    alg->setPropertyValue("Filename", outfile);
    alg->execute();
  }
  // Save the output
  setProperty("OutputWorkspace", peakWS);
}

/**
 * Calculate the covariance matrix of a spherical region and store the
 * eigenvectors and eigenvalues that diagonalise the covariance matrix in the
 * vectors provided
 *
 *  @param ws             input workspace
 *  @param getRadiusSq    Coord transfrom for sphere
 *  @param pos            V3D of peak centre
 *  @param radiusSquared  radius that defines spherical region for covarariance
 *  @param qAxisIsFixed      bool to fix an eigenvector along direction pos
 *  @param bgDensity      background counts per unit volume
 *  @param eigenvects     eigenvectors of covariance matrix of spherical region
 *  @param eigenvals      eigenvectors of covariance matrix of spherical region
 */
template <typename MDE, size_t nd>
void IntegratePeaksMD2::findEllipsoid(
    typename MDEventWorkspace<MDE, nd>::sptr ws,
    const CoordTransform &getRadiusSq, const V3D &pos,
    const coord_t &radiusSquared, const bool &qAxisIsFixed,
    const double &bgDensity, std::vector<V3D> &eigenvects,
    std::vector<double> &eigenvals) {
  double w_sum = 0.0;  // sum of weights
  Matrix<double> Evec; // hold eigenvectors
  Matrix<double> Eval; // hold eigenvals in diag
  // get leaf-only iterators over all boxes in ws
  auto function = std::make_unique<Geometry::MDAlgorithms::MDBoxMaskFunction>(
      pos, radiusSquared);
  MDBoxBase<MDE, nd> *baseBox = ws->getBox();
  MDBoxIterator<MDE, nd> MDiter(baseBox, 1000, true, function.get());

  Matrix<double> cov_mat;
  Matrix<double> Pinv(3, 3);
  if (qAxisIsFixed) {
    // 2D covar in plane perp to Q (uhat,vhat basis)
    cov_mat = Matrix<double>(2, 2);
    // transformation from Qlab to Qhat, vhat and uhat,
    getPinv(pos, Pinv);
  } else {
    cov_mat = Matrix<double>(3, 3);
  }
  std::vector<double> mean(3, 0.0);
  double var_Qhat = 0.0; //  variance parallel to Q (used if fix Q axis)

  // loop over all boxes inside radius
  do {
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(MDiter.getBox());
    if (box && !box->getIsMasked()) {
      const std::vector<MDE> &events = box->getConstEvents();
      auto bg = bgDensity / (static_cast<double>(events.size()) *
                             (box->getInverseVolume()));
      // For each event
      for (const auto &evnt : events) {
        std::vector<coord_t> center(nd);
        for (size_t d = 0; d < nd; ++d) {
          center[d] = evnt.getCenter(d);
        }
        coord_t out[nd];
        getRadiusSq.apply(center.data(), out);

        if (evnt.getSignal() > bg && out[0] < radiusSquared) {
          auto signal = (evnt.getSignal() - bg);
          w_sum += signal;

          if (qAxisIsFixed) {
            // transform coords to Q, uhat, vhat basis
            // use V3D for matrix algebra
            auto tmp = Pinv * V3D(static_cast<double>(center[0]),
                                  static_cast<double>(center[1]),
                                  static_cast<double>(center[2]));
            for (size_t d = 0; d < center.size(); ++d) {
              center[d] = static_cast<coord_t>(tmp[d]);
            }
          }

          // update mean
          for (size_t d = 0; d < mean.size(); ++d) {
            mean[d] += (signal / w_sum) * (center[d] - mean[d]);
          }

          if (qAxisIsFixed) {
            // get variance along Q
            var_Qhat += signal * pow((center[0] - mean[0]), 2);
          }
          for (size_t row = 0; row < cov_mat.numRows(); ++row) {
            for (size_t col = 0; col < cov_mat.numRows(); ++col) {
              // symmeteric matrix
              if (row <= col) {
                double cov = 0.0;
                if (!qAxisIsFixed) {
                  cov = signal * (center[row] - mean[row]) *
                        (center[col] - mean[col]);
                } else {
                  cov = signal * (center[row + 1] - mean[row + 1]) *
                        (center[col + 1] - mean[col + 1]);
                }
                if (row == col) {
                  cov_mat[row][col] += cov;
                } else {
                  cov_mat[row][col] += cov;
                  cov_mat[col][row] += cov;
                }
              }
            }
          }
        }
      }
    }
    box->releaseEvents();
  } while (MDiter.next());
  // normalise the covariance matrix
  cov_mat /= w_sum;                // normalise by sum of weights
  cov_mat.Diagonalise(Evec, Eval); // 3 x 3 matrices
  eigenvals = Eval.Diagonal();
  if (qAxisIsFixed) {
    // insert variance along Q (first eigenvector)
    eigenvals.insert(eigenvals.begin(), var_Qhat / w_sum);
    eigenvects.push_back(pos / pos.norm());
  }
  // set min eigenval to be small but non-zero (1e-6)
  // when no discernible peak above background
  std::replace_if(eigenvals.begin(), eigenvals.end(),
                  [&](auto x) { return x < 1e-6; }, 1e-6);
  // populate rest of eigenvect/vals
  for (size_t ivect = 0; ivect < cov_mat.numRows(); ++ivect) {
    if (!qAxisIsFixed) {
      eigenvects.push_back(V3D(Evec[0][ivect], Evec[1][ivect], Evec[2][ivect]));
    } else {
      // transform back to Qlab basis
      eigenvects.push_back(V3D(0.0, 0.0, 0.0));
      for (size_t ibasis = 0; ibasis < Evec.numRows(); ++ibasis) {
        eigenvects.back() +=
            V3D(Pinv[ibasis + 1][0], Pinv[ibasis + 1][1], Pinv[ibasis + 1][2]) *
            Evec[ibasis][ivect];
      }
    }
  }
}

/**
 * Get the inverse of the matrix P which transforms from Qlab to basis Qhat,
 * and uhat,vhat in plane perpendicular to Q. P is a matrix with columns
 * corresponding to new basis vectors. The inverse of P is equivilent to the
 * transpose (as for any roation matrix)
 *
 *  @param q     Qlab of peak center.
 *  @param Pinv  3 x 3 matrix with rows correpsonding to new basis vectors
 */
void IntegratePeaksMD2::getPinv(const V3D &q, Kernel::Matrix<double> &Pinv) {
  // loop over 3 mutually-orthogonal vectors  until get one with
  // a component perp to Q (within tolerance)
  double dotprod = 1;
  size_t ii = 0;
  V3D qhat = q / q.norm();
  V3D tmp;
  do {
    tmp = V3D(0, 0, 0); // reset u
    tmp[ii] = 1.0;
    dotprod = qhat.scalar_prod(tmp);
    ii++;
  } while (abs(dotprod) > 1.0 - 1e-6);
  // populate Pinv with basis vector rows
  Pinv.setRow(0, qhat);
  tmp = qhat.cross_prod(tmp);
  Pinv.setRow(1, tmp / tmp.norm());
  tmp = qhat.cross_prod(tmp);
  Pinv.setRow(2, tmp / tmp.norm());
}

/*
 * Define edges for each instrument by masking. For CORELLI, tubes 1 and
 *16, and pixels 0 and 255. Get Q in the lab frame for every peak, call it
 *C For every point on the edge, the trajectory in reciprocal space is a
 *straight line, going through O=V3D(0,0,0). Calculate a point at a fixed
 *momentum, say k=1. Q in the lab frame
 *E=V3D(-k*sin(tt)*cos(ph),-k*sin(tt)*sin(ph),k-k*cos(ph)).
 * Normalize E to 1: E=E*(1./E.norm())
 *
 * @param inst: instrument
 */
void IntegratePeaksMD2::calculateE1(
    const Geometry::DetectorInfo &detectorInfo) {
  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if (detectorInfo.isMonitor(i))
      continue; // skip monitor
    if (!detectorInfo.isMasked(i))
      continue; // edge is masked so don't check if not masked
    const auto &det = detectorInfo.detector(i);
    double tt1 = det.getTwoTheta(V3D(0, 0, 0), V3D(0, 0, 1)); // two theta
    double ph1 = det.getPhi();                                // phi
    V3D E1 = V3D(-std::sin(tt1) * std::cos(ph1), -std::sin(tt1) * std::sin(ph1),
                 1. - std::cos(tt1)); // end of trajectory
    E1 = E1 * (1. / E1.norm());       // normalize
    E1Vec.emplace_back(E1);
  }
}

/** Calculate if this Q is on a detector
 * The distance from C to OE is given by dv=C-E*(C.scalar_prod(E))
 * If dv.norm<integration_radius, one of the detector trajectories on the
 *edge is too close to the peak This method is applied to all masked
 *pixels. If there are masked pixels trajectories inside an integration
 *volume, the peak must be rejected.
 *
 * @param QLabFrame: The Peak center.
 * @param r: Peak radius.
 */
double IntegratePeaksMD2::detectorQ(Mantid::Kernel::V3D QLabFrame, double r) {
  double edge = r;
  for (auto &E1 : E1Vec) {
    V3D distv = QLabFrame - E1 * (QLabFrame.scalar_prod(E1)); // distance to the
                                                              // trajectory as a
                                                              // vector
    if (distv.norm() < r) {
      edge = distv.norm();
    }
  }
  return edge;
}

void IntegratePeaksMD2::runMaskDetectors(
    const Mantid::DataObjects::PeaksWorkspace_sptr &peakWS,
    const std::string &property, const std::string &values) {
  // For CORELLI do not count as edge if next to another detector bank
  if (property == "Tube" && peakWS->getInstrument()->getName() == "CORELLI") {
    IAlgorithm_sptr alg = createChildAlgorithm("MaskBTP");
    alg->setProperty<Workspace_sptr>("Workspace", peakWS);
    alg->setProperty("Bank", "1,7,12,17,22,27,30,59,63,69,74,79,84,89");
    alg->setProperty(property, "1");
    if (!alg->execute())
      throw std::runtime_error(
          "MaskDetectors Child Algorithm has not executed successfully");
    IAlgorithm_sptr alg2 = createChildAlgorithm("MaskBTP");
    alg2->setProperty<Workspace_sptr>("Workspace", peakWS);
    alg2->setProperty("Bank", "6,11,16,21,26,29,58,62,68,73,78,83,88,91");
    alg2->setProperty(property, "16");
    if (!alg2->execute())
      throw std::runtime_error(
          "MaskDetectors Child Algorithm has not executed successfully");
  } else {
    IAlgorithm_sptr alg = createChildAlgorithm("MaskBTP");
    alg->setProperty<Workspace_sptr>("Workspace", peakWS);
    alg->setProperty(property, values);
    if (!alg->execute())
      throw std::runtime_error(
          "MaskDetectors Child Algorithm has not executed successfully");
  }
}

void IntegratePeaksMD2::checkOverlap(
    int i, const Mantid::DataObjects::PeaksWorkspace_sptr &peakWS,
    Mantid::Kernel::SpecialCoordinateSystem CoordinatesToUse, double radius) {
  // Get a direct ref to that peak.
  IPeak &p1 = peakWS->getPeak(i);
  V3D pos1;
  if (CoordinatesToUse == Kernel::QLab) //"Q (lab frame)"
    pos1 = p1.getQLabFrame();
  else if (CoordinatesToUse == Kernel::QSample) //"Q (sample frame)"
    pos1 = p1.getQSampleFrame();
  else if (CoordinatesToUse == Kernel::HKL) //"HKL"
    pos1 = p1.getHKL();
  for (int j = i + 1; j < peakWS->getNumberPeaks(); ++j) {
    // Get a direct ref to rest of peaks peak.
    IPeak &p2 = peakWS->getPeak(j);
    V3D pos2;
    if (CoordinatesToUse == Kernel::QLab) //"Q (lab frame)"
      pos2 = p2.getQLabFrame();
    else if (CoordinatesToUse == Kernel::QSample) //"Q (sample frame)"
      pos2 = p2.getQSampleFrame();
    else if (CoordinatesToUse == Kernel::HKL) //"HKL"
      pos2 = p2.getHKL();
    if (pos1.distance(pos2) < radius) {
      g_log.warning() << " Warning:  Peak integration spheres for peaks " << i
                      << " and " << j << " overlap.  Distance between peaks is "
                      << pos1.distance(pos2) << '\n';
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void IntegratePeaksMD2::exec() {
  inWS = getProperty("InputWorkspace");

  CALL_MDEVENT_FUNCTION(this->integrate, inWS);
}

double f_eval2(double x, void *params) {
  std::shared_ptr<const API::CompositeFunction> fun =
      *reinterpret_cast<std::shared_ptr<const API::CompositeFunction> *>(
          params);
  FunctionDomain1DVector domain(x);
  FunctionValues yval(domain);
  fun->function(domain, yval);
  return yval[0];
}

} // namespace MDAlgorithms
} // namespace Mantid
