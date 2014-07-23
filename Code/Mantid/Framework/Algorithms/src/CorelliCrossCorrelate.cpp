#include "MantidAlgorithms/CorelliCrossCorrelate.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{

  using namespace Kernel;
  using namespace API;
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
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CorelliCrossCorrelate::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Algorithms
} // namespace Mantid
