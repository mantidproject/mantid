//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/PeakIntegration.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <ostream>
#include <iomanip>
#include <sstream>
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <boost/algorithm/string.hpp>
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidAPI/MemoryManager.h"

namespace Mantid {
namespace Crystal {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PeakIntegration)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

/// Constructor
PeakIntegration::PeakIntegration() : API::Algorithm() {}

/// Destructor
PeakIntegration::~PeakIntegration() {}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void PeakIntegration::init() {

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace", "",
                                                        Direction::Input),
                  "Name of the peaks workspace.");
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
                              boost::make_shared<InstrumentValidator>()),
      "A 2D workspace with X values of time of flight");
  declareProperty(
      new WorkspaceProperty<PeaksWorkspace>("OutPeaksWorkspace", "",
                                            Direction::Output),
      "Name of the output peaks workspace with integrated intensities.");
  declareProperty("IkedaCarpenterTOF", false,
                  "Integrate TOF using IkedaCarpenter fit.\n"
                  "Default is false which is best for corrected data.");
  declareProperty("MatchingRunNo", true, "Integrate only peaks where run "
                                         "number of peak matches run number of "
                                         "sample.\n"
                                         "Default is true.");
  declareProperty("NBadEdgePixels", 0, "Number of bad Edge Pixels");
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 *successfully
 */
void PeakIntegration::exec() {
  retrieveProperties();

  /// Input peaks workspace
  PeaksWorkspace_sptr inPeaksW = getProperty("InPeaksWorkspace");

  /// Output peaks workspace, create if needed
  PeaksWorkspace_sptr peaksW = getProperty("OutPeaksWorkspace");
  if (peaksW != inPeaksW)
    peaksW = inPeaksW->clone();

  double qspan = 0.12;
  IC = getProperty("IkedaCarpenterTOF");
  bool matchRun = getProperty("MatchingRunNo");
  if (peaksW->mutableSample().hasOrientedLattice()) {
    OrientedLattice latt = peaksW->mutableSample().getOrientedLattice();
    qspan = 1. /
            std::max(latt.a(), std::max(latt.b(), latt.c())); // 1/6*2Pi about 1

  } else {
    qspan = 0.12;
  }

  // To get the workspace index from the detector ID
  const auto pixel_to_wi = inputW->getDetectorIDToWorkspaceIndexMap();

  // Sort events if EventWorkspace so it will run in parallel
  EventWorkspace_const_sptr inWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputW);
  if (inWS) {
    inWS->sortAll(TOF_SORT, NULL);
  }

  // Get some stuff from the input workspace
  const int YLength = static_cast<int>(inputW->blocksize());
  outputW = API::WorkspaceFactory::Instance().create(
      inputW, peaksW->getNumberPeaks(), YLength + 1, YLength);
  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(inputW, outputW, true);
  size_t Numberwi = inputW->getNumberHistograms();
  int NumberPeaks = peaksW->getNumberPeaks();
  int MinPeaks = 0;

  for (int i = NumberPeaks - 1; i >= 0; i--) {
    Peak &peak = peaksW->getPeaks()[i];
    int pixelID = peak.getDetectorID();

    // Find the workspace index for this detector ID
    auto wiEntry = pixel_to_wi.find(pixelID);
    if (wiEntry != pixel_to_wi.end()) {
      size_t wi = wiEntry->second;
      if ((matchRun && peak.getRunNumber() != inputW->getRunNumber()) ||
          wi >= Numberwi)
        peaksW->removePeak(i);
    } else // This is for appending peak workspaces when running
           // SNSSingleCrystalReduction one bank at at time
        if (i + 1 > MinPeaks)
      MinPeaks = i + 1;
  }
  NumberPeaks = peaksW->getNumberPeaks();
  if (NumberPeaks <= 0) {
    g_log.error(
        "RunNumbers of InPeaksWorkspace and InputWorkspace do not match");
    return;
  }

  Progress prog(this, MinPeaks, 1.0, NumberPeaks);
  PARALLEL_FOR3(inputW, peaksW, outputW)
  for (int i = MinPeaks; i < NumberPeaks; i++) {

    PARALLEL_START_INTERUPT_REGION

    // Direct ref to that peak
    Peak &peak = peaksW->getPeaks()[i];

    double col = peak.getCol();
    double row = peak.getRow();

    // Average integer postion
    int XPeak = int(col + 0.5);
    int YPeak = int(row + 0.5);

    double TOFPeakd = peak.getTOF();
    std::string bankName = peak.getBankName();

    boost::shared_ptr<const IComponent> parent =
        inputW->getInstrument()->getComponentByName(bankName);

    if (!parent)
      continue;

    int TOFPeak = 0, TOFmin = 0, TOFmax = 0;
    TOFmax =
        fitneighbours(i, bankName, XPeak, YPeak, i, qspan, peaksW, pixel_to_wi);

    MantidVec &X0 = outputW->dataX(i);
    TOFPeak = VectorHelper::getBinIndex(X0, TOFPeakd);

    double I = 0., sigI = 0.;
    // Find point of peak centre
    // Get references to the current spectrum
    const MantidVec &X = outputW->readX(i);
    const MantidVec &Y = outputW->readY(i);
    const MantidVec &E = outputW->readE(i);
    int numbins = static_cast<int>(Y.size());
    if (TOFmin > numbins)
      TOFmin = numbins;
    if (TOFmax > numbins)
      TOFmax = numbins;

    const double peakLoc = X[TOFPeak];
    int iTOF;
    for (iTOF = TOFmin; iTOF < TOFmax; iTOF++) {
      if (Y[iTOF] > 0.0 && Y[iTOF + 1] > 0.0)
        break;
    }
    TOFmin = iTOF;
    for (iTOF = TOFmax; iTOF > TOFmin; iTOF--) {
      if (Y[iTOF] > 0.0 && Y[iTOF - 1] > 0.0)
        break;
    }
    TOFmax = iTOF;
    if (TOFmax <= TOFmin)
      continue;
    const int n = TOFmax - TOFmin + 1;
    // double pktime = 0.0;

    // for (iTOF = TOFmin; iTOF < TOFmax; iTOF++) pktime+= X[iTOF];
    if (n >= 8 &&
        IC) // Number of fitting parameters large enough if Ikeda-Carpenter fit
    {
      for (iTOF = TOFmin; iTOF <= TOFmax; iTOF++) {
        if (((Y[iTOF] - Y[TOFPeak] / 2.) * (Y[iTOF + 1] - Y[TOFPeak] / 2.)) <
            0.)
          break;
      }
      double Gamma = fabs(X[TOFPeak] - X[iTOF]);
      double SigmaSquared = Gamma * Gamma;
      const double peakHeight = Y[TOFPeak] * Gamma; // Intensity*HWHM

      IAlgorithm_sptr fit_alg;
      try {
        fit_alg = createChildAlgorithm("Fit", -1, -1, false);
      } catch (Exception::NotFoundError &) {
        g_log.error("Can't locate Fit algorithm");
        throw;
      }
      // Initialize Ikeda-Carpender function variables
      double Alpha0 = 1.6;
      double Alpha1 = 1.5;
      double Beta0 = 31.9;
      double Kappa = 46.0;
      std::ostringstream fun_str;
      fun_str << "name=IkedaCarpenterPV,I=" << peakHeight
              << ",Alpha0=" << Alpha0 << ",Alpha1=" << Alpha1
              << ",Beta0=" << Beta0 << ",Kappa=" << Kappa
              << ",SigmaSquared=" << SigmaSquared << ",Gamma=" << Gamma
              << ",X0=" << peakLoc;
      fit_alg->setPropertyValue("Function", fun_str.str());
      if (Alpha0 != 1.6 || Alpha1 != 1.5 || Beta0 != 31.9 || Kappa != 46.0) {
        std::ostringstream tie_str;
        tie_str << "Alpha0=" << Alpha0 << ",Alpha1=" << Alpha1
                << ",Beta0=" << Beta0 << ",Kappa=" << Kappa;
        fit_alg->setProperty("Ties", tie_str.str());
      }
      fit_alg->setProperty("InputWorkspace", outputW);
      fit_alg->setProperty("WorkspaceIndex", i);
      fit_alg->setProperty("StartX", X[TOFmin]);
      fit_alg->setProperty("EndX", X[TOFmax]);
      fit_alg->setProperty("MaxIterations", 5000);
      fit_alg->setProperty("CreateOutput", true);
      fit_alg->setProperty("Output", "fit");
      fit_alg->executeAsChildAlg();
      MatrixWorkspace_sptr fitWS = fit_alg->getProperty("OutputWorkspace");

      /*double chisq = fit_alg->getProperty("OutputChi2overDoF");
      if(chisq > 0 && chisq < 400 && !haveFit && PeakIntensity < 30.0) // use
      fit of strong peaks for weak peaks
      {
        std::vector<double> params = fit_alg->getProperty("Parameters");
        Alpha0 = params[1];
        Alpha1 = params[2];
        Beta0 = params[3];
        Kappa = params[4];
        haveFit = true;
      }
      std::string funct = fit_alg->getPropertyValue("Function");
    */

      // Evaluate fit at points
      const Mantid::MantidVec &y = fitWS->readY(1);

      // Calculate intensity
      for (iTOF = 0; iTOF < n; iTOF++)
        if (!boost::math::isnan(y[iTOF]) && !boost::math::isinf(y[iTOF]))
          I += y[iTOF];
    } else
      for (iTOF = TOFmin; iTOF <= TOFmax; iTOF++)
        I += Y[iTOF];

    if (!IC) {
      sigI = peak.getSigmaIntensity();
    } else {
      // Calculate errors correctly for nonPoisson distributions
      for (iTOF = TOFmin; iTOF <= TOFmax; iTOF++)
        sigI += E[iTOF] * E[iTOF];
      sigI = sqrt(sigI);
    }

    peak.setIntensity(I);
    peak.setSigmaIntensity(sigI);

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }

  PARALLEL_CHECK_INTERUPT_REGION

  // Save the output
  setProperty("OutPeaksWorkspace", peaksW);
}

void PeakIntegration::retrieveProperties() {
  inputW = getProperty("InputWorkspace");
  if (inputW->readY(0).size() <= 1)
    throw std::runtime_error("Must Rebin data with more than 1 bin");
  // Check if detectors are RectangularDetectors
  Instrument_const_sptr inst = inputW->getInstrument();
  boost::shared_ptr<RectangularDetector> det;
  for (int i = 0; i < inst->nelements(); i++) {
    det = boost::dynamic_pointer_cast<RectangularDetector>((*inst)[i]);
    if (det)
      break;
  }
}

int PeakIntegration::fitneighbours(int ipeak, std::string det_name, int x0,
                                   int y0, int idet, double qspan,
                                   PeaksWorkspace_sptr &Peaks,
                                   const detid2index_map &pixel_to_wi) {
  UNUSED_ARG(ipeak);
  UNUSED_ARG(det_name);
  UNUSED_ARG(x0);
  UNUSED_ARG(y0);
  API::IPeak &peak = Peaks->getPeak(ipeak);
  // Number of slices
  int TOFmax = 0;

  IAlgorithm_sptr slice_alg = createChildAlgorithm("IntegratePeakTimeSlices");
  slice_alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
  std::ostringstream tab_str;
  tab_str << "LogTable" << ipeak;

  slice_alg->setPropertyValue("OutputWorkspace", tab_str.str());
  slice_alg->setProperty<PeaksWorkspace_sptr>("Peaks", Peaks);
  slice_alg->setProperty("PeakIndex", ipeak);
  slice_alg->setProperty("PeakQspan", qspan);

  int nPixels = std::max<int>(0, getProperty("NBadEdgePixels"));

  slice_alg->setProperty("NBadEdgePixels", nPixels);
  slice_alg->executeAsChildAlg();
  Mantid::API::MemoryManager::Instance().releaseFreeMemory();

  MantidVec &Xout = outputW->dataX(idet);
  MantidVec &Yout = outputW->dataY(idet);
  MantidVec &Eout = outputW->dataE(idet);
  TableWorkspace_sptr logtable = slice_alg->getProperty("OutputWorkspace");

  peak.setIntensity(slice_alg->getProperty("Intensity"));
  peak.setSigmaIntensity(slice_alg->getProperty("SigmaIntensity"));

  TOFmax = static_cast<int>(logtable->rowCount());
  for (int iTOF = 0; iTOF < TOFmax; iTOF++) {
    Xout[iTOF] = logtable->getRef<double>(std::string("Time"), iTOF);
    if (IC) // Ikeda-Carpenter fit
    {
      Yout[iTOF] = logtable->getRef<double>(std::string("TotIntensity"), iTOF);
      Eout[iTOF] =
          logtable->getRef<double>(std::string("TotIntensityError"), iTOF);
    } else {
      Yout[iTOF] = logtable->getRef<double>(std::string("ISAWIntensity"), iTOF);
      Eout[iTOF] =
          logtable->getRef<double>(std::string("ISAWIntensityError"), iTOF);
    }
  }

  outputW->getSpectrum(idet)->clearDetectorIDs();
  // Find the pixel ID at that XY position on the rectangular detector
  int pixelID = peak.getDetectorID(); // det->getAtXY(x0,y0)->getID();

  // Find the corresponding workspace index, if any
  auto wiEntry = pixel_to_wi.find(pixelID);
  if (wiEntry != pixel_to_wi.end()) {
    size_t wi = wiEntry->second;
    // Set detectorIDs
    outputW->getSpectrum(idet)
        ->addDetectorIDs(inputW->getSpectrum(wi)->getDetectorIDs());
  }

  return TOFmax - 1;
}

} // namespace Crystal
} // namespace Mantid
