//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnwrapSNS.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include <limits>

namespace Mantid
{
namespace Algorithms
{

DECLARE_ALGORITHM(UnwrapSNS)

/// Sets documentation strings for this algorithm
void UnwrapSNS::initDocs()
{
  this->setWikiSummary("Takes an input [[workspace]] that contains 'raw' data, unwraps the data according to the reference flightpath provided and converts the units to wavelength. The output workspace will have common bins in the maximum theoretical wavelength range. ");
  this->setOptionalMessage("Takes an input workspace that contains 'raw' data, unwraps the data according to the reference flightpath provided and converts the units to wavelength. The output workspace will have common bins in the maximum theoretical wavelength range.");
}


using namespace Kernel;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using Geometry::IInstrument_const_sptr;
using Kernel::Exception::InstrumentDefinitionError;
using Kernel::Exception::NotFoundError;
using std::size_t;

/// Default constructor
UnwrapSNS::UnwrapSNS():m_progress(NULL)
{}

/// Destructor
UnwrapSNS::~UnwrapSNS()
{
  if(m_progress)
    delete m_progress;
  m_progress=NULL;
}

/// Algorithm's name for identification overriding a virtual method
const std::string UnwrapSNS::name() const
{
  return "UnwrapSNS";
}

/// Algorithm's version for identification overriding a virtual method
int UnwrapSNS::version() const
{
  return 1;
}

/// Algorithm's category for identification overriding a virtual method
const std::string UnwrapSNS::category() const
{
  return "Units";
}

/// Initialisation method
void UnwrapSNS::init()
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
  declareProperty("LRef", 0.0, validator,
    "The length of the reference flight path (in metres)" );
  validator = new BoundedValidator<double>;
  validator->setLower(0.01);
  declareProperty("Tmin", Mantid::EMPTY_DBL(), validator,
                  "The minimum time-of-flight of the frame (in microseconds). If not set the data range will be used.");
  validator = new BoundedValidator<double>;
  validator->setLower(0.01);
  declareProperty("Tmax", Mantid::EMPTY_DBL(), validator,
                  "The minimum time-of-flight of the frame (in microseconds). If not set the data range will be used.");
//  declareProperty("ForceHist", false); // TODO remove

  // Calculate and set the constant factor for the conversion to wavelength
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  m_conversionConstant = (PhysicalConstants::h * toAngstroms) / (PhysicalConstants::NeutronMass * TOFisinMicroseconds);
}

/** Executes the algorithm
 *  @throw std::runtime_error if the workspace is invalid or a child algorithm fails
 *  @throw Kernel::Exception::InstrumentDefinitionError if detector, source or sample positions cannot be calculated
 *
 */
void UnwrapSNS::exec()
{
  // Get the input workspace
  m_inputWS = getProperty("InputWorkspace");

  // without the primary flight path the algorithm cannot work
  try {
    IInstrument_const_sptr instrument = m_inputWS->getInstrument();
    Geometry::IObjComponent_const_sptr sample = instrument->getSample();
    m_L1 = instrument->getSource()->getDistance(*sample);
  }
  catch (NotFoundError e)
  {
    throw InstrumentDefinitionError("Unable to calculate source-sample distance",
        m_inputWS->getTitle());
  }


  // Get the "reference" flightpath (currently passed in as a property)
  m_LRef = getProperty("LRef");

  m_XSize = static_cast<int>(m_inputWS->dataX(0).size());
  m_numberOfSpectra = static_cast<int>(m_inputWS->getNumberHistograms());
  g_log.debug() << "Number of spectra in input workspace: " << m_numberOfSpectra << "\n";


  // go off and do the event version if appropriate
  m_inputEvWS = boost::dynamic_pointer_cast<const EventWorkspace>(m_inputWS);
  if ((m_inputEvWS != NULL)) // && ! this->getProperty("ForceHist")) // TODO remove ForceHist option
  {
    this->execEvent();
    return;
  }

  this->getTofRangeData(false);

  // set up the progress bar
  m_progress = new Progress(this, 0.0, 1.0, m_numberOfSpectra);

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != m_inputWS)
  {
    outputWS = WorkspaceFactory::Instance().create(m_inputWS, m_numberOfSpectra,
                                                   m_XSize,m_XSize-1);
    setProperty("OutputWorkspace", outputWS);
  }

  PARALLEL_FOR2(m_inputWS,outputWS)
  for (int workspaceIndex = 0; workspaceIndex < m_numberOfSpectra; workspaceIndex++)
  {
    PARALLEL_START_INTERUPT_REGION
    // get the total flight path
    bool isMonitor;
    double Ld = this->calculateFlightpath(workspaceIndex, isMonitor);
    if (Ld < 0.)
    {
      // If the detector flightpath is missing, zero the data
       g_log.debug() << "Detector information for workspace index " << workspaceIndex
                     << " is not available.\n";
       outputWS->dataX(workspaceIndex) = m_inputWS->dataX(workspaceIndex);
       outputWS->dataY(workspaceIndex).assign(m_XSize-1, 0.0);
       outputWS->dataE(workspaceIndex).assign(m_XSize-1, 0.0);
    }
    else {
      // fix the x-axis
      size_t pivot = this->unwrapX(m_inputWS->readX(workspaceIndex),
                                   outputWS->dataX(workspaceIndex), Ld);
      pivot++; // one-off difference between x and y

      // fix the counts using the pivot point
      const MantidVec& yIn = m_inputWS->readY(workspaceIndex);
      MantidVec& yOut = outputWS->dataY(workspaceIndex);
      yOut.clear();
      yOut.insert(yOut.begin(), yIn.begin() + pivot, yIn.end());
      yOut.insert(yOut.end(), yIn.begin(), yIn.begin() + pivot);

      // fix the uncertainties using the pivot point
      const MantidVec& eIn = m_inputWS->readE(workspaceIndex);
      MantidVec& eOut = outputWS->dataE(workspaceIndex);
      eOut.clear();
      eOut.insert(eOut.begin(), eIn.begin() + pivot, eIn.end());
      eOut.insert(eOut.end(),   eIn.begin(),         eIn.begin() + pivot);
    }
    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  m_inputWS.reset();
  this->runMaskDetectors();
}

void UnwrapSNS::execEvent()
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

  this->getTofRangeData(true);

  // do the actual work
//  PARALLEL_FOR2(m_inputWS, outW)
  for (int workspaceIndex = 0; workspaceIndex < m_numberOfSpectra; workspaceIndex++)
  {
//    PARALLEL_START_INTERUPT_REGION
    std::size_t numEvents = outW->getEventList(workspaceIndex).getNumberEvents();
    bool isMonitor;
    double Ld = this->calculateFlightpath(workspaceIndex, isMonitor);
    MantidVec time_bins;
    this->unwrapX(m_inputWS->dataX(workspaceIndex), time_bins, Ld);
    outW->setX(workspaceIndex, time_bins);
    if (numEvents > 0)
    {
      MantidVec times(numEvents);
      outW->getEventList(workspaceIndex).getTofs(times);
      double filterVal = m_Tmin * Ld / m_LRef;
      for (size_t j = 0; j < numEvents; j++)
      {
        if (times[j] < filterVal)
          times[j] += m_frameWidth;
        else
          break; // stop filtering
      }
      outW->getEventList(workspaceIndex).setTofs(times);
    }
    m_progress->report();
//    PARALLEL_END_INTERUPT_REGION
  }
//  PARALLEL_CHECK_INTERUPT_REGION

  outW->clearMRU();
  this->runMaskDetectors();
}

/** Calculates the total flightpath for the given detector.
 *  This is L1+L2 normally, but is the source-detector distance for a monitor.
 *  @param spectrum ::  The workspace index
 *  @param L1 ::        The primary flightpath
 *  @param isMonitor :: Output: true is this detector is a monitor
 *  @return The flightpath (Ld) for the detector linked to spectrum
 *  @throw Kernel::Exception::InstrumentDefinitionError if the detector position can't be obtained
 */
double UnwrapSNS::calculateFlightpath(const int& spectrum, bool& isMonitor) const
{
  double Ld = -1.0;
  try
  {
    // Get the detector object for this histogram
    Geometry::IDetector_const_sptr det = m_inputWS->getDetector(spectrum);
    // Get the sample-detector distance for this detector (or source-detector if a monitor)
    // This is the total flightpath
    isMonitor = det->isMonitor();
    // Get the L2 distance if this detector is not a monitor
    if ( !isMonitor )
    {
      double L2 = det->getDistance(*(m_inputWS->getInstrument()->getSample()));
      Ld = m_L1 + L2;
    }
    // If it is a monitor, then the flightpath is the distance to the source
    else
    {
      Ld = det->getDistance(*(m_inputWS->getInstrument()->getSource()));
    }
  }
  catch (Exception::NotFoundError)
  {
    // If the detector information is missing, return a negative number
  }

  return Ld;
}

int UnwrapSNS::unwrapX(const MantidVec& datain, MantidVec& dataout, const double& Ld)
{
  MantidVec tempX_L; // lower half - to be frame wrapped
  tempX_L.reserve(m_XSize);
  tempX_L.clear();
  MantidVec tempX_U; // upper half - to not be frame wrapped
  tempX_U.reserve(m_XSize);
  tempX_U.clear();

  double filterVal = m_Tmin * Ld / m_LRef;
  dataout.clear();
  int specialBin = 0;
  for (int bin = 0; bin < m_XSize; ++bin)
  {
    // This is the time-of-flight value under consideration in the current iteration of the loop
    const double tof = datain[bin];
    if (tof < filterVal) {
      tempX_L.push_back(tof + m_frameWidth);
      // Record the bins that fall in this range for copying over the data & errors
      if (specialBin < bin)
        specialBin = bin;
    }
    else {
      tempX_U.push_back(tof);

    }
  } // loop over X values

  // now put it back into the vector supplied
  dataout.clear();
  dataout.insert(dataout.begin(), tempX_U.begin(), tempX_U.end());
  dataout.insert(dataout.end(), tempX_L.begin(),tempX_L.end());
  assert(datain.size() == dataout.size());

  return specialBin;
}

void UnwrapSNS::getTofRangeData(const bool isEvent)
{

  // Get the min & max frame values
  double empty = Mantid::EMPTY_DBL();
  double temp;

  // check for Tmin property
  temp = this->getProperty("Tmin");
  if (temp == empty)
  {
    m_Tmin = std::numeric_limits<double>::max();
    if (isEvent)
    {
      m_Tmin = m_inputEvWS->getTofMin();
    }
    else
    {
      for (int workspaceIndex = 0; workspaceIndex < m_numberOfSpectra; workspaceIndex++)
      {
        temp = m_inputWS->dataX(workspaceIndex).front();
        if (temp < m_Tmin)
          m_Tmin = temp;
      }
    }
  }
  else
  {
    m_Tmin = temp;
  }

  // check for Tmax property
  temp = this->getProperty("Tmax");
  if (temp == empty)
  {
    m_Tmax = std::numeric_limits<double>::min();
    if (isEvent)
    {
      m_Tmax = m_inputEvWS->getTofMax();
    }
    else
    {
      for (int workspaceIndex = 0; workspaceIndex < m_numberOfSpectra; workspaceIndex++)
      {
        temp = m_inputWS->dataX(workspaceIndex).back();
        if (temp > m_Tmax)
          m_Tmax = temp;
      }
    }
  }
  else
  {
    m_Tmax = temp;
  }

  m_frameWidth = m_Tmax - m_Tmin;

  g_log.information() << "Frame range in microseconds is: " << m_Tmin << " - " << m_Tmax << "\n";
  if (m_Tmin < 0.)
    throw std::runtime_error("Cannot have Tmin less than zero");
  if (m_Tmin > m_Tmax)
    throw std::runtime_error("Have case of Tmin > Tmax");

  g_log.information() << "Wavelength cuttoff is : " << (m_conversionConstant * m_Tmin / m_LRef)
                      << "Angstrom, Frame width is: " << m_frameWidth << "microseconds\n";
}

void UnwrapSNS::runMaskDetectors()
{
  IAlgorithm_sptr alg = createSubAlgorithm("MaskDetectors");
  alg->setProperty<MatrixWorkspace_sptr>("Workspace", this->getProperty("OutputWorkspace"));
  alg->setProperty<MatrixWorkspace_sptr>("MaskedWorkspace", this->getProperty("InputWorkspace"));
  if (!alg->execute())
    throw std::runtime_error("MaskDetectors sub-algorithm has not executed successfully");
}

} // namespace Algorithm
} // namespace Mantid
