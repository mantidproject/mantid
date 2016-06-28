#include "MantidMDAlgorithms/IntegratePeaksMDHisto.h"

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/CompositeValidator.h"

#include "MantidGeometry/Crystal/IPeak.h"

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
//using Mantid::API::WorkspaceProperty;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;


// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksMDHisto)

//----------------------------------------------------------------------------------------------
/**
 * Constructor
 */
IntegratePeaksMDHisto::IntegratePeaksMDHisto()
    {}


//----------------------------------------------------------------------------------------------
/**
  * Initialize the algorithm's properties.
  */
void IntegratePeaksMDHisto::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input Sample MDEventWorkspace in HKL.");

  auto fluxValidator = boost::make_shared<CompositeValidator>();
  fluxValidator->add<WorkspaceUnitValidator>("Momentum");
  fluxValidator->add<InstrumentValidator>();
  fluxValidator->add<CommonBinsValidator>();
  auto solidAngleValidator = fluxValidator->clone();

  declareProperty(make_unique<WorkspaceProperty<>>(
                      "FluxWorkspace", "", Direction::Input, fluxValidator),
                  "An input workspace containing momentum dependent flux.");
  declareProperty(make_unique<WorkspaceProperty<>>("SolidAngleWorkspace", "",
                                                   Direction::Input,
                                                   solidAngleValidator),
                  "An input workspace containing momentum integrated vanadium "
                  "(a measure of the solid angle).");

  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::Input),
                  "A PeaksWorkspace containing the peaks to integrate.");

  declareProperty(
      make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "",
                                                     Direction::Output),
      "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
      "with the peaks' integrated intensities.");
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void IntegratePeaksMDHisto::exec() {
  /// Peak workspace to integrate
  PeaksWorkspace_sptr inPeakWS =
      getProperty("PeaksWorkspace");

  /// Output peaks workspace, create if needed
  PeaksWorkspace_sptr peakWS =
      getProperty("OutputWorkspace");
  if (peakWS != inPeakWS)
    peakWS = inPeakWS->clone();

  MatrixWorkspace_sptr flux = getProperty("FluxWorkspace");
  MatrixWorkspace_sptr sa =
      getProperty("SolidAngleWorkspace");

  IMDEventWorkspace_sptr m_inputWS = getProperty("InputWorkspace");

  double box = 0.5;
  int gridPts = 201;
  
  int npeaks = peakWS->getNumberPeaks();

  auto prog = make_unique<Progress>(this, 0.3, 1.0, npeaks);
  PARALLEL_FOR1(peakWS)
  for (int i = 0; i < npeaks; i++) {
    PARALLEL_START_INTERUPT_REGION

    IPeak &p = peakWS->getPeak(i);
    // round to integer
    int h = static_cast<int>(p.getH() + 0.5);
    int k = static_cast<int>(p.getK() + 0.5);
    int l = static_cast<int>(p.getL() + 0.5);
    MDHistoWorkspace_sptr normBox = normalize(
        h, k, l, box, gridPts, flux, sa, m_inputWS);
    double intensity = 0.0;
    double errorSquared = 0.0;
    integratePeak(normBox, intensity, errorSquared, gridPts);
    p.setIntensity(intensity);
    p.setSigmaIntensity(sqrt(errorSquared));
    prog->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

MDHistoWorkspace_sptr IntegratePeaksMDHisto::normalize(
    int h, int k, int l, double box, int gridPts,
     MatrixWorkspace_sptr flux,  MatrixWorkspace_sptr sa,
     IMDEventWorkspace_sptr ws) {
  IAlgorithm_sptr normAlg = createChildAlgorithm("MDNormSCD");
  normAlg->setProperty("InputWorkspace", ws);
  normAlg->setProperty("AlignedDim0",
        "[H,0,0],"+boost::lexical_cast<std::string>(h-box)+","+
        boost::lexical_cast<std::string>(h+box)+","+
        boost::lexical_cast<std::string>(gridPts));
  normAlg->setProperty("AlignedDim1",
        "[0,K,0],"+boost::lexical_cast<std::string>(k-box)+","+
        boost::lexical_cast<std::string>(k+box)+","+
        boost::lexical_cast<std::string>(gridPts));
  normAlg->setProperty("AlignedDim2",
        "[0,0,L],"+boost::lexical_cast<std::string>(l-box)+","+
        boost::lexical_cast<std::string>(l+box)+","+
        boost::lexical_cast<std::string>(gridPts));
  normAlg->setProperty("FluxWorkspace", flux);
  normAlg->setProperty("SolidAngleWorkspace", sa);
  normAlg->setProperty("OutputWorkspace", "mdout");
  normAlg->setProperty("OutputNormalizationWorkspace", "mdnorm");
  normAlg->executeAsChildAlg();

  Workspace_sptr mdout = normAlg->getProperty("OutputWorkspace");
  Workspace_sptr mdnorm = normAlg->getProperty("OutputNormalizationWorkspace");

  IAlgorithm_sptr alg = createChildAlgorithm("DivideMD");
  alg->setProperty("LHSWorkspace", mdout);
  alg->setProperty("RHSWorkspace", mdnorm);
  alg->setPropertyValue("OutputWorkspace", "out");
  alg->execute();
  Workspace_sptr out = alg->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<MDHistoWorkspace>(out);
}

void  IntegratePeaksMDHisto::integratePeak(MDHistoWorkspace_sptr out, double& intensity, double& errorSquared, int gridPts) {
    double *F = out->getSignalArray();
    int noPoints = 10;
    double Fmax = 0;
    double Fmin = 1e300;
    for (int i = 0; i < gridPts*gridPts*gridPts; i++) {
      if (!boost::math::isnan(F[i]) && !boost::math::isinf(F[i]) && F[i] != 0.0) {
        if (F[i] < Fmin) Fmin = F[i];
        if (F[i] > Fmax) Fmax = F[i];
      }
    }

    double *SqError = out->getErrorSquaredArray();

    double minIntensity = Fmin + 0.01 * (Fmax - Fmin);
    int measuredPoints  = 0;
    int peakPoints  = 0;
    double peakSum = 0.0;
    double measuredSum = 0.0;
    double errSqSum = 0.0;
    double measuredErrSqSum = 0.0;
    //double Peak[201*201*201] = {0};
    //double PeakErr2[201*201*201] = {0};
    for (int Hindex  = 0; Hindex < gridPts; Hindex++) {
        for (int Kindex  = 0; Kindex < gridPts; Kindex++) {
            for (int Lindex  = 0; Lindex < gridPts; Lindex++) {
              int iHKL = Hindex + gridPts * (Kindex +gridPts * Lindex);
                if (!boost::math::isnan(F[iHKL]) && !boost::math::isinf(F[iHKL])) {
                    measuredPoints = measuredPoints + 1;
                    measuredSum = measuredSum + F[iHKL];
                    measuredErrSqSum = measuredErrSqSum + F[iHKL];
                    if (F[iHKL] > minIntensity) {
                        int neighborPoints  = 0;
                        for (int Hj  = -2; Hj < 3; Hj++) {
                            for (int Kj  = -2; Kj <  3; Kj++) {
                              for (int Lj  = -2; Lj <  3; Lj++) {
                                    int jHKL = Hindex + Hj + gridPts * (Kindex + Kj +gridPts * (Lindex + Lj));
                                    if (Lindex+Lj >=   0 && Lindex+Lj < gridPts  && Kindex+Kj >=   0 && Kindex+Kj < gridPts && Hindex+Hj >=   0 && Hindex+Hj < gridPts && F[jHKL] > minIntensity) {
                                        neighborPoints = neighborPoints + 1;
                                    }
                                }
                            }
                        }
                        if (neighborPoints >=  noPoints) {
                            //Peak[iHKL] = F[iHKL];
                            //PeakErr2[iHKL] = SqError[iHKL];
                            peakPoints = peakPoints + 1;
                            peakSum = peakSum + F[iHKL];
                            errSqSum = errSqSum + SqError[iHKL];
                        }
                    }
                }
                else{
                   double minR = sqrt( std::pow(float(Hindex)/float(gridPts) - 0.5, 2) + std::pow(float(Kindex)/float(gridPts) - 0.5, 2) + std::pow(float(Lindex)/float(gridPts) - 0.5, 2));
                    if (minR < 0.1) {
                        intensity = 0.0;
                        errorSquared = 0.0;
                        return;
                    }
                }
            }
        }
    }
    double ratio = float(peakPoints)/float(measuredPoints - peakPoints);
    //std::cout peakSum,  errSqSum,  ratio,  measuredSum,  measuredErrSqSum
    intensity = peakSum - ratio * (measuredSum - peakSum);
    errorSquared = errSqSum + ratio * (measuredErrSqSum - errSqSum);
    return;
}






} // namespace MDAlgorithms
} // namespace Mantid
