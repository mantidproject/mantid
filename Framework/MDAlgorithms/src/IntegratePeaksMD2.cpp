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
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
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
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/SetValueWhenProperty.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidMDAlgorithms/GSLFunctions.h"
#include "MantidMDAlgorithms/MDBoxMaskFunction.h"

#include "boost/math/distributions.hpp"

#include <cmath>
#include <fstream>
#include <gsl/gsl_integration.h>

namespace Mantid::MDAlgorithms {

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
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input MDEventWorkspace.");

  auto radiiValidator = std::make_shared<ArrayBoundedValidator<double>>();
  radiiValidator->setLower(0.0);
  radiiValidator->setLowerExclusive(true);
  declareProperty(std::make_unique<ArrayProperty<double>>("PeakRadius", std::vector<double>({1.0}), radiiValidator,
                                                          Direction::Input),
                  "Fixed radius around each peak position in which to integrate, or the "
                  "semi-axis lengths (a,b,c) describing an ellipsoid shape used for "
                  "integration (in the same units as the workspace).");

  radiiValidator->setLowerExclusive(false);
  declareProperty(std::make_unique<ArrayProperty<double>>("BackgroundInnerRadius", std::vector<double>({0.0}),
                                                          radiiValidator, Direction::Input),
                  "Inner radius, or three values for semi-axis lengths (a,b,c) of the "
                  "ellipsoid shape, used to evaluate the background of the peak.\n"
                  "If smaller than PeakRadius, then we assume BackgroundInnerRadius = "
                  "PeakRadius.");

  declareProperty(std::make_unique<ArrayProperty<double>>("BackgroundOuterRadius", std::vector<double>({0.0}),
                                                          radiiValidator, Direction::Input),
                  "Outer radius, or three values for semi-axis lengths (a,b,c) of the "
                  "ellipsoid shape, to use to evaluate the background of the peak.\n"
                  "The signal density around the peak (BackgroundInnerRadius < r < "
                  "BackgroundOuterRadius) is used to estimate the background under the "
                  "peak.\n"
                  "If smaller than PeakRadius, no background measurement is done.");

  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("PeaksWorkspace", "", Direction::Input),
                  "A PeaksWorkspace containing the peaks to integrate.");

  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
                  "with the peaks' integrated intensities.");

  declareProperty("ReplaceIntensity", true,
                  "Always replace intensity in PeaksWorkspacem (default).\n"
                  "If false, then do not replace intensity if calculated value "
                  "is 0 (used for SNSSingleCrystalReduction)");

  declareProperty("IntegrateIfOnEdge", true,
                  "Only warning if all of peak outer radius is not on detector (default).\n"
                  "If false, do not integrate if the outer radius is not on a detector.");

  declareProperty("AdaptiveQBackground", false,
                  "Default is false.   If true, "
                  "BackgroundOuterRadius + AdaptiveQMultiplier * **|Q|** and "
                  "BackgroundInnerRadius + AdaptiveQMultiplier * **|Q|**");

  declareProperty("Ellipsoid", false, "Default is sphere.");

  declareProperty("FixQAxis", false, "Fix one axis of ellipsoid to be along direction of Q.");

  declareProperty("Cylinder", false, "Default is sphere.  Use next five parameters for cylinder.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("CylinderLength", 0.0, Direction::Input),
                  "Length of cylinder in which to integrate (in the same units as the "
                  "workspace).");

  declareProperty(std::make_unique<PropertyWithValue<double>>("PercentBackground", 0.0, Direction::Input),
                  "Percent of CylinderLength that is background (20 is 20%)");

  std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
  peakNames.emplace_back("NoFit");
  declareProperty("ProfileFunction", "Gaussian", std::make_shared<StringListValidator>(peakNames),
                  "Fitting function for profile that is used only with "
                  "Cylinder integration.");

  std::vector<std::string> integrationOptions(2);
  integrationOptions[0] = "Sum";
  integrationOptions[1] = "GaussianQuadrature";
  auto integrationvalidator = std::make_shared<StringListValidator>(integrationOptions);
  declareProperty("IntegrationOption", "GaussianQuadrature", integrationvalidator,
                  "Integration method for calculating intensity "
                  "used only with Cylinder integration.");

  declareProperty(std::make_unique<FileProperty>("ProfilesFile", "", FileProperty::OptionalSave,
                                                 std::vector<std::string>(1, "profiles")),
                  "Save (Optionally) as Isaw peaks file with profiles included");

  declareProperty("AdaptiveQMultiplier", 0.0,
                  "PeakRadius + AdaptiveQMultiplier * **|Q|** "
                  "so each peak has a "
                  "different integration radius.  Q includes the 2*pi factor.");

  declareProperty("CorrectIfOnEdge", false,
                  "Only warning if all of peak outer radius is not on detector (default).\n"
                  "If false, correct for volume off edge for both background and "
                  "intensity (the peak is assumed uniform Gaussian so this only applies "
                  "to spherical integration).");

  declareProperty("UseOnePercentBackgroundCorrection", true,
                  "If this options is enabled, then the top 1% of the "
                  "background will be removed"
                  "before the background subtraction.");

  // continued ellipsoid args
  declareProperty("FixMajorAxisLength", true,
                  "This option is ignored if all peak radii are specified. "
                  "Otherwise, if True the ellipsoid radidi (proportional to "
                  "the sqrt of the eigenvalues of the covariance matrix) are "
                  "scaled such that the major axis radius is equal to the "
                  "PeakRadius. If False then the ellipsoid radii are set to "
                  "3 times the sqrt of the eigenvalues of the covariance "
                  "matrix");

  declareProperty("UseCentroid", false,
                  "Perform integration on estimated centroid not peak position "
                  "(ignored if all three peak radii are specified).");

  auto maxIterValidator = std::make_shared<BoundedValidator<int>>();
  maxIterValidator->setLower(1);
  declareProperty("MaxIterations", 1, maxIterValidator,
                  "Number of iterations in covariance estimation (ignored if all "
                  "peak radii are specified). 2-3 should be sufficient.");

  declareProperty(
      "MaskEdgeTubes", true,
      "Mask tubes on the edge of all banks in the PeaksWorkspace instrument (note the edge pixels at top/bottom of all "
      "tubes will always be masked even if this property is False). Note the algorithm will treat "
      "any masked pixels as edges (including pixels already masked prior to the execution of this algorithm) - this "
      "means a custom mask can be applied to the PeaksWorkspace before integration.");

  // Group Properties
  std::string general_grp = "General Inputs";
  std::string cylin_grp = "Cylindrical Integration";
  std::string ellip_grp = "Ellipsoid Integration";

  setPropertyGroup("InputWorkspace", general_grp);
  setPropertyGroup("PeakRadius", general_grp);
  setPropertyGroup("BackgroundInnerRadius", general_grp);
  setPropertyGroup("BackgroundOuterRadius", general_grp);
  setPropertyGroup("PeaksWorkspace", general_grp);
  setPropertyGroup("OutputWorkspace", general_grp);
  setPropertyGroup("ReplaceIntensity", general_grp);
  setPropertyGroup("IntegrateIfOnEdge", general_grp);
  setPropertyGroup("AdaptiveQBackground", general_grp);

  setPropertyGroup("Ellipsoid", ellip_grp);
  setPropertyGroup("FixQAxis", ellip_grp);

  setPropertyGroup("Cylinder", cylin_grp);
  setPropertyGroup("CylinderLength", cylin_grp);
  setPropertyGroup("PercentBackground", cylin_grp);
  setPropertyGroup("ProfileFunction", cylin_grp);
  setPropertyGroup("IntegrationOption", cylin_grp);
  setPropertyGroup("ProfilesFile", cylin_grp);

  setPropertyGroup("AdaptiveQMultiplier", general_grp);
  setPropertyGroup("CorrectIfOnEdge", general_grp);
  setPropertyGroup("UseOnePercentBackgroundCorrection", general_grp);

  setPropertyGroup("FixMajorAxisLength", ellip_grp);
  setPropertyGroup("UseCentroid", ellip_grp);
  setPropertyGroup("MaxIterations", ellip_grp);

  setPropertyGroup("MaskEdgeTubes", general_grp);

  // SetValue when another property value changes
  setPropertySettings("Ellipsoid", std::make_unique<SetValueWhenProperty>(
                                       "Cylinder", [](std::string ellipsoid, const std::string &cylinder) {
                                         // Set Ellipsoid to 0, if user has set Cylinder to 1
                                         if (ellipsoid == "1" && cylinder == "1") {
                                           return std::string{"0"};
                                         } else {
                                           return ellipsoid;
                                         };
                                       }));
  setPropertySettings("Cylinder", std::make_unique<SetValueWhenProperty>(
                                      "Ellipsoid", [](std::string cylinder, const std::string &ellipsoid) {
                                        // Set Cylinder to 0, if user has set Ellipsoid to 1
                                        if (cylinder == "1" && ellipsoid == "1") {
                                          return std::string{"0"};
                                        } else {
                                          return cylinder;
                                        };
                                      }));

  // Set these Properties as visible only when Cylinder = 1
  setPropertySettings("CylinderLength", std::make_unique<VisibleWhenProperty>("Cylinder", IS_EQUAL_TO, "1"));
  setPropertySettings("PercentBackground", std::make_unique<VisibleWhenProperty>("Cylinder", IS_EQUAL_TO, "1"));
  setPropertySettings("ProfileFunction", std::make_unique<VisibleWhenProperty>("Cylinder", IS_EQUAL_TO, "1"));
  setPropertySettings("IntegrationOption", std::make_unique<VisibleWhenProperty>("Cylinder", IS_EQUAL_TO, "1"));
  setPropertySettings("ProfilesFile", std::make_unique<VisibleWhenProperty>("Cylinder", IS_EQUAL_TO, "1"));

  // Set these Properties as visible only when Ellipsoid = 1
  setPropertySettings("FixQAxis", std::make_unique<VisibleWhenProperty>("Ellipsoid", IS_EQUAL_TO, "1"));
  setPropertySettings("FixMajorAxisLength", std::make_unique<VisibleWhenProperty>("Ellipsoid", IS_EQUAL_TO, "1"));
  setPropertySettings("UseCentroid", std::make_unique<VisibleWhenProperty>("Ellipsoid", IS_EQUAL_TO, "1"));
  setPropertySettings("MaxIterations", std::make_unique<VisibleWhenProperty>("Ellipsoid", IS_EQUAL_TO, "1"));

  // Disable / greyed out these Properties based on the value of another
  setPropertySettings("CorrectIfOnEdge",
                      std::make_unique<Kernel::EnabledWhenProperty>("IntegrateIfOnEdge", IS_EQUAL_TO, "1"));
}

std::map<std::string, std::string> IntegratePeaksMD2::validateInputs() {
  std::map<std::string, std::string> result;

  std::vector<double> PeakRadius = getProperty("PeakRadius");
  std::vector<double> BackgroundInnerRadius = getProperty("BackgroundInnerRadius");
  std::vector<double> BackgroundOuterRadius = getProperty("BackgroundOuterRadius");
  bool ellipsoid = getProperty("Ellipsoid");
  bool cylinder = getProperty("Cylinder");

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

  if (ellipsoid && cylinder) {
    std::stringstream errmsg;
    errmsg << "Ellipsoid and Cylinder cannot both be true";
    result["Ellipsoid"] = errmsg.str();
    result["Cylinder"] = errmsg.str();
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Integrate the peaks of the workspace using parameters saved in the algorithm
 * class
 * @param ws ::  MDEventWorkspace to integrate
 */
template <typename MDE, size_t nd> void IntegratePeaksMD2::integrate(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  if (nd != 3)
    throw std::invalid_argument("For now, we expect the input MDEventWorkspace "
                                "to have 3 dimensions only.");

  /// Peak workspace to integrate
  IPeaksWorkspace_sptr inPeakWS = getProperty("PeaksWorkspace");

  /// Output peaks workspace, create if needed
  IPeaksWorkspace_sptr peakWS = getProperty("OutputWorkspace");
  if (peakWS != inPeakWS)
    peakWS = inPeakWS->clone();
  // This only fails in the unit tests which say that MaskBTP is not registered
  bool maskTubes = getProperty("MaskEdgeTubes");
  try {
    PeaksWorkspace_sptr p = std::dynamic_pointer_cast<PeaksWorkspace>(inPeakWS);
    if (p) {
      if (maskTubes) {
        runMaskDetectors(p, "Tube", "edges");
      }
      runMaskDetectors(p, "Pixel", "edges");
    }
  } catch (...) {
    g_log.error("Can't execute MaskBTP algorithm for this instrument to set "
                "edge for IntegrateIfOnEdge option");
  }

  calculateE1(inPeakWS->detectorInfo()); // fill E1Vec for use in detectorQ
  Mantid::Kernel::SpecialCoordinateSystem CoordinatesToUse = ws->getSpecialCoordinateSystem();

  /// Radius to use around peaks
  std::vector<double> PeakRadius = getProperty("PeakRadius");
  /// Background (end) radius
  std::vector<double> BackgroundOuterRadius = getProperty("BackgroundOuterRadius");
  /// Start radius of the background
  std::vector<double> BackgroundInnerRadius = getProperty("BackgroundInnerRadius");
  /// One percent background correction
  bool useOnePercentBackgroundCorrection = getProperty("UseOnePercentBackgroundCorrection");

  bool manualEllip = false;
  if (PeakRadius.size() > 1) {
    manualEllip = true;
    // make sure the background radii are 3 values (they default to 1)
    if (BackgroundInnerRadius.size() == 1)
      BackgroundInnerRadius.resize(3, BackgroundInnerRadius[0]);
    if (BackgroundOuterRadius.size() == 1)
      BackgroundOuterRadius.resize(3, BackgroundOuterRadius[0]);
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
  bool majorAxisLengthFixed = getProperty("FixMajorAxisLength");
  bool useCentroid = getProperty("UseCentroid");
  int maxCovarIter = getProperty("MaxIterations");
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
  std::vector<double> BackgroundInnerRadiusVector(peakWS->getNumberPeaks(), BackgroundInnerRadius[0]);
  std::vector<double> BackgroundOuterRadiusVector(peakWS->getNumberPeaks(), BackgroundOuterRadius[0]);
  if (cylinderBool) {
    numSteps = 100;
    size_t histogramNumber = peakWS->getNumberPeaks();
    Workspace_sptr wsProfile = WorkspaceFactory::Instance().create("Workspace2D", histogramNumber, numSteps, numSteps);
    wsProfile2D = std::dynamic_pointer_cast<Workspace2D>(wsProfile);
    AnalysisDataService::Instance().addOrReplace("ProfilesData", wsProfile2D);
    Workspace_sptr wsFit = WorkspaceFactory::Instance().create("Workspace2D", histogramNumber, numSteps, numSteps);
    wsFit2D = std::dynamic_pointer_cast<Workspace2D>(wsFit);
    AnalysisDataService::Instance().addOrReplace("ProfilesFit", wsFit2D);
    Workspace_sptr wsDiff = WorkspaceFactory::Instance().create("Workspace2D", histogramNumber, numSteps, numSteps);
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
      label << Utils::round(p.getH()) << "_" << Utils::round(p.getK()) << "_" << Utils::round(p.getL()) << "_"
            << p.getRunNumber();
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
    peakMin = static_cast<size_t>(static_cast<double>(numSteps) * percentBackground / 100.);
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
    std::string save_path = ConfigService::Instance().getString("defaultsave.directory");
    outFile = save_path + outFile;
    out.open(outFile.c_str(), std::ofstream::out);
  }
  // volume of Background sphere with inner volume subtracted
  double volumeBkg = 4.0 / 3.0 * M_PI * (std::pow(BackgroundOuterRadius[0], 3) - std::pow(BackgroundOuterRadius[0], 3));
  // volume of PeakRadius sphere
  double volumeRadius = 4.0 / 3.0 * M_PI * std::pow(PeakRadius[0], 3);

  // Initialize progress reporting
  int nPeaks = peakWS->getNumberPeaks();
  Progress progress(this, 0., 1., nPeaks);
  bool doParallel = cylinderBool ? false : Kernel::threadSafe(*ws, *peakWS);
  PARALLEL_FOR_IF(doParallel)
  for (int i = 0; i < nPeaks; ++i) {
    PARALLEL_START_INTERRUPT_REGION
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

    const double edgeDist = calculateDistanceToEdge(p.getQLabFrame());
    if (edgeDist < std::max(BackgroundOuterRadius[0], PeakRadius[0])) {
      g_log.warning() << "Warning: sphere/cylinder for integration is off edge "
                         "of detector for peak "
                      << i << "; radius of edge =  " << edgeDist << '\n';
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
      double adaptiveRadius = adaptiveQMultiplier * lenQpeak + *std::max_element(PeakRadius.begin(), PeakRadius.end());
      if (adaptiveRadius <= 0.0) {
        g_log.error() << "Error: Radius for integration sphere of peak " << i << " is negative =  " << adaptiveRadius
                      << '\n';
        adaptiveRadius = 0.;
        p.setIntensity(0.0);
        p.setSigmaIntensity(0.0);
        PeakRadiusVector[i] = 0.0;
        BackgroundInnerRadiusVector[i] = 0.0;
        BackgroundOuterRadiusVector[i] = 0.0;
        continue;
      }
      PeakRadiusVector[i] = adaptiveRadius;
      BackgroundInnerRadiusVector[i] = adaptiveQBackgroundMultiplier * lenQpeak +
                                       *std::max_element(BackgroundInnerRadius.begin(), BackgroundInnerRadius.end());
      BackgroundOuterRadiusVector[i] = adaptiveQBackgroundMultiplier * lenQpeak +
                                       *std::max_element(BackgroundOuterRadius.begin(), BackgroundOuterRadius.end());
      // define the radius squared for a sphere intially
      CoordTransformDistance getRadiusSq(nd, center, dimensionsUsed);
      // set spherical shape
      PeakShape *sphereShape =
          new PeakShapeSpherical(PeakRadiusVector[i], BackgroundInnerRadiusVector[i], BackgroundOuterRadiusVector[i],
                                 CoordinatesToUse, this->name(), this->version());
      p.setPeakShape(sphereShape);
      const double scaleFactor = pow(PeakRadiusVector[i], 3) /
                                 (pow(BackgroundOuterRadiusVector[i], 3) - pow(BackgroundInnerRadiusVector[i], 3));
      // Integrate spherical background shell if specified
      if (BackgroundOuterRadius[0] > PeakRadius[0]) {
        // Get the total signal inside background shell
        ws->getBox()->integrateSphere(
            getRadiusSq, static_cast<coord_t>(pow(BackgroundOuterRadiusVector[i], 2)), bgSignal, bgErrorSquared,
            static_cast<coord_t>(pow(BackgroundInnerRadiusVector[i], 2)), useOnePercentBackgroundCorrection);
        // correct bg signal by Vpeak/Vshell (same for sphere and ellipse)
        bgSignal *= scaleFactor;
        bgErrorSquared *= scaleFactor * scaleFactor;
      }
      // if ellipsoid find covariance and centroid in spherical region
      // using one-pass algorithm from https://doi.org/10.1145/359146.359153
      if (isEllipse) {
        // flat bg to subtract
        const auto bgDensity = bgSignal / (4 * M_PI * pow(PeakRadiusVector[i], 3) / 3);
        std::vector<V3D> eigenvects;
        std::vector<double> eigenvals;
        V3D translation(0.0, 0.0, 0.0); // translation from peak pos to centroid
        if (PeakRadius.size() == 1) {
          V3D mean(0.0, 0.0, 0.0); // vector to hold centroid
          findEllipsoid<MDE, nd>(ws, getRadiusSq, pos, static_cast<coord_t>(pow(PeakRadiusVector[i], 2)), qAxisIsFixed,
                                 useCentroid, bgDensity, eigenvects, eigenvals, mean, maxCovarIter);
          if (!majorAxisLengthFixed) {
            // replace radius for this peak with 3*stdev along major axis
            auto max_stdev = sqrt(*std::max_element(eigenvals.begin(), eigenvals.end()));
            BackgroundOuterRadiusVector[i] = 3 * max_stdev * (BackgroundOuterRadiusVector[i] / PeakRadiusVector[i]);
            BackgroundInnerRadiusVector[i] = 3 * max_stdev * (BackgroundInnerRadiusVector[i] / PeakRadiusVector[i]);
            PeakRadiusVector[i] = 3 * max_stdev;
          }
          if (useCentroid) {
            // calculate translation to apply when drawing
            translation = mean - pos;
            // update integration center with mean
            for (size_t d = 0; d < 3; ++d) {
              center[d] = static_cast<coord_t>(mean[d]);
            }
          }
        } else {
          // Use the manually specified radii instead of finding them via
          // findEllipsoid
          std::transform(PeakRadius.begin(), PeakRadius.end(), std::back_inserter(eigenvals),
                         [](double r) { return std::pow(r, 2.0); });
          eigenvects.push_back(V3D(1.0, 0.0, 0.0));
          eigenvects.push_back(V3D(0.0, 1.0, 0.0));
          eigenvects.push_back(V3D(0.0, 0.0, 1.0));
        }
        // transform ellispoid onto sphere of radius = R
        getRadiusSq = CoordTransformDistance(nd, center, dimensionsUsed, 1, /* outD */
                                             eigenvects, eigenvals);
        // Integrate ellipsoid background shell if specified
        if (PeakRadius.size() == 1) {
          if (BackgroundOuterRadius[0] > PeakRadius[0]) {
            // Get the total signal inside "BackgroundOuterRadius"
            bgSignal = 0;
            bgErrorSquared = 0;
            ws->getBox()->integrateSphere(
                getRadiusSq, static_cast<coord_t>(pow(BackgroundOuterRadiusVector[i], 2)), bgSignal, bgErrorSquared,
                static_cast<coord_t>(pow(BackgroundInnerRadiusVector[i], 2)), useOnePercentBackgroundCorrection);
            // correct bg signal by Vpeak/Vshell (same as previously
            // calculated for sphere)
            bgSignal *= scaleFactor;
            bgErrorSquared *= scaleFactor * scaleFactor;
          }
          // set peak shape
          // get radii in same proprtion as eigenvalues
          auto max_stdev = pow(*std::max_element(eigenvals.begin(), eigenvals.end()), 0.5);
          std::vector<double> peakRadii(3, 0.0);
          std::vector<double> backgroundInnerRadii(3, 0.0);
          std::vector<double> backgroundOuterRadii(3, 0.0);
          for (size_t irad = 0; irad < peakRadii.size(); irad++) {
            auto scale = pow(eigenvals[irad], 0.5) / max_stdev;
            peakRadii[irad] = PeakRadiusVector[i] * scale;
            backgroundInnerRadii[irad] = BackgroundInnerRadiusVector[i] * scale;
            backgroundOuterRadii[irad] = BackgroundOuterRadiusVector[i] * scale;
          }
          PeakShape *ellipsoidShape =
              new PeakShapeEllipsoid(eigenvects, peakRadii, backgroundInnerRadii, backgroundOuterRadii,
                                     CoordinatesToUse, this->name(), this->version(), translation);
          p.setPeakShape(ellipsoidShape);
        } else {
          // Use the manually specified radii instead of finding them via
          // findEllipsoid
          std::vector<double> eigenvals_background_inner;
          std::vector<double> eigenvals_background_outer;
          std::transform(BackgroundInnerRadius.begin(), BackgroundInnerRadius.end(),
                         std::back_inserter(eigenvals_background_inner), [](double r) { return std::pow(r, 2.0); });
          std::transform(BackgroundOuterRadius.begin(), BackgroundOuterRadius.end(),
                         std::back_inserter(eigenvals_background_outer), [](double r) { return std::pow(r, 2.0); });

          if (BackgroundOuterRadiusVector[0] > PeakRadiusVector[0]) {
            // transform ellispoid onto sphere of radius = R
            auto getRadiusSqInner = CoordTransformDistance(nd, center, dimensionsUsed, 1, /* outD */
                                                           eigenvects, eigenvals_background_inner);
            auto getRadiusSqOuter = CoordTransformDistance(nd, center, dimensionsUsed, 1, /* outD */
                                                           eigenvects, eigenvals_background_outer);
            // Get the total signal inside "BackgroundOuterRadius"
            bgSignal = 0;
            bgErrorSquared = 0;
            signal_t bgSignalInner = 0;
            signal_t bgSignalOuter = 0;
            signal_t bgErrorSquaredInner = 0;
            signal_t bgErrorSquaredOuter = 0;
            ws->getBox()->integrateSphere(getRadiusSqInner,
                                          static_cast<coord_t>(pow(BackgroundInnerRadiusVector[i], 2)), bgSignalInner,
                                          bgErrorSquaredInner, 0.0, useOnePercentBackgroundCorrection);
            ws->getBox()->integrateSphere(getRadiusSqOuter,
                                          static_cast<coord_t>(pow(BackgroundOuterRadiusVector[i], 2)), bgSignalOuter,
                                          bgErrorSquaredOuter, 0.0, useOnePercentBackgroundCorrection);
            // correct bg signal by Vpeak/Vshell (same as previously
            // calculated for sphere)
            bgSignal = bgSignalOuter - bgSignalInner;
            bgErrorSquared = bgErrorSquaredInner + bgErrorSquaredOuter;
            g_log.debug() << "unscaled background signal from ellipsoid integration = " << bgSignal << '\n';
            const double scale = (PeakRadius[0] * PeakRadius[1] * PeakRadius[2]) /
                                 (BackgroundOuterRadius[0] * BackgroundOuterRadius[1] * BackgroundOuterRadius[2] -
                                  BackgroundInnerRadius[0] * BackgroundInnerRadius[1] * BackgroundInnerRadius[2]);
            bgSignal *= scale;
            bgErrorSquared *= pow(scale, 2);
          }
          // set peak shape
          // get radii in same proprtion as eigenvalues
          auto max_stdev = pow(*std::max_element(eigenvals.begin(), eigenvals.end()), 0.5);
          auto max_stdev_inner =
              pow(*std::max_element(eigenvals_background_inner.begin(), eigenvals_background_inner.end()), 0.5);
          auto max_stdev_outer =
              pow(*std::max_element(eigenvals_background_outer.begin(), eigenvals_background_outer.end()), 0.5);
          std::vector<double> peakRadii(3, 0.0);
          std::vector<double> backgroundInnerRadii(3, 0.0);
          std::vector<double> backgroundOuterRadii(3, 0.0);
          for (size_t irad = 0; irad < peakRadii.size(); irad++) {
            peakRadii[irad] = PeakRadiusVector[i] * pow(eigenvals[irad], 0.5) / max_stdev;
            backgroundInnerRadii[irad] =
                BackgroundInnerRadiusVector[i] * pow(eigenvals_background_inner[irad], 0.5) / max_stdev_inner;
            backgroundOuterRadii[irad] =
                BackgroundOuterRadiusVector[i] * pow(eigenvals_background_outer[irad], 0.5) / max_stdev_outer;
          }
          PeakShape *ellipsoidShape =
              new PeakShapeEllipsoid(eigenvects, peakRadii, backgroundInnerRadii, backgroundOuterRadii,
                                     CoordinatesToUse, this->name(), this->version());
          p.setPeakShape(ellipsoidShape);
        }
      }
      ws->getBox()->integrateSphere(getRadiusSq, static_cast<coord_t>(PeakRadiusVector[i] * PeakRadiusVector[i]),
                                    signal, errorSquared, 0.0 /* innerRadiusSquared */,
                                    useOnePercentBackgroundCorrection);
      //
    } else {
      CoordTransformDistance cylinder(nd, center, dimensionsUsed, 2);

      // Perform the integration into whatever box is contained within.
      Counts signal_fit(numSteps);
      signal_fit = 0;

      ws->getBox()->integrateCylinder(cylinder, static_cast<coord_t>(PeakRadius[0]),
                                      static_cast<coord_t>(cylinderLength), signal, errorSquared,
                                      signal_fit.mutableRawData());

      // Integrate around the background radius
      if (BackgroundOuterRadius[0] > PeakRadius[0]) {
        // Get the total signal inside "BackgroundOuterRadius"
        signal_fit = 0;

        ws->getBox()->integrateCylinder(cylinder, static_cast<coord_t>(BackgroundOuterRadius[0]),
                                        static_cast<coord_t>(cylinderLength), bgSignal, bgErrorSquared,
                                        signal_fit.mutableRawData());

        Points points(signal_fit.size(), LinearGenerator(0, 1));
        wsProfile2D->setHistogram(i, points, signal_fit);

        // Evaluate the signal inside "BackgroundInnerRadius"
        signal_t interiorSignal = 0;
        signal_t interiorErrorSquared = 0;

        // Integrate this 3rd radius, if needed
        if (BackgroundInnerRadius[0] != PeakRadius[0]) {
          ws->getBox()->integrateCylinder(cylinder, static_cast<coord_t>(BackgroundInnerRadius[0]),
                                          static_cast<coord_t>(cylinderLength), interiorSignal, interiorErrorSquared,
                                          signal_fit.mutableRawData());
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
        const double interiorRatio = (BackgroundInnerRadius[0] / BackgroundOuterRadius[0]);
        // Volume of the bg shell, relative to the volume of the
        // BackgroundOuterRadius cylinder
        const double bgVolume = 1.0 - interiorRatio * interiorRatio * cylinderLength;

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
            background_total = background_total + wsProfile2D->y(i)[j];
          else
            signal = signal + wsProfile2D->y(i)[j];
        }
        errorSquared = std::fabs(signal);
      } else {

        auto fitAlgorithm = createChildAlgorithm("Fit", -1, -1, false);
        // fitAlgorithm->setProperty("CreateOutput", true);
        // fitAlgorithm->setProperty("Output", "FitPeaks1D");
        std::string myFunc = std::string("name=LinearBackground;name=") + profileFunction;
        auto maxPeak = std::max_element(signal_fit.begin(), signal_fit.end());

        std::ostringstream strs;
        strs << maxPeak[0];
        std::string strMax = strs.str();
        if (profileFunction == "Gaussian") {
          myFunc += ", PeakCentre=50, Height=" + strMax;
          fitAlgorithm->setProperty("Constraints", "40<f1.PeakCentre<60");
        } else if (profileFunction == "BackToBackExponential" || profileFunction == "IkedaCarpenterPV") {
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
          out << std::setw(20) << std::fixed << std::setprecision(10) << ifun->getParameter(j) << " ";
        }
        double chi2 = fitAlgorithm->getProperty("OutputChi2overDoF");
        out << std::setw(20) << std::fixed << std::setprecision(10) << chi2 << "\n";

        std::shared_ptr<const CompositeFunction> fun = std::dynamic_pointer_cast<const CompositeFunction>(ifun);

        const auto &x = wsProfile2D->x(i);
        wsFit2D->setSharedX(i, wsProfile2D->sharedX(i));
        wsDiff2D->setSharedX(i, wsProfile2D->sharedX(i));

        FunctionDomain1DVector domain(x.rawData());
        FunctionValues yy(domain);
        fun->function(domain, yy);
        auto funcValues = yy.toVector();

        wsFit2D->mutableY(i) = funcValues;
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

          gsl_integration_qags(&F, x[peakMin], x[peakMax], 0, 1e-7, 1000, w, &signal, &error);

          gsl_integration_workspace_free(w);
        }
        errorSquared = std::fabs(signal);
        // Get background counts
        for (size_t j = 0; j < numSteps; j++) {
          double background = ifun->getParameter(0) + ifun->getParameter(1) * x[j];
          if (j < peakMin || j > peakMax)
            background_total = background_total + background;
        }
      }
    }
    checkOverlap(i, peakWS, CoordinatesToUse, 2.0 * std::max(PeakRadiusVector[i], BackgroundOuterRadiusVector[i]));
    // Save it back in the peak object.
    if (signal != 0. || replaceIntensity) {
      double edgeMultiplier = 1.0;
      double peakMultiplier = 1.0;
      if (correctEdge) {
        if (edgeDist < BackgroundOuterRadius[0]) {
          double e1 = BackgroundOuterRadius[0] - edgeDist;
          // volume of cap of sphere with h = edge
          double f1 = M_PI * std::pow(e1, 2) / 3 * (3 * BackgroundOuterRadius[0] - e1);
          edgeMultiplier = volumeBkg / (volumeBkg - f1);
        }
        if (edgeDist < PeakRadius[0]) {
          double sigma = PeakRadius[0] / 3.0;
          // assume gaussian peak
          double e1 = std::exp(-std::pow(edgeDist, 2) / (2 * sigma * sigma)) * PeakRadius[0];
          // volume of cap of sphere with h = edge
          double f1 = M_PI * std::pow(e1, 2) / 3 * (3 * PeakRadius[0] - e1);
          peakMultiplier = volumeRadius / (volumeRadius - f1);
        }
      }

      p.setIntensity(peakMultiplier * signal - edgeMultiplier * (ratio * background_total + bgSignal));
      p.setSigmaIntensity(sqrt(peakMultiplier * errorSquared +
                               edgeMultiplier * (ratio * ratio * std::fabs(background_total) + bgErrorSquared)));
    }

    g_log.information() << "Peak " << i << " at " << pos << ": signal " << signal << " (sig^2 " << errorSquared
                        << "), with background " << bgSignal + ratio * background_total << " (sig^2 "
                        << bgErrorSquared + ratio * ratio * std::fabs(background_total) << ") subtracted.\n";
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  // This flag is used by the PeaksWorkspace to evaluate whether it has
  // been integrated.
  peakWS->mutableRun().addProperty("PeaksIntegrated", 1, true);
  // These flags are specific to the algorithm.
  peakWS->mutableRun().addProperty("PeakRadius", PeakRadiusVector, true);
  peakWS->mutableRun().addProperty("BackgroundInnerRadius", BackgroundInnerRadiusVector, true);
  peakWS->mutableRun().addProperty("BackgroundOuterRadius", BackgroundOuterRadiusVector, true);

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
 *  @param useCentroid    bool to use estimated centroid in variance calc
 *  @param bgDensity      background counts per unit volume
 *  @param eigenvects     eigenvectors of covariance matrix of spherical region
 *  @param eigenvals      eigenvectors of covariance matrix of spherical region
 *  @param mean           container to hold centroid (fixed at pos if not using)
 *  @param maxIter        max number of iterations in covariance determination
 */
template <typename MDE, size_t nd>
void IntegratePeaksMD2::findEllipsoid(const typename MDEventWorkspace<MDE, nd>::sptr ws,
                                      const CoordTransform &getRadiusSq, const V3D &pos, const coord_t &radiusSquared,
                                      const bool &qAxisIsFixed, const bool &useCentroid, const double &bgDensity,
                                      std::vector<V3D> &eigenvects, std::vector<double> &eigenvals, V3D &mean,
                                      const int maxIter) {

  // get leaf-only iterators over all boxes in ws
  auto function = std::make_unique<Geometry::MDAlgorithms::MDBoxMaskFunction>(pos, radiusSquared);
  MDBoxBase<MDE, nd> *baseBox = ws->getBox();
  MDBoxIterator<MDE, nd> MDiter(baseBox, 1000, true, function.get());

  // get initial vector of events inside sphere
  std::vector<std::pair<V3D, double>> peak_events;

  do {
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(MDiter.getBox());
    if (box && !box->getIsMasked()) {

      // simple check whether box is defintely not contained
      coord_t boxCenter[nd];
      box->getCenter(boxCenter);
      V3D displacement;   // vector between peak pos and box center
      coord_t rboxSq = 0; // dist from center to vertex sq
      for (size_t d = 0; d < nd; ++d) {
        auto dim = box->getExtents(d);
        rboxSq += static_cast<coord_t>(0.25 * dim.getSize() * dim.getSize());
        displacement[d] = pos[d] - static_cast<double>(boxCenter[d]);
      }

      if (displacement.norm() < static_cast<double>(sqrt(rboxSq)) + static_cast<double>(sqrt(radiusSquared))) {
        // box MIGHT intersect peak spherical region so go through events
        const std::vector<MDE> &events = box->getConstEvents();
        auto bg = bgDensity / (static_cast<double>(events.size()) * (box->getInverseVolume()));
        // For each event
        for (const auto &evnt : events) {

          coord_t center_array[nd];
          for (size_t d = 0; d < nd; ++d) {
            center_array[d] = evnt.getCenter(d);
          }
          coord_t out[1];
          auto *cen_ptr = center_array; // pointer to first element
          getRadiusSq.apply(cen_ptr, out);

          if (evnt.getSignal() > bg && out[0] < radiusSquared) {
            // need in V3D for matrix maths later
            V3D center;
            for (size_t d = 0; d < nd; ++d) {
              center[d] = static_cast<double>(center_array[d]);
            }
            peak_events.emplace_back(center, evnt.getSignal() - bg);
          }
        }
      }
    }
    box->releaseEvents();
  } while (MDiter.next());
  calcCovar(peak_events, pos, radiusSquared, qAxisIsFixed, useCentroid, eigenvects, eigenvals, mean, maxIter);
}

/**
 * Calculate the covariance matrix of a spherical region and store the
 * eigenvectors and eigenvalues that diagonalise the covariance matrix in the
 * vectors provided
 *
 *  @param peak_events    container of center and signal of events in sphere
 *  @param pos            V3D of nominal peak centre
 *  @param radiusSquared  radius squared of spherical region for covarariance
 *  @param qAxisIsFixed   bool to fix an eigenvector along direction pos
 *  @param useCentroid    bool to use estimated centroid in variance calc
 *  @param eigenvects     eigenvectors of covariance matrix of spherical region
 *  @param eigenvals      eigenvalues of covariance matrix of spherical region
 *  @param mean           container to hold centroid (fixed at pos if not using)
 *  @param maxIter        max number of iterations in covariance determination
 */
void IntegratePeaksMD2::calcCovar(const std::vector<std::pair<V3D, double>> &peak_events, const V3D &pos,
                                  const coord_t &radiusSquared, const bool &qAxisIsFixed, const bool &useCentroid,
                                  std::vector<V3D> &eigenvects, std::vector<double> &eigenvals, V3D &mean,
                                  const int maxIter) {

  size_t nd = 3;

  // to calc threshold mdsq to exclude events over 3 stdevs away
  boost::math::chi_squared chisq(static_cast<double>(nd));
  auto mdsq_max = boost::math::quantile(chisq, 0.997);
  Matrix<double> invCov; // required to calc mdsq
  double prev_cov_det = DBL_MAX;

  // initialise mean with pos
  mean = pos;
  Matrix<double> Pinv(nd, nd);
  if (qAxisIsFixed) {
    // transformation from Qlab to Qhat, vhat and uhat,
    getPinv(pos, Pinv);
    mean = Pinv * mean;
  }
  Matrix<double> cov_mat(nd, nd);

  for (int nIter = 0; nIter < maxIter; nIter++) {

    // reset on each loop
    cov_mat.zeroMatrix();
    double w_sum = 0;   // sum of weights
    size_t nmasked = 0; // num masked events outside 3stdevs
    auto prev_pos = mean;

    for (size_t ievent = 0; ievent < peak_events.size(); ievent++) {

      const auto event = peak_events[ievent];
      auto center = event.first;
      if (qAxisIsFixed) {
        // transform coords to Q, uhat, vhat basis
        center = Pinv * center;
      }

      bool useEvent = true;
      if (nIter > 0) {
        // check if point within 3 stdevs of mean (in MD frame)
        // prev_pos is the mean if useCentroid
        const auto displ = center - prev_pos;
        auto mdsq = displ.scalar_prod(invCov * displ);
        if (mdsq > mdsq_max) {
          // exclude points outside 3 stdevs
          useEvent = false;
          nmasked += 1;
        }
      }

      // if prev cov_mat chec
      if (useEvent) {
        const auto signal = event.second;
        w_sum += signal;

        if (useCentroid) {
          // update mean
          mean += (center - mean) * (signal / w_sum);
        }

        // weight for variance
        auto wi = signal * (w_sum - signal) / w_sum;
        size_t istart = 0;
        if (qAxisIsFixed) {
          // variance along Q (skipped in next nested loops below)
          cov_mat[0][0] += wi * pow((center[0] - mean[0]), 2);
          istart = 1;
        }
        for (size_t row = istart; row < cov_mat.numRows(); ++row) {
          for (size_t col = istart; col < cov_mat.numRows(); ++col) {
            // symmeteric matrix
            if (row <= col) {
              auto cov = wi * (center[row] - mean[row]) * (center[col] - mean[col]);
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
    // normalise the covariance matrix
    cov_mat /= w_sum; // normalise by sum of weights

    // check if another iteration is required
    bool anyMasked = (nIter > 0) ? (nmasked > 0) : true;
    // check if ellipsoid volume greater than sphere
    auto cov_det = cov_mat.determinant();
    bool isEllipVolGreater = cov_det > pow(static_cast<double>(radiusSquared / 9), 3);
    // check for convergence of variances
    bool isConverged = (cov_det > 0.95 * prev_cov_det);

    if (!anyMasked || isEllipVolGreater || isConverged) {
      break;
    } else {
      prev_cov_det = cov_det;
      // required to eval Mahalanobis distance
      invCov = Matrix<double>(cov_mat);
      invCov.Invert();
    }
  }

  if (qAxisIsFixed) {
    // transform back to MD basis
    Matrix<double> P(Pinv);
    P.Transpose();
    mean = P * mean;
    cov_mat = P * cov_mat * Pinv;
  }
  Matrix<double> evecs; // hold eigenvectors
  Matrix<double> evals; // hold eigenvals in diag
  cov_mat.Diagonalise(evecs, evals);

  auto min_eval = evals[0][0];
  for (size_t d = 1; d < nd; ++d) {
    min_eval = std::min(min_eval, evals[d][d]);
  }
  if (min_eval > static_cast<double>(radiusSquared / 9)) {
    // haven't found good covar - set to spherical region
    evals.identityMatrix();
    evals = evals * (static_cast<double>(radiusSquared) / 9);
    g_log.warning() << "Covariance of peak at ";
    pos.printSelf(g_log.warning());
    g_log.warning() << " is not well constrained, it has been set to spherical" << std::endl;
  }

  // convert to vectors for output
  eigenvals = evals.Diagonal();
  // set min eigenval to be small but non-zero (1e-6)
  // when no discernible peak above background
  std::replace_if(eigenvals.begin(), eigenvals.end(), [&](auto x) { return x < 1e-6; }, 1e-6);

  // populate V3D vector of eigenvects (needed for ellipsoid shape)
  eigenvects = std::vector<V3D>(nd);
  for (size_t ivect = 0; ivect < nd; ++ivect) {
    eigenvects[ivect] = V3D(evecs[0][ivect], evecs[1][ivect], evecs[2][ivect]);
  }
}

/**
 * Get the inverse of the matrix P. Left multiply a vector by Pinv to transform
 * from Qlab to basis Qhat, and uhat,vhat in plane perpendicular to Q. P is a
 * matrix with columns corresponding to new basis vectors. The inverse of P is
 * equivalent to the transpose (as for any roation matrix)
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
void IntegratePeaksMD2::calculateE1(const Geometry::DetectorInfo &detectorInfo) {
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
 */
double IntegratePeaksMD2::calculateDistanceToEdge(const Mantid::Kernel::V3D &QLabFrame) {
  double edgeDist = DBL_MAX;
  for (auto &E1 : E1Vec) {
    V3D distv = QLabFrame - E1 * (QLabFrame.scalar_prod(E1)); // distance to the
                                                              // trajectory as a
                                                              // vector
    edgeDist = std::min(edgeDist, distv.norm());              // want smallest dist to peak
  }
  return edgeDist;
}

void IntegratePeaksMD2::runMaskDetectors(const Mantid::DataObjects::PeaksWorkspace_sptr &peakWS,
                                         const std::string &property, const std::string &values) {
  // For CORELLI do not count as edge if next to another detector bank
  if (property == "Tube" && peakWS->getInstrument()->getName() == "CORELLI") {
    auto alg = createChildAlgorithm("MaskBTP");
    alg->setProperty<Workspace_sptr>("Workspace", peakWS);
    alg->setProperty("Bank", "1,7,12,17,22,27,30,59,63,69,74,79,84,89");
    alg->setProperty(property, "1");
    if (!alg->execute())
      throw std::runtime_error("MaskDetectors Child Algorithm has not executed successfully");
    auto alg2 = createChildAlgorithm("MaskBTP");
    alg2->setProperty<Workspace_sptr>("Workspace", peakWS);
    alg2->setProperty("Bank", "6,11,16,21,26,29,58,62,68,73,78,83,88,91");
    alg2->setProperty(property, "16");
    if (!alg2->execute())
      throw std::runtime_error("MaskDetectors Child Algorithm has not executed successfully");
  } else {
    auto alg = createChildAlgorithm("MaskBTP");
    alg->setProperty<Workspace_sptr>("Workspace", peakWS);
    alg->setProperty(property, values);
    if (!alg->execute())
      throw std::runtime_error("MaskDetectors Child Algorithm has not executed successfully");
  }
}

void IntegratePeaksMD2::checkOverlap(int i, const IPeaksWorkspace_sptr &peakWS,
                                     Mantid::Kernel::SpecialCoordinateSystem CoordinatesToUse, double radius) {
  // Get a direct ref to that peak.
  const IPeak &p1 = peakWS->getPeak(i);
  V3D pos1;
  if (CoordinatesToUse == Kernel::QLab) //"Q (lab frame)"
    pos1 = p1.getQLabFrame();
  else if (CoordinatesToUse == Kernel::QSample) //"Q (sample frame)"
    pos1 = p1.getQSampleFrame();
  else if (CoordinatesToUse == Kernel::HKL) //"HKL"
    pos1 = p1.getHKL();
  for (int j = i + 1; j < peakWS->getNumberPeaks(); ++j) {
    // Get a direct ref to rest of peaks peak.
    const IPeak &p2 = peakWS->getPeak(j);
    V3D pos2;
    if (CoordinatesToUse == Kernel::QLab) //"Q (lab frame)"
      pos2 = p2.getQLabFrame();
    else if (CoordinatesToUse == Kernel::QSample) //"Q (sample frame)"
      pos2 = p2.getQSampleFrame();
    else if (CoordinatesToUse == Kernel::HKL) //"HKL"
      pos2 = p2.getHKL();
    if (pos1.distance(pos2) < radius) {
      g_log.warning() << " Warning:  Peak integration spheres for peaks " << i << " and " << j
                      << " overlap.  Distance between peaks is " << pos1.distance(pos2) << '\n';
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
      *reinterpret_cast<std::shared_ptr<const API::CompositeFunction> *>(params);
  FunctionDomain1DVector domain(x);
  FunctionValues yval(domain);
  fun->function(domain, yval);
  return yval[0];
}

} // namespace Mantid::MDAlgorithms
