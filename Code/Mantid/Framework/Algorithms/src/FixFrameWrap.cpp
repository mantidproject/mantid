#include "MantidAlgorithms/FixFrameWrap.h"
#include "MantidDataObjects/EventList.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"
#include <stdexcept>

using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Progress;
using Mantid::DataObjects::EventList;
using Mantid::DataObjects::EventWorkspace;
using Mantid::Geometry::IInstrument_const_sptr;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::Exception::InstrumentDefinitionError;
using Mantid::Kernel::Exception::NotFoundError;
using std::string;

namespace Mantid
{
namespace Algorithms
{

DECLARE_ALGORITHM(FixFrameWrap)

const string FixFrameWrap::name() const
{
  return "FixFrameWrap";
}

int FixFrameWrap::version() const
{
  return 1;
}

const string FixFrameWrap::category() const
{
  return "Diffraction";
}

void FixFrameWrap::initDocs()
{
  string msg("Algorithm to move events that are in the wrong frame due to DAS into the correct one.");
  this->setWikiSummary(msg);
  this->setOptionalMessage(msg);
}

FixFrameWrap::FixFrameWrap()
{
}

FixFrameWrap::~FixFrameWrap()
{
}

void FixFrameWrap::init()
{
  declareProperty(new API::WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input),
                  "A Workspace with calibrated time-of-flight to correct");
  declareProperty(new API::WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
                  "The corrected workspace");
  BoundedValidator<double> *validator = new BoundedValidator<double>;
  validator->setLower(0.01);
  declareProperty("LRef", 0., validator,
                  "The length of the reference flight path (in metres)" );
  validator = new BoundedValidator<double>;
  validator->setLower(0.0);
  declareProperty("Tmin", Mantid::EMPTY_DBL(), validator,
                  "The minimum time-of-flight of the frame (in microseconds). If not set the data range will be used.");
  validator = new BoundedValidator<double>;
  validator->setLower(0.0);
  declareProperty("Tmax", Mantid::EMPTY_DBL(), validator,
                  "The minimum time-of-flight of the frame (in microseconds). If not set the data range will be used.");
}

void FixFrameWrap::exec()
{
  // generic pointer for the input workspace
  matrixInputW = this->getProperty("InputWorkspace");

  // without the primary flight path the algorithm cannot work
  try {
    IInstrument_const_sptr instrument = matrixInputW->getInstrument();
    Geometry::IObjComponent_const_sptr sample = instrument->getSample();
    L1 = instrument->getSource()->getDistance(*sample);
  }
  catch (NotFoundError e)
  {
    throw InstrumentDefinitionError("Unable to calculate source-sample distance",
                                               matrixInputW->getTitle());
  }

  // process events
  eventInputW = boost::dynamic_pointer_cast<EventWorkspace>(matrixInputW);
  if (eventInputW != NULL)
  {
    this->execEvent();
    this->cleanup();
    return;
  }

  // process a histogram
  throw std::runtime_error("FixFrameWrap does not support histograms");
}

void FixFrameWrap::execEvent()
{
  //Create a new outputworkspace with not much in it
  std::size_t nHist = eventInputW->getNumberHistograms();
  MatrixWorkspace_sptr matrixOutW = this->getProperty("OutputWorkspace");
  DataObjects::EventWorkspace_sptr outW;
  if (matrixOutW == matrixInputW)
    outW = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutW);
  else
  {
    outW = boost::dynamic_pointer_cast<EventWorkspace>(
                API::WorkspaceFactory::Instance().create("EventWorkspace",nHist,2,1) );
    //Copy required stuff from it
    API::WorkspaceFactory::Instance().initializeFromParent(matrixInputW, outW, false);
    outW->copyDataFrom( (*eventInputW) );

    // cast to the matrixoutput workspace and save it
    matrixOutW = boost::dynamic_pointer_cast<MatrixWorkspace>(outW);
    this->setProperty("OutputWorkspace", matrixOutW);
  }
  double Lref = this->getProperty("LRef");

  // set up the progress bar
  Progress * prog;
  prog = new Progress(this,0.0,1.0,nHist*2);

  // algorithm assumes the data is sorted so it can jump out early
  outW->sortAll(Mantid::DataObjects::TOF_SORT, prog);

  // find the minimum time and frame width
  double tmin = Mantid::EMPTY_DBL();
  double tmax = Mantid::EMPTY_DBL();
  for (size_t i = 0; i < nHist; i++)
  {
    EventList& events = outW->getEventList(i);
    if (events.getNumberEvents() > 0)
    {
      tmin = *(events.dataX().begin());
      tmax = *(events.dataX().rbegin());
      break;
    }
  }
  {
    double temp;
    double empty = Mantid::EMPTY_DBL();
    temp = this->getProperty("Tmin");
    if (temp != empty)
      tmin = temp;
    temp = this->getProperty("Tmax");
    if (temp != empty)
      tmax = temp;
  }
  double frameWidth = tmax - tmin;
  g_log.information() << "Tmin = " << tmin << " Tmax = " << tmax
                       << " Frame width = " << frameWidth << "\n";

  for (size_t i = 0; i < nHist; i++)
  {
//    int spec = eventInputW->getAxis(1)->spectraNo(i); // TODO will need later
    std::size_t numEvents = outW->getEventList(i).getNumberEvents();
    if (numEvents > 0)
    {
//      outW->getEventList(i).addTof(frameWidth);
      MantidVec times(numEvents);
      outW->getEventList(i).getTofs(times);
      bool isMonitor;
      double Ld = this->calculateFlightpath(i, L1, isMonitor);
      double filterVal = tmin * Ld / Lref;
      std::cout << (Ld/Lref) << " filterVal = " << filterVal << std::endl; // REMOVE
      for (size_t j = 0; j < numEvents; j++)
      {
        if (times[j] < filterVal)
          times[j] += frameWidth;
        else
          break; // stop filtering
      }
      outW->getEventList(i).setTofs(times);
    }
    prog->report();
  }
  outW->clearMRU();
  // cleanup progress bar
  delete prog;
}

/** Calculates the total flightpath for the given detector.
 *  This is L1+L2 normally, but is the source-detector distance for a monitor.
 *  @param spectrum ::  The workspace index
 *  @param L1 ::        The primary flightpath
 *  @param isMonitor :: Output: true is this detector is a monitor
 *  @return The flightpath (Ld) for the detector linked to spectrum
 *  @throw Kernel::Exception::InstrumentDefinitionError if the detector position can't be obtained
 */
double FixFrameWrap::calculateFlightpath(const int& spectrum, const double& L1, bool& isMonitor) const
{
  double Ld = -1.0;
  try
  {
    // Get the detector object for this histogram
    Geometry::IDetector_const_sptr det = matrixInputW->getDetector(spectrum);
    // Get the sample-detector distance for this detector (or source-detector if a monitor)
    // This is the total flightpath
    isMonitor = det->isMonitor();
    // Get the L2 distance if this detector is not a monitor
    if ( !isMonitor )
    {
      double L2 = det->getDistance(*(matrixInputW->getInstrument()->getSample()));
      Ld = L1 + L2;
    }
    // If it is a monitor, then the flightpath is the distance to the source
    else
    {
      Ld = det->getDistance(*(matrixInputW->getInstrument()->getSource()));
    }
  }
  catch (NotFoundError)
  {
    // If the detector information is missing, return a negative number
  }

  return Ld;
}

void FixFrameWrap::cleanup()
{
}

} // namespace Algorithms
} // namespace Mantid
