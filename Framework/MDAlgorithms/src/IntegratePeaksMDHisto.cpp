#include "MantidMDAlgorithms/IntegratePeaksMDHisto.h"

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidGeometry/Crystal/IPeak.h"

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;
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

  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "A name for the output data MDHistoWorkspace.");
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputNormalizationWorkspace", "", Direction::Output),
                  "A name for the output normalization MDHistoWorkspace.");
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
  Mantid::DataObjects::PeaksWorkspace_sptr inPeakWS =
      getProperty("PeaksWorkspace");

  /// Output peaks workspace, create if needed
  Mantid::DataObjects::PeaksWorkspace_sptr peakWS =
      getProperty("OutputWorkspace");
  if (peakWS != inPeakWS)
    peakWS = inPeakWS->clone();

  API::MatrixWorkspace_const_sptr flux = getProperty("FluxWorkspace");
  API::MatrixWorkspace_const_sptr sa =
      getProperty("SolidAngleWorkspace");

  API::IMDEventWorkspace_const_sptr m_inputWS = getProperty("InputWorkspace");

  double box = 0.5;
  int gridPts = 201;
  
  int npeaks = peakWS->getNumberPeaks();

  auto prog = make_unique<API::Progress>(this, 0.3, 1.0, npeaks);
  PARALLEL_FOR1(peakWS)
  for (int i = 0; i < npeaks; i++) {
    PARALLEL_START_INTERUPT_REGION

    IPeak &p = peakWS->getPeak(i);
    // round to integer
    int h = int(p.getH() + 0.5);
    int k = int(p.getK() + 0.5);
    int l = int(p.getL() + 0.5);
    MDHistoWorkspace_sptr normBox = normalize(h, k, l, box, gridPts, flux, sa, m_inputWS);

      //PARALLEL_CRITICAL(updateMD) {
        //signal += m_normWS->getSignalAt(linIndex);
      //}
    prog->report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}
MDHistoWorkspace_sptr IntegratePeaksMDHisto::normalize(
    int h, int k, int l, double box, int gridPts,
     API::MatrixWorkspace_const_sptr flux,  API::MatrixWorkspace_const_sptr sa,  API::IMDEventWorkspace_const_sptr ws) {

  API::IAlgorithm_sptr normAlg = createChildAlgorithm("MDNormSCD");
  normAlg->setProperty("InputWorkspace", ws);
  normAlg->setProperty("AlignedDim0",
        "[H,0,0],"+boost::lexical_cast<std::string>(h-box)+","+boost::lexical_cast<std::string>(h+box)+","+boost::lexical_cast<std::string>(gridPts));
  normAlg->setProperty("AlignedDim1",
        "[0,K,0],"+boost::lexical_cast<std::string>(k-box)+","+boost::lexical_cast<std::string>(k+box)+","+boost::lexical_cast<std::string>(gridPts));
  normAlg->setProperty("AlignedDim2",
        "[0,0,L],"+boost::lexical_cast<std::string>(l-box)+","+boost::lexical_cast<std::string>(l+box)+","+boost::lexical_cast<std::string>(gridPts));
  normAlg->setProperty("FluxWorkspace", flux);
  normAlg->setProperty("SolidAngleWorkspace", sa);
  normAlg->setProperty("OutputWorkspace", "mdout");
  normAlg->setProperty("OutputNormalizationWorkspace", "mdnorm");
  normAlg->executeAsChildAlg();

  MDHistoWorkspace_sptr mdout = normAlg->getProperty("OutputWorkspace");
  MDHistoWorkspace_sptr mdnorm = normAlg->getProperty("OutputtNormalizationWorkspace");

  API::IAlgorithm_sptr alg = createChildAlgorithm("DivideMD");
  alg->setProperty("LHSWorkspace", mdout);
  alg->setProperty("RHSWorkspace", mdnorm);
  alg->setPropertyValue("OutputWorkspace", "out");
  alg->execute();
  MDHistoWorkspace_sptr out = alg->getProperty("OutputWorkspace");
  return out;
}





} // namespace MDAlgorithms
} // namespace Mantid
