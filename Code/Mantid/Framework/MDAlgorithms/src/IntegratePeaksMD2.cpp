#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDAlgorithms/GSLFunctions.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDAlgorithms/IntegratePeaksMD2.h"
#include "MantidMDEvents/CoordTransformDistance.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/Utils.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/Progress.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <gsl/gsl_integration.h>
#include <fstream>

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksMD2)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IntegratePeaksMD2::IntegratePeaksMD2() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IntegratePeaksMD2::~IntegratePeaksMD2() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegratePeaksMD2::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input MDEventWorkspace.");

  std::vector<std::string> propOptions;
  propOptions.push_back("Q (lab frame)");
  propOptions.push_back("Q (sample frame)");
  propOptions.push_back("HKL");

  declareProperty(
      new PropertyWithValue<double>("PeakRadius", 1.0, Direction::Input),
      "Fixed radius around each peak position in which to integrate (in the "
      "same units as the workspace).");

  declareProperty(
      new PropertyWithValue<double>("BackgroundInnerRadius", 0.0,
                                    Direction::Input),
      "Inner radius to use to evaluate the background of the peak.\n"
      "If smaller than PeakRadius, then we assume BackgroundInnerRadius = "
      "PeakRadius.");

  declareProperty(
      new PropertyWithValue<double>("BackgroundOuterRadius", 0.0,
                                    Direction::Input),
      "Outer radius to use to evaluate the background of the peak.\n"
      "The signal density around the peak (BackgroundInnerRadius < r < "
      "BackgroundOuterRadius) is used to estimate the background under the "
      "peak.\n"
      "If smaller than PeakRadius, no background measurement is done.");

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace", "",
                                                        Direction::Input),
                  "A PeaksWorkspace containing the peaks to integrate.");

  declareProperty(
      new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
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

  declareProperty("AdaptiveQRadius", false,
                  "Default is false.   If true, all input radii are multiplied "
                  "by the magnitude of Q at the peak center so each peak has a "
                  "different integration radius.  Q includes the 2*pi factor.");

  declareProperty("Cylinder", false,
                  "Default is sphere.  Use next five parameters for cylinder.");

  declareProperty(
      new PropertyWithValue<double>("CylinderLength", 0.0, Direction::Input),
      "Length of cylinder in which to integrate (in the same units as the "
      "workspace).");

  declareProperty(
      new PropertyWithValue<double>("PercentBackground", 0.0, Direction::Input),
      "Percent of CylinderLength that is background (20 is 20%)");

  std::vector<std::string> peakNames =
      FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
  peakNames.push_back("NoFit");
  declareProperty("ProfileFunction", "Gaussian",
                  boost::make_shared<StringListValidator>(peakNames),
                  "Fitting function for profile that is used only with "
                  "Cylinder integration.");

  std::vector<std::string> integrationOptions(2);
  integrationOptions[0] = "Sum";
  integrationOptions[1] = "GaussianQuadrature";
  auto integrationvalidator =
      boost::make_shared<StringListValidator>(integrationOptions);
  declareProperty("IntegrationOption", "GaussianQuadrature",
                  integrationvalidator,
                  "Integration method for calculating intensity "
                  "used only with Cylinder integration.");

  declareProperty(
      new FileProperty("ProfilesFile", "", FileProperty::OptionalSave,
                       std::vector<std::string>(1, "profiles")),
      "Save (Optionally) as Isaw peaks file with profiles included");
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
    runMaskDetectors(peakWS, "Tube", "edges");
    runMaskDetectors(peakWS, "Pixel", "edges");
  } catch (...) {
    g_log.error("Can't execute MaskBTP algorithm for this instrument to set "
                "edge for IntegrateIfOnEdge option");
  }

  // Get the instrument and its detectors
  inst = peakWS->getInstrument();
  int CoordinatesToUse = ws->getSpecialCoordinateSystem();

  /// Radius to use around peaks
  double PeakRadius = getProperty("PeakRadius");
  /// Background (end) radius
  double BackgroundOuterRadius = getProperty("BackgroundOuterRadius");
  /// Start radius of the background
  double BackgroundInnerRadius = getProperty("BackgroundInnerRadius");
  if (BackgroundInnerRadius < PeakRadius)
    BackgroundInnerRadius = PeakRadius;
  /// Cylinder Length to use around peaks for cylinder
  double cylinderLength = getProperty("CylinderLength");
  Workspace2D_sptr wsProfile2D, wsFit2D, wsDiff2D;
  size_t numSteps = 0;
  bool cylinderBool = getProperty("Cylinder");
  bool adaptiveQRadius = getProperty("AdaptiveQRadius");
  std::vector<double> PeakRadiusVector(peakWS->getNumberPeaks(), PeakRadius);
  std::vector<double> BackgroundInnerRadiusVector(peakWS->getNumberPeaks(),
                                                  BackgroundInnerRadius);
  std::vector<double> BackgroundOuterRadiusVector(peakWS->getNumberPeaks(),
                                                  BackgroundOuterRadius);
  if (cylinderBool) {
    numSteps = 100;
    size_t histogramNumber = peakWS->getNumberPeaks();
    Workspace_sptr wsProfile = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, numSteps, numSteps);
    wsProfile2D = boost::dynamic_pointer_cast<Workspace2D>(wsProfile);
    AnalysisDataService::Instance().addOrReplace("ProfilesData", wsProfile2D);
    Workspace_sptr wsFit = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, numSteps, numSteps);
    wsFit2D = boost::dynamic_pointer_cast<Workspace2D>(wsFit);
    AnalysisDataService::Instance().addOrReplace("ProfilesFit", wsFit2D);
    Workspace_sptr wsDiff = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, numSteps, numSteps);
    wsDiff2D = boost::dynamic_pointer_cast<Workspace2D>(wsDiff);
    AnalysisDataService::Instance().addOrReplace("ProfilesFitDiff", wsDiff2D);
    TextAxis *const newAxis1 = new TextAxis(peakWS->getNumberPeaks());
    TextAxis *const newAxis2 = new TextAxis(peakWS->getNumberPeaks());
    TextAxis *const newAxis3 = new TextAxis(peakWS->getNumberPeaks());
    wsProfile2D->replaceAxis(1, newAxis1);
    wsFit2D->replaceAxis(1, newAxis2);
    wsDiff2D->replaceAxis(1, newAxis3);
    for (int i = 0; i < peakWS->getNumberPeaks(); ++i) {
      // Get a direct ref to that peak.
      IPeak &p = peakWS->getPeak(i);
      std::ostringstream label;
      label << Utils::round(p.getH()) << "_" << Utils::round(p.getK()) << "_"
            << Utils::round(p.getL()) << "_" << p.getRunNumber();
      newAxis1->setLabel(i, label.str());
      newAxis2->setLabel(i, label.str());
      newAxis3->setLabel(i, label.str());
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

  std::string profileFunction = getProperty("ProfileFunction");
  std::string integrationOption = getProperty("IntegrationOption");
  std::ofstream out;
  if (cylinderBool && profileFunction.compare("NoFit") != 0) {
    std::string outFile = getProperty("InputWorkspace");
    outFile.append(profileFunction);
    outFile.append(".dat");
    std::string save_path =
        ConfigService::Instance().getString("defaultsave.directory");
    outFile = save_path + outFile;
    out.open(outFile.c_str(), std::ofstream::out);
  }
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
    if (CoordinatesToUse == 1) //"Q (lab frame)"
      pos = p.getQLabFrame();
    else if (CoordinatesToUse == 2) //"Q (sample frame)"
      pos = p.getQSampleFrame();
    else if (CoordinatesToUse == 3) //"HKL"
      pos = p.getHKL();

    // Do not integrate if sphere is off edge of detector

    if (!detectorQ(p.getQLabFrame(),
                   std::max(BackgroundOuterRadius, PeakRadius))) {
      g_log.warning() << "Warning: sphere/cylinder for integration is off edge "
                         "of detector for peak " << i << std::endl;
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
      coord_t lenQpeak = 1.0;
      if (adaptiveQRadius) {
        lenQpeak = 0.0;
        for (size_t d = 0; d < nd; d++) {
          lenQpeak += center[d] * center[d];
        }
        lenQpeak = std::sqrt(lenQpeak);
      }
      PeakRadiusVector[i] = lenQpeak * PeakRadius;
      BackgroundInnerRadiusVector[i] = lenQpeak * BackgroundInnerRadius;
      BackgroundOuterRadiusVector[i] = lenQpeak * BackgroundOuterRadius;
      CoordTransformDistance sphere(nd, center, dimensionsUsed);

      // Perform the integration into whatever box is contained within.
      ws->getBox()->integrateSphere(
          sphere,
          static_cast<coord_t>(lenQpeak * PeakRadius * lenQpeak * PeakRadius),
          signal, errorSquared);

      // Integrate around the background radius

      if (BackgroundOuterRadius > PeakRadius) {
        // Get the total signal inside "BackgroundOuterRadius"
        ws->getBox()->integrateSphere(
            sphere, static_cast<coord_t>(lenQpeak * BackgroundOuterRadius *
                                         lenQpeak * BackgroundOuterRadius),
            bgSignal, bgErrorSquared);

        // Evaluate the signal inside "BackgroundInnerRadius"
        signal_t interiorSignal = 0;
        signal_t interiorErrorSquared = 0;

        // Integrate this 3rd radius, if needed
        if (BackgroundInnerRadius != PeakRadius) {
          ws->getBox()->integrateSphere(
              sphere, static_cast<coord_t>(lenQpeak * BackgroundInnerRadius *
                                           lenQpeak * BackgroundInnerRadius),
              interiorSignal, interiorErrorSquared);
        } else {
          // PeakRadius == BackgroundInnerRadius, so use the previous value
          interiorSignal = signal;
          interiorErrorSquared = errorSquared;
        }
        // Subtract the peak part to get the intensity in the shell
        // (BackgroundInnerRadius < r < BackgroundOuterRadius)
        bgSignal -= interiorSignal;
        // We can subtract the error (instead of adding) because the two values
        // are 100% dependent; this is the same as integrating a shell.
        bgErrorSquared -= interiorErrorSquared;

        // Relative volume of peak vs the BackgroundOuterRadius sphere
        double ratio = (PeakRadius / BackgroundOuterRadius);
        double peakVolume = ratio * ratio * ratio;

        // Relative volume of the interior of the shell vs overall background
        double interiorRatio = (BackgroundInnerRadius / BackgroundOuterRadius);
        // Volume of the bg shell, relative to the volume of the
        // BackgroundOuterRadius sphere
        double bgVolume = 1.0 - interiorRatio * interiorRatio * interiorRatio;

        // Finally, you will multiply the bg intensity by this to get the
        // estimated background under the peak volume
        double scaleFactor = peakVolume / bgVolume;
        bgSignal *= scaleFactor;
        bgErrorSquared *= scaleFactor * scaleFactor;
      }
    } else {
      CoordTransformDistance cylinder(nd, center, dimensionsUsed, 2);

      // Perform the integration into whatever box is contained within.
      std::vector<signal_t> signal_fit;

      signal_fit.clear();
      for (size_t j = 0; j < numSteps; j++)
        signal_fit.push_back(0.0);
      ws->getBox()->integrateCylinder(cylinder,
                                      static_cast<coord_t>(PeakRadius),
                                      static_cast<coord_t>(cylinderLength),
                                      signal, errorSquared, signal_fit);
      for (size_t j = 0; j < numSteps; j++) {
        wsProfile2D->dataX(i)[j] = static_cast<double>(j);
        wsProfile2D->dataY(i)[j] = signal_fit[j];
        wsProfile2D->dataE(i)[j] = std::sqrt(signal_fit[j]);
      }

      // Integrate around the background radius
      if (BackgroundOuterRadius > PeakRadius) {
        // Get the total signal inside "BackgroundOuterRadius"

        signal_fit.clear();
        for (size_t j = 0; j < numSteps; j++)
          signal_fit.push_back(0.0);
        ws->getBox()->integrateCylinder(
            cylinder, static_cast<coord_t>(BackgroundOuterRadius),
            static_cast<coord_t>(cylinderLength), bgSignal, bgErrorSquared,
            signal_fit);
        for (size_t j = 0; j < numSteps; j++) {
          wsProfile2D->dataX(i)[j] = static_cast<double>(j);
          wsProfile2D->dataY(i)[j] = signal_fit[j];
          wsProfile2D->dataE(i)[j] = std::sqrt(signal_fit[j]);
        }

        // Evaluate the signal inside "BackgroundInnerRadius"
        signal_t interiorSignal = 0;
        signal_t interiorErrorSquared = 0;

        // Integrate this 3rd radius, if needed
        if (BackgroundInnerRadius != PeakRadius) {
          ws->getBox()->integrateCylinder(
              cylinder, static_cast<coord_t>(BackgroundInnerRadius),
              static_cast<coord_t>(cylinderLength), interiorSignal,
              interiorErrorSquared, signal_fit);
        } else {
          // PeakRadius == BackgroundInnerRadius, so use the previous value
          interiorSignal = signal;
          interiorErrorSquared = errorSquared;
        }
        // Subtract the peak part to get the intensity in the shell
        // (BackgroundInnerRadius < r < BackgroundOuterRadius)
        bgSignal -= interiorSignal;
        // We can subtract the error (instead of adding) because the two values
        // are 100% dependent; this is the same as integrating a shell.
        bgErrorSquared -= interiorErrorSquared;
        // Relative volume of peak vs the BackgroundOuterRadius cylinder
        double ratio = (PeakRadius / BackgroundOuterRadius);
        double peakVolume = ratio * ratio * cylinderLength;

        // Relative volume of the interior of the shell vs overall background
        double interiorRatio = (BackgroundInnerRadius / BackgroundOuterRadius);
        // Volume of the bg shell, relative to the volume of the
        // BackgroundOuterRadius cylinder
        double bgVolume = 1.0 - interiorRatio * interiorRatio * cylinderLength;

        // Finally, you will multiply the bg intensity by this to get the
        // estimated background under the peak volume
        double scaleFactor = peakVolume / bgVolume;
        bgSignal *= scaleFactor;
        bgErrorSquared *= scaleFactor * scaleFactor;
      } else {
        for (size_t j = 0; j < numSteps; j++) {
          wsProfile2D->dataX(i)[j] = static_cast<double>(j);
          wsProfile2D->dataY(i)[j] = signal_fit[j];
          wsProfile2D->dataE(i)[j] = std::sqrt(signal_fit[j]);
        }
      }

      if (profileFunction.compare("NoFit") == 0) {
        signal = 0.;
        for (size_t j = 0; j < numSteps; j++) {
          if (j < peakMin || j > peakMax)
            background_total = background_total + wsProfile2D->dataY(i)[j];
          else
            signal = signal + wsProfile2D->dataY(i)[j];
        }
        errorSquared = std::fabs(signal);
      } else {
        API::IAlgorithm_sptr findpeaks =
            createChildAlgorithm("FindPeaks", -1, -1, false);
        findpeaks->setProperty("InputWorkspace", wsProfile2D);
        findpeaks->setProperty<int>("FWHM", 7);
        findpeaks->setProperty<int>("Tolerance", 4);
        // FindPeaks will do the checking on the validity of WorkspaceIndex
        findpeaks->setProperty("WorkspaceIndex", static_cast<int>(i));

        // Get the specified peak positions, which is optional
        findpeaks->setProperty<std::string>("PeakFunction", profileFunction);
        // FindPeaks will use linear or flat if they are better
        findpeaks->setProperty<std::string>("BackgroundType", "Quadratic");
        findpeaks->setProperty<bool>("HighBackground", true);
        findpeaks->setProperty<bool>("RawPeakParameters", true);
        std::vector<double> peakPosToFit;
        peakPosToFit.push_back(static_cast<double>(numSteps / 2));
        findpeaks->setProperty("PeakPositions", peakPosToFit);
        findpeaks->setProperty<int>("MinGuessedPeakWidth", 4);
        findpeaks->setProperty<int>("MaxGuessedPeakWidth", 4);
        try {
          findpeaks->executeAsChildAlg();
        } catch (...) {
          g_log.error("Can't execute FindPeaks algorithm");
          continue;
        }

        API::ITableWorkspace_sptr paramws = findpeaks->getProperty("PeaksList");
        if (paramws->rowCount() < 1)
          continue;
        std::ostringstream fun_str;
        fun_str << "name=" << profileFunction;

        size_t numcols = paramws->columnCount();
        std::vector<std::string> paramsName = paramws->getColumnNames();
        std::vector<double> paramsValue;
        API::TableRow row = paramws->getRow(0);
        int spectrum;
        row >> spectrum;
        for (size_t j = 1; j < numcols; ++j) {
          double parvalue;
          row >> parvalue;
          if (j == numcols - 4)
            fun_str << ";name=Quadratic";
          // erase f0. or f1.
          // if (j > 0 && j < numcols-1) fun_str << "," <<
          // paramsName[j].erase(0,3) <<"="<<parvalue;
          if (j > 0 && j < numcols - 1)
            fun_str << "," << paramsName[j] << "=" << parvalue;
          paramsValue.push_back(parvalue);
        }
        if (i == 0) {
          for (size_t j = 0; j < numcols; ++j)
            out << std::setw(20) << paramsName[j] << " ";
          out << "\n";
        }
        out << std::setw(20) << i;
        for (size_t j = 0; j < numcols - 1; ++j)
          out << std::setw(20) << std::fixed << std::setprecision(10)
              << paramsValue[j] << " ";
        out << "\n";

        // Evaluate fit at points

        IFunction_sptr ifun =
            FunctionFactory::Instance().createInitialized(fun_str.str());
        boost::shared_ptr<const CompositeFunction> fun =
            boost::dynamic_pointer_cast<const CompositeFunction>(ifun);
        const Mantid::MantidVec &x = wsProfile2D->readX(i);
        wsFit2D->dataX(i) = x;
        wsDiff2D->dataX(i) = x;
        FunctionDomain1DVector domain(x);
        FunctionValues yy(domain);
        fun->function(domain, yy);
        const Mantid::MantidVec &yValues = wsProfile2D->readY(i);
        for (size_t j = 0; j < numSteps; j++) {
          wsFit2D->dataY(i)[j] = yy[j];
          wsDiff2D->dataY(i)[j] = yValues[j] - yy[j];
        }

        // Calculate intensity
        signal = 0.0;
        if (integrationOption.compare("Sum") == 0) {
          for (size_t j = peakMin; j <= peakMax; j++)
            if (!boost::math::isnan(yy[j]) && !boost::math::isinf(yy[j]))
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
          // paramsValue[numcols-2] is chisq
          double background = paramsValue[numcols - 3] * x[j] * x[j] +
                              paramsValue[numcols - 4] * x[j] +
                              paramsValue[numcols - 5];
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
      p.setIntensity(signal - ratio * background_total - bgSignal);
      p.setSigmaIntensity(sqrt(errorSquared +
                               ratio * ratio * std::fabs(background_total) +
                               bgErrorSquared));
    }

    g_log.information() << "Peak " << i << " at " << pos << ": signal "
                        << signal << " (sig^2 " << errorSquared
                        << "), with background "
                        << bgSignal + ratio * background_total << " (sig^2 "
                        << bgErrorSquared +
                               ratio * ratio * std::fabs(background_total)
                        << ") subtracted." << std::endl;
  }
  // This flag is used by the PeaksWorkspace to evaluate whether it has been
  // integrated.
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

/** Calculate if this Q is on a detector
 * Define edges for each instrument by masking. For CORELLI, tubes 1 and 16, and
 *pixels 0 and 255.
 * Get Q in the lab frame for every peak, call it C
 * For every point on the edge, the trajectory in reciprocal space is a straight
 *line, going through O=V3D(0,0,0).
 * Calculate a point at a fixed momentum, say k=1. Q in the lab frame
 *E=V3D(-k*sin(tt)*cos(ph),-k*sin(tt)*sin(ph),k-k*cos(ph)).
 * Normalize E to 1: E=E*(1./E.norm())
 * The distance from C to OE is given by dv=C-E*(C.scalar_prod(E))
 * If dv.norm<integration_radius, one of the detector trajectories on the edge
 *is too close to the peak
 * This method is applied to all masked pixels. If there are masked pixels
 *trajectories inside an integration volume, the peak must be rejected.
 *
 * @param QLabFrame: The Peak center.
 * @param r: Peak radius.
 */
bool IntegratePeaksMD2::detectorQ(Mantid::Kernel::V3D QLabFrame, double r) {
  std::vector<detid_t> detectorIDs = inst->getDetectorIDs();

  for (auto detID = detectorIDs.begin(); detID != detectorIDs.end(); ++detID) {
    Mantid::Geometry::IDetector_const_sptr det = inst->getDetector(*detID);
    if (det->isMonitor())
      continue; // skip monitor
    if (!det->isMasked())
      continue; // edge is masked so don't check if not masked
    double tt1 = det->getTwoTheta(V3D(0, 0, 0), V3D(0, 0, 1)); // two theta
    double ph1 = det->getPhi();                                // phi
    V3D E1 = V3D(-std::sin(tt1) * std::cos(ph1), -std::sin(tt1) * std::sin(ph1),
                 1. - std::cos(tt1)); // end of trajectory
    E1 = E1 * (1. / E1.norm());       // normalize
    V3D distv = QLabFrame -
                E1 * (QLabFrame.scalar_prod(
                         E1)); // distance to the trajectory as a vector
    if (distv.norm() < r) {
      return false;
    }
  }

  return true;
}
void IntegratePeaksMD2::runMaskDetectors(
    Mantid::DataObjects::PeaksWorkspace_sptr peakWS, std::string property,
    std::string values) {
  IAlgorithm_sptr alg = createChildAlgorithm("MaskBTP");
  alg->setProperty<Workspace_sptr>("Workspace", peakWS);
  alg->setProperty(property, values);
  if (!alg->execute())
    throw std::runtime_error(
        "MaskDetectors Child Algorithm has not executed successfully");
}

void
IntegratePeaksMD2::checkOverlap(int i,
                                Mantid::DataObjects::PeaksWorkspace_sptr peakWS,
                                int CoordinatesToUse, double radius) {
  // Get a direct ref to that peak.
  IPeak &p1 = peakWS->getPeak(i);
  V3D pos1;
  if (CoordinatesToUse == 1) //"Q (lab frame)"
    pos1 = p1.getQLabFrame();
  else if (CoordinatesToUse == 2) //"Q (sample frame)"
    pos1 = p1.getQSampleFrame();
  else if (CoordinatesToUse == 3) //"HKL"
    pos1 = p1.getHKL();
  for (int j = i + 1; j < peakWS->getNumberPeaks(); ++j) {
    // Get a direct ref to rest of peaks peak.
    IPeak &p2 = peakWS->getPeak(j);
    V3D pos2;
    if (CoordinatesToUse == 1) //"Q (lab frame)"
      pos2 = p2.getQLabFrame();
    else if (CoordinatesToUse == 2) //"Q (sample frame)"
      pos2 = p2.getQSampleFrame();
    else if (CoordinatesToUse == 3) //"HKL"
      pos2 = p2.getHKL();
    if (pos1.distance(pos2) < radius) {
      g_log.warning() << " Warning:  Peak integration spheres for peaks " << i
                      << " and " << j << " overlap.  Distance between peaks is "
                      << pos1.distance(pos2) << std::endl;
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
  boost::shared_ptr<const API::CompositeFunction> fun =
      *(boost::shared_ptr<const API::CompositeFunction> *)params;
  FunctionDomain1DVector domain(x);
  FunctionValues yval(domain);
  fun->function(domain, yval);
  return yval[0];
}

} // namespace Mantid
} // namespace MDEvents
