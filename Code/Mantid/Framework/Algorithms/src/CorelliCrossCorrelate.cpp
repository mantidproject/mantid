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
    //declareProperty(new PropertyWithValue<int>("TimingOffset",Direction::Input), "Correlation chopper TDC timing offset in nanoseconds.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CorelliCrossCorrelate::exec()
  {
    // TODO Auto-generated execute stub
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
    g_log.debug() << "TDC timing offest " << offset << "\n";

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
    if (chopperSequence.empty()) {
      throw Exception::InstrumentDefinitionError("Found the correlation chopper but no chopper sequence?");
    }
    else
      {
	g_log.debug("Found chopper sequence: " + chopperSequence[0]);
	std::vector<std::string> chopperSequenceSplit;
	boost::split(chopperSequenceSplit,chopperSequence[0],boost::is_space());
	//Chopper sequence, alternating between open and closed. If index%2=0 than absorbing.
	g_log.debug() << "Found chopper sequence length: " << chopperSequenceSplit.size() << " seq[0] = " << chopperSequenceSplit[0] << " seq["<<chopperSequenceSplit.size()-1 <<"] = " << chopperSequenceSplit[chopperSequenceSplit.size()-1] <<"\n";
	//calculate duty cycle ~0.5
	sequence.resize(chopperSequenceSplit.size());
	double totalOpen = 0;
	sequence[0] = std::stod(chopperSequenceSplit[0]);
	g_log.debug() << "Number = 0 CumSum = " << sequence[0] << "\n";
	for (unsigned int i=1; i<chopperSequenceSplit.size(); i++) {
	  double seqAngle = std::stod(chopperSequenceSplit[i]);
	  sequence[i]=sequence[i-1]+seqAngle;
	  if (i % 2 == 1)
	    totalOpen+=seqAngle;
	  g_log.debug() << "Number = " << i << " CumSum = " << sequence[i] << "\n";
	}
	double dutyCycle = totalOpen/sequence.back();
	g_log.debug() << "duty cycle = " << dutyCycle << "\n";
      }

    //Check to make sure that there are TDC timings for the correlation chopper and read them in.
    std::vector<DateAndTime> tdc;
    if ( inputWS->run().hasProperty("chopper4_TDC") ){
      g_log.notice("Found chopper4_TDC!");
      ITimeSeriesProperty* tdcLog = dynamic_cast<ITimeSeriesProperty*>( inputWS->run().getLogData("chopper4_TDC") );
      tdc = tdcLog->timesAsVector();
      g_log.debug() << "TDC length = " << tdc.size() << "\n";
      for (int i=0; i<10; i++)
	g_log.debug() << "Before TDC["<<i<<"] = " << " - " << tdc[i] <<"\n";
      g_log.debug() << "Before TDC["<<tdc.size()-1<<"] = " << " - " << tdc[tdc.size()-1] <<"\n";

      //std::transform(tdc.begin(),tdc.end(),tdc.begin(),std::bind2nd(std::minus<DateAndTime>(),offset));
      /*typename std::vector<DataAndTime>::iterator tdcIt;
      for (tdcIt = tdc.begin(); tdcIt != tdc.end(); ++tdcIt)
      tdcIt-=offset;*/
      for (unsigned long i=0; i<tdc.size(); ++i)
	tdc[i]-=offset;

      for (int i=0; i<10; i++)
	g_log.debug() << "After  TDC["<<i<<"] = " << " - " << tdc[i] <<"\n";
      g_log.debug() << "After  TDC["<<tdc.size()-1<<"] = " << " - " << tdc[tdc.size()-1] <<"\n";
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
    g_log.debug() << "Distance ChopperToSource = " << distanceChopperToSource << " ChopperToSample = " << distanceChopperToSample << "\n";

    int64_t numHistograms = static_cast<int64_t>(inputWS->getNumberHistograms());
    g_log.debug() << "Num histograms = " << numHistograms << "\n";
    for (int64_t i=0; i < numHistograms; ++i)
      {
	EventList *evlist=outputWS->getEventListPtr(i);
	IDetector_const_sptr detector = inputWS->getDetector(i);
	double tofScale = distanceChopperToSource/(distanceChopperToSource+distanceChopperToSample+detector->getDistance(*sample));
	g_log.debug() << "No. " << i << " Dsd = " << detector->getDistance(*sample) << " Order = "<< evlist->getSortType() << " Number of events " << evlist->getNumberEvents() <<"\n";

	switch (evlist->getEventType())
	  {
	  case TOF:
	    //Switch to weights if needed.
	    evlist->switchTo(WEIGHTED);
	    /* no break */
	    // Fall through
	    g_log.debug() << "Case TOF\n";
	  case WEIGHTED:
	    g_log.debug() << "Case WEIGHTED\n";
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

	size_t numevents = events.size();
	for (size_t i = 0; i < numevents; i++)
	  {
	    const WeightedEvent& e = events[i];
	    g_log.debug() << i << " PulseTime = " << e.pulseTime() << " Tof = " << e.tof() << " Weight = " << e.weight() <<"\n";
	  }

	int tdc_i = 0;
	typename std::vector<WeightedEvent>::iterator it;
	for (it = events.begin(); it != events.end(); ++it)
	  {
	    //Do Magic!!!
	    DateAndTime tofTime = it->pulseTime() + static_cast<int64_t>(it->tof()*1000.*tofScale);
	    while (tofTime>tdc[tdc_i])
	      {
		//check for bounds
		tdc_i+=1;
	      }
	    double angle = 360.*static_cast<double>(tofTime.totalNanoseconds()-tdc[tdc_i-1].totalNanoseconds())/period;
	    std::vector<double>::iterator where;
	    where = std::lower_bound(sequence.begin(),sequence.end(),angle);
	    if ( (where-sequence.begin()+1)%2 == 0)
	      {
		it->m_weight *= static_cast<float>(-0.5);
	      }
	    else
	      {
		it->m_weight *= static_cast<float>(0.5);
	      }
	    g_log.debug() << i << " PulseTime = " << it->pulseTime() << " Tof = " << it->tof() << " Weight = " << it->weight() <<"\n";
	    g_log.debug() << i << " TofTime = " << tofTime <<"\n";
	    g_log.debug() << i << " TDC ID  = " << tdc_i << " TDC = " << tdc[tdc_i-1] << " Diff = " << tofTime.totalNanoseconds()-tdc[tdc_i-1].totalNanoseconds() << "\n";
	    g_log.debug() << " Angle = " << angle << " i = " << (where-sequence.begin()) << "\n";
	  }
      }
    setProperty("OutputWorkspace", outputWS);
  }



} // namespace Algorithms
} // namespace Mantid
