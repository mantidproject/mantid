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

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;


// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksMDHisto)

//----------------------------------------------------------------------------------------------
/**
 * Constructor
 */
IntegratePeaksMDHisto::IntegratePeaksMDHisto()
    : m_normWS(), m_inputWS(), m_hmin(0.0f), m_hmax(0.0f), m_kmin(0.0f),
      m_kmax(0.0f), m_lmin(0.0f), m_lmax(0.0f), m_hIntegrated(true),
      m_kIntegrated(true), m_lIntegrated(true), m_rubw(3, 3), m_kiMin(0.0),
      m_kiMax(EMPTY_DBL()), m_hIdx(-1), m_kIdx(-1), m_lIdx(-1), m_hX(), m_kX(),
      m_lX(), m_samplePos(), m_beamDir() {}


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

  API::MatrixWorkspace_const_sptr integrFlux = getProperty("FluxWorkspace");
  integrFlux->getXMinMax(m_kiMin, m_kiMax);
  API::MatrixWorkspace_const_sptr solidAngleWS =
      getProperty("SolidAngleWorkspace");

  m_inputWS = getProperty("InputWorkspace");
  if (inputEnergyMode() != "Elastic") {
    throw std::invalid_argument("Invalid energy transfer mode. Algorithm "
                                "currently only supports elastic data.");
  }
  // Min/max dimension values
  const auto hdim(m_inputWS->getDimension(0)), kdim(m_inputWS->getDimension(1)),
      ldim(m_inputWS->getDimension(2));
  m_hmin = hdim->getMinimum();
  m_kmin = kdim->getMinimum();
  m_lmin = ldim->getMinimum();
  m_hmax = hdim->getMaximum();
  m_kmax = kdim->getMaximum();
  m_lmax = ldim->getMaximum();

  const auto &exptInfoZero = *(m_inputWS->getExperimentInfo(0));
  auto source = exptInfoZero.getInstrument()->getSource();
  auto sample = exptInfoZero.getInstrument()->getSample();
  if (source == nullptr || sample == nullptr) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Instrument not sufficiently defined: failed to get source and/or "
        "sample");
  }
  m_samplePos = sample->getPos();
  m_beamDir = m_samplePos - source->getPos();
  m_beamDir.normalize();
}


  auto prog = make_unique<API::Progress>(this, 0.3, 1.0, ndets);
  PARALLEL_FOR1(integrPeaks)
  for (int64_t i = 0; i < npeaks; i++) {
    PARALLEL_START_INTERUPT_REGION

    const auto peak = peaks[i];
    int h = int(p.getH() + 0.5);
    int k = int(p.getK() + 0.5);
    int l = int(p.getL() + 0.5);
    MDNormSCD(InputWorkspace = 'MDdata';
        AlignedDim0='[H,0,0],'+str(H-0.5)+','+str(H+0.5)+',201';
        AlignedDim1='[0,K,0],'+str(K-0.5)+','+str(K+0.5)+',201';
        AlignedDim2='[0,0,L],'+str(L-0.5)+','+str(L+0.5)+',201';
        FluxWorkspace = flux;
        SolidAngleWorkspace = sa;
        OutputWorkspace = 'mdout'+str(run)+'_'+str(i);
        OutputNormalizationWorkspace = 'mdnorm'+str(run)+'_'+str(i))

      PARALLEL_CRITICAL(updateMD) {
        signal += m_normWS->getSignalAt(linIndex);
        m_normWS->setSignalAt(linIndex, signal);
      }
    }
    prog->report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}



} // namespace MDAlgorithms
} // namespace Mantid
