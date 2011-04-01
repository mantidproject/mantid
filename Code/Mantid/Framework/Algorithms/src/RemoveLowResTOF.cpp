#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAlgorithms/RemoveLowResTOF.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <limits>
#include <math.h>

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using namespace API;
using DataObjects::EventWorkspace;
using Geometry::IInstrument_const_sptr;
using Kernel::Exception::InstrumentDefinitionError;
using Kernel::Exception::NotFoundError;
using std::size_t;
using std::string;

DECLARE_ALGORITHM(RemoveLowResTOF)


/// Default constructor
RemoveLowResTOF::RemoveLowResTOF(): m_progress(NULL)
{
}

/// Destructor
RemoveLowResTOF::~RemoveLowResTOF()
{
  if(m_progress)
    delete m_progress;
  m_progress=NULL;
}

/// Algorithm's name for identification overriding a virtual method
const string RemoveLowResTOF::name() const
{
  return "RemoveLowResTOF";
}

/// Algorithm's version for identification overriding a virtual method
int RemoveLowResTOF::version() const
{
  return 1;
}

/// Algorithm's category for identification overriding a virtual method
const string RemoveLowResTOF::category() const
{
  return "Diffraction";
}

void RemoveLowResTOF::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("TOF"));
  wsValidator->add(new HistogramValidator<>);
  wsValidator->add(new RawCountValidator<>);
  wsValidator->add(new InstrumentValidator<>);
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "A workspace with x values in units of TOF and y values in counts" );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm" );

  BoundedValidator<double> *validator = new BoundedValidator<double>;
  validator->setLower(0.01);
  declareProperty("ReferenceDIFC", 0.0, validator,
    "The DIFC value for the reference" );

  validator = new BoundedValidator<double>;
  validator->setLower(0.01);
  declareProperty("K", 3.22, validator,
                  "Some arbitrary number whose default is 3.22 for reasons that I don't understand" );

  validator = new BoundedValidator<double>;
  validator->setLower(0.01);
  declareProperty("Tmin", Mantid::EMPTY_DBL(), validator,
                  "The minimum time-of-flight of the frame (in microseconds). If not set the data range will be used." );
}

void RemoveLowResTOF::exec()
{
  // Get the input workspace
  m_inputWS = this->getProperty("InputWorkspace");

  // without the primary flight path the algorithm cannot work
  try {
    m_instrument = m_inputWS->getInstrument();
    Geometry::IObjComponent_const_sptr sample = m_instrument->getSample();
    m_L1 = m_instrument->getSource()->getDistance(*sample);
  }
  catch (NotFoundError e)
  {
    throw InstrumentDefinitionError("Unable to calculate source-sample distance",
        m_inputWS->getTitle());
  }

  m_DIFCref = this->getProperty("ReferenceDIFC");
  m_K = this->getProperty("K");

  m_numberOfSpectra = m_inputWS->getNumberHistograms();

  // go off and do the event version if appropriate
  m_inputEvWS = boost::dynamic_pointer_cast<const EventWorkspace>(m_inputWS);
  if (m_inputEvWS != NULL)
  {
    this->execEvent();
    return;
  }

  // set up the progress bar
  m_progress = new Progress(this, 0.0, 1.0, m_numberOfSpectra);
  size_t  xSize = m_inputWS->dataX(0).size();

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != m_inputWS)
  {
    outputWS = WorkspaceFactory::Instance().create(m_inputWS, m_numberOfSpectra,
                                                   xSize, xSize-1);
    setProperty("OutputWorkspace", outputWS);
  }

  this->getTminData(false);

  for (size_t workspaceIndex = 0; workspaceIndex < m_numberOfSpectra; workspaceIndex++)
  {
    // copy the data from the input workspace
    outputWS->dataX(workspaceIndex) = m_inputWS->readX(workspaceIndex);
    outputWS->dataY(workspaceIndex) = m_inputWS->readY(workspaceIndex);
    outputWS->dataE(workspaceIndex) = m_inputWS->readE(workspaceIndex);

    // calculate where to zero out to
    double tofMin = this->calcTofMin(workspaceIndex);
    const MantidVec& X = m_inputWS->readX(0);
    MantidVec::const_iterator last = std::lower_bound(X.begin(),X.end(),tofMin);
    if (last==X.end()) --last;
    size_t endBin = last - X.begin();

    // flatten out the data
    for (size_t i = 0; i < endBin; i++)
    {
      outputWS->maskBin(workspaceIndex, i);
    }
    m_progress->report();
  }

  this->runMaskDetectors();
}

void RemoveLowResTOF::execEvent()
{
  // set up the output workspace
  MatrixWorkspace_sptr matrixOutW = this->getProperty("OutputWorkspace");
  DataObjects::EventWorkspace_sptr outW;
  if (matrixOutW == m_inputWS)
    outW = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutW);
  else
  {
    outW = boost::dynamic_pointer_cast<EventWorkspace>(
                API::WorkspaceFactory::Instance().create("EventWorkspace",m_numberOfSpectra,2,1) );
    //Copy required stuff from it
    API::WorkspaceFactory::Instance().initializeFromParent(m_inputWS, outW, false);
    outW->copyDataFrom( (*m_inputEvWS) );

    // cast to the matrixoutput workspace and save it
    matrixOutW = boost::dynamic_pointer_cast<MatrixWorkspace>(outW);
    this->setProperty("OutputWorkspace", matrixOutW);
  }

  // set up the progress bar
  m_progress = new Progress(this,0.0,1.0,m_numberOfSpectra*2);

  // algorithm assumes the data is sorted so it can jump out early
  outW->sortAll(Mantid::DataObjects::TOF_SORT, m_progress);

  this->getTminData(true);

  // do the actual work
  for (size_t workspaceIndex = 0; workspaceIndex < m_numberOfSpectra; workspaceIndex++)
  {
    double tmin = this->calcTofMin(workspaceIndex);
    if (tmin != tmin)
    {
      g_log.warning() << "tmin for workspaceIndex " << workspaceIndex << " is nan. Clearing out data.\n";
      outW->getEventList(workspaceIndex).clear();
    }
    else
      outW->getEventList(workspaceIndex).maskTof(0., tmin);
  }

  outW->clearMRU();
  this->runMaskDetectors();
}

double RemoveLowResTOF::calcTofMin(const size_t workspaceIndex)
{
  const Geometry::V3D& sourcePos = m_instrument->getSource()->getPos();
  const Geometry::V3D& samplePos = m_instrument->getSample()->getPos();
  const Geometry::V3D& beamline = samplePos - sourcePos;
  double beamline_norm = 2. * beamline.norm();

  const int spec = m_inputWS->getAxis(1)->spectraNo(workspaceIndex);
  std::vector<int> detNumbers = m_inputWS->spectraMap().getDetectors(spec);
  double dspmap = 0;
  for (std::vector<int>::const_iterator detNum = detNumbers.begin(); detNum != detNumbers.end(); detNum++)
  {
    dspmap += AlignDetectors::calcConversion(m_L1, beamline, beamline_norm, samplePos,
                                                 m_instrument->getDetector(*detNum), 0., false);
  }
  dspmap = dspmap / static_cast<double>(detNumbers.size());

  // this is related to the reference tof
  double sqrtdmin = sqrt(m_Tmin / m_DIFCref) + m_K * log10(dspmap * m_DIFCref);
  double tmin = sqrtdmin * sqrtdmin / dspmap;

  g_log.debug() << "tmin[" << workspaceIndex << "] " << tmin << "\n";
  return tmin;
}

void RemoveLowResTOF::getTminData(const bool isEvent)
{
  // get it from the properties
  double empty = Mantid::EMPTY_DBL();
  double temp = this->getProperty("Tmin");
  if (temp != empty)
  {
    m_Tmin = temp;
    return;
  }

  m_Tmin = std::numeric_limits<double>::max();
  if (isEvent)
  {
    for (size_t workspaceIndex = 0; workspaceIndex < m_numberOfSpectra; workspaceIndex++)
    {
      temp = m_inputEvWS->getEventList(workspaceIndex).getTofMin();
      if (temp < m_Tmin)
        m_Tmin = temp;
    }
  }
  else
  {
    for (size_t workspaceIndex = 0; workspaceIndex < m_numberOfSpectra; workspaceIndex++)
    {
      temp = m_inputWS->dataX(workspaceIndex).front();
      if (temp < m_Tmin)
          m_Tmin = temp;
    }
  }
  g_log.information() << "Tmin = " << m_Tmin << " microseconds\n";
  if (m_Tmin < 0.)
    throw std::runtime_error("Cannot have minimum time less than zero");
}


void RemoveLowResTOF::runMaskDetectors()
{
  IAlgorithm_sptr alg = createSubAlgorithm("MaskDetectors");
  alg->setProperty<MatrixWorkspace_sptr>("Workspace", this->getProperty("OutputWorkspace"));
  alg->setProperty<MatrixWorkspace_sptr>("MaskedWorkspace", this->getProperty("InputWorkspace"));
  if (!alg->execute())
    throw std::runtime_error("MaskDetectors sub-algorithm has not executed successfully");
}

} // namespace Algorithm
} // namespace Mantid
