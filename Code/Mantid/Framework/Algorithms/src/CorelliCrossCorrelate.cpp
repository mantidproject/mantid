#include "MantidAlgorithms/CorelliCrossCorrelate.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid
{
namespace Algorithms
{

  using namespace Kernel;
  using namespace API;
  using namespace Geometry;
  using namespace DataObjects;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CorelliCrossCorrelate)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CorelliCrossCorrelate::CorelliCrossCorrelate()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CorelliCrossCorrelate::~CorelliCrossCorrelate()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CorelliCrossCorrelate::init()
  {
    auto wsValidator = boost::make_shared<CompositeValidator>();
    wsValidator->add<WorkspaceUnitValidator>("TOF");
    wsValidator->add<InstrumentValidator>();
    
    declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace","",Direction::Input,wsValidator), "An input workspace.");
    declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");

    declareProperty("TimingOffset", static_cast<int64_t>(EMPTY_INT()), boost::make_shared<MandatoryValidator<int64_t> >(), "Correlation chopper TDC timing offset in nanoseconds.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CorelliCrossCorrelate::exec()
  {
    inputWS = getProperty("InputWorkspace");
    outputWS = getProperty("OutputWorkspace");
    // TODO Check is getSortType == SORTED
    if (outputWS != inputWS)
    {
      //Make a brand new EventWorkspace
      outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
          API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
      //Copy geometry over.
      API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
      //You need to copy over the data as well.
      outputWS->copyDataFrom( (*inputWS) );
    }

    const int64_t offset = getProperty("TimingOffset");

    //This algorithm will only work for CORELLI, check for CORELLI.
    std::string InstrumentName = inputWS->getInstrument()->getName();
    if ( InstrumentName != "CORELLI") {
        throw std::runtime_error("This Algorithm will only work for Corelli.");
      }

    //Must include the correlation-chopper in the IDF.
    IComponent_const_sptr chopper = inputWS->getInstrument()->getComponentByName("correlation-chopper");
    if (!chopper) {
	throw Exception::InstrumentDefinitionError("Correlation chopper not found. This will not work!");
      }

    //Read in chopper sequence from IDF.
    std::vector<std::string> chopperSequence = chopper->getStringParameter("sequence");
    std::vector<double> sequence;

    float weightTransparent;
    float weightAbsorbing;
    if (chopperSequence.empty()) {
      throw Exception::InstrumentDefinitionError("Found the correlation chopper but no chopper sequence?");
    }
    else
      {
	g_log.debug("Found chopper sequence: " + chopperSequence[0]);
	std::vector<std::string> chopperSequenceSplit;	
	//Chopper sequence, alternating between open and closed. If index%2=0 than absorbing.
	boost::split(chopperSequenceSplit,chopperSequence[0],boost::is_space());

	//calculate duty cycle ~0.5
	sequence.resize(chopperSequenceSplit.size());
	double totalOpen = 0;
	sequence[0] = std::stod(chopperSequenceSplit[0]);

	for (unsigned int i=1; i<chopperSequenceSplit.size(); i++) {
	  double seqAngle = std::stod(chopperSequenceSplit[i]);
	  sequence[i]=sequence[i-1]+seqAngle;
	  if (i % 2 == 1)
	    totalOpen+=seqAngle;
	}
	double dutyCycle = totalOpen/sequence.back();
	weightTransparent = static_cast<float>(1.0/dutyCycle);
	weightAbsorbing = static_cast<float>(-1.0/(1.0-dutyCycle));
	g_log.debug() << "dutyCycle = " << dutyCycle << " weightTransparent = " << weightTransparent << " weightAbsorbing = " << weightAbsorbing << "\n";
      }

    //Check to make sure that there are TDC timings for the correlation chopper and read them in.
    std::vector<DateAndTime> tdc;
    if ( inputWS->run().hasProperty("chopper4_TDC") ){
      g_log.debug("Found chopper4_TDC!");
      ITimeSeriesProperty* tdcLog = dynamic_cast<ITimeSeriesProperty*>( inputWS->run().getLogData("chopper4_TDC") );
      tdc = tdcLog->timesAsVector();

      for (unsigned long i=0; i<tdc.size(); ++i)
	tdc[i]+=offset;
    }
    else {
      throw std::runtime_error("Missing correlations chopper TDC timings. Did you LoadLogs?");
    }

    double frequency = dynamic_cast<TimeSeriesProperty<double>*>(inputWS->run().getLogData("BL9:Chop:Skf4:MotorSpeed"))->getStatistics().mean;
    double period = 1e9/frequency;
    g_log.debug() << "Frequency = " << frequency << "Hz Period = " << period << "ns\n";

    IComponent_const_sptr source = inputWS->getInstrument()->getSource();
    IComponent_const_sptr sample = inputWS->getInstrument()->getSample();
    if ( source == NULL || sample == NULL )
      {
	throw Exception::InstrumentDefinitionError("Instrument not sufficiently defined: failed to get source and/or sample");
      }
    const double distanceChopperToSource = source->getDistance(*chopper);
    const double distanceChopperToSample = sample->getDistance(*chopper);

    int64_t numHistograms = static_cast<int64_t>(inputWS->getNumberHistograms());
    g_log.notice("Start cross-correlation\n");
    // TODO Add Progess
    // TODO Parallel this for loop
    for (int64_t i=0; i < numHistograms; ++i)
      {
	EventList *evlist=outputWS->getEventListPtr(i);
	IDetector_const_sptr detector = inputWS->getDetector(i);
	double tofScale = distanceChopperToSource/(distanceChopperToSource+distanceChopperToSample+detector->getDistance(*sample));

	switch (evlist->getEventType())
	  {
	  case TOF:
	    //Switch to weights if needed.
	    evlist->switchTo(WEIGHTED);
	    /* no break */
	    // Fall through
	  case WEIGHTED:
	    break;
	  case WEIGHTED_NOTIME:
	    throw std::runtime_error("This event list has no pulse time information.");
	    break;
	  }

	std::vector<WeightedEvent>& events = evlist->getWeightedEvents();

	//Skip if empty.
	if (events.empty()) continue;

	//Check for duplicate pulses problem in Corelli.
	DateAndTime emptyTime;
	if (events.back().pulseTime() == emptyTime)
	  throw std::runtime_error("Missing pulse times on events. This will not work.");

	int tdc_i = 0;
	std::vector<WeightedEvent>::iterator it;
	for (it = events.begin(); it != events.end(); ++it)
	  {
	    DateAndTime tofTime = it->pulseTime() + static_cast<int64_t>(it->tof()*1000.*tofScale);
	    while (tofTime>tdc[tdc_i])
	      {
		//TODO check for bounds
		tdc_i+=1;
	      }

	    double angle = 360.*static_cast<double>(tofTime.totalNanoseconds()-tdc[tdc_i-1].totalNanoseconds())/period;

	    std::vector<double>::iterator location;
	    location = std::lower_bound(sequence.begin(),sequence.end(),angle);

	    if ( (location-sequence.begin())%2 == 0)
		it->m_weight *= weightAbsorbing;
	    else
		it->m_weight *= weightTransparent;
	  }
      }
    setProperty("OutputWorkspace", outputWS);
  }



} // namespace Algorithms
} // namespace Mantid
