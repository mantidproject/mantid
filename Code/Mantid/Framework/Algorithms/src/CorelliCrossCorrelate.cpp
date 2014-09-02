#include "MantidAlgorithms/CorelliCrossCorrelate.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/IComponent.h"

namespace Mantid
{
namespace Algorithms
{

  using namespace Kernel;
  using namespace API;
  using namespace Geometry;
  using DataObjects::EventWorkspace;

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
    //declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CorelliCrossCorrelate::exec()
  {
    // TODO Auto-generated execute stub
    inputWS = getProperty("InputWorkspace");

    //This algorithm will only work for CORELLI, check for CORELLI.
    std::string InstrumentName = inputWS->getInstrument()->getName();
    if ( InstrumentName != "CORELLI") {
        throw std::runtime_error("This Algorithm will only work for Corelli.");
      }

    //Must include the correlation-chopper in the IDF.
    IComponent_const_sptr Chopper = inputWS->getInstrument()->getComponentByName("correlation-chopper");
    if (!Chopper) {
	throw std::runtime_error("Correlation chopper not found. This will not work!");
      }
    //Read in chopper sequence from IDF.
    std::vector<std::string> ChopperSequence = Chopper->getStringParameter("sequence");
    if (ChopperSequence.empty()) {
      throw std::runtime_error("Found the correlation chopper but no chopper sequence?");
    }
    else
      {
	g_log.debug("Found chopper sequence - "+ChopperSequence[0]);
      }

    //Check to make sure that there are TDC timings for the correlation chopper and read them in.
    if ( inputWS->run().hasProperty("chopper4_TDC") ){
      g_log.notice("Has chopper4_TDC!");
      inputWS->run().getProperty("chopper4_TDC");
    }
    else {
      throw std::runtime_error("Missing correlations chopper TDC timings. Did you LoadLogs?");
    }

    //Get reported correlation chopper motor speed.
    //chopper_freq = w.getRun().getProperty("BL9:Chop:Skf4:MotorSpeed").timeAverageValue()
    //double frequency = dynamic_cast<TimeSeriesProperty<double>*>(inputWS->run().getLogData("BL9:Chop:Skf4:MotorSpeed"))->getStatistics().mean;

  }



} // namespace Algorithms
} // namespace Mantid
