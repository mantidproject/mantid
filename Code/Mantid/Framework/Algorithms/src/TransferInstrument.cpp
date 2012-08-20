/*WIKI* 

Transfer an instrument from a giving workspace to a receiving workspace for the same instrument.

The instrument in of the receiving workspace is replaced by a copy of the instrument in the giving workspace
and so gains any manipulations such as calibration done to the instrument in the giving workspace.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/TransferInstrument.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <iostream>
#include "MantidAPI/MemoryManager.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

namespace Mantid
{
namespace Algorithms
{

using std::size_t;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(TransferInstrument)

/// Sets documentation strings for this algorithm
void TransferInstrument::initDocs()
{
  this->setWikiSummary("Transfers an instrument from on workspace to another workspace with same base instrument.");
  this->setOptionalMessage("Transfers an instrument from on workspace to another workspace with same base instrument.");
}

/// Get a reference to the logger. It is used to print out information, warning and error messages
Mantid::Kernel::Logger& TransferInstrument::g_log = Mantid::Kernel::Logger::get("TransferInstrument");



using namespace Kernel;
using namespace API;
using namespace Geometry;
///using namespace DataObjects;

/// Default constructor
TransferInstrument::TransferInstrument() : 
  Algorithm()
{}

/// Destructor
TransferInstrument::~TransferInstrument() {}

void TransferInstrument::init()
{
  declareProperty(new WorkspaceProperty<>("GivingWorkspace","",Direction::Input),
    "Name of the workspace giving the instrument" );
  declareProperty(new WorkspaceProperty<>("ReceivingWorkspace","",Direction::InOut),
    "Name of the workspace receiving the instrument" );
}

/** Executes the algorithm
 *  @throw std::out_of_range If a property is set to an invalid value for the input workspace
 */
void TransferInstrument::exec()
{

  // Get the giving workspace
  m_givingWorkspace = getProperty("GivingWorkspace");

  // Get the receiving workspace
  m_receivingWorkspace = getProperty("ReceivingWorkspace"); 

  // Retrieve and validate the input properties
  this->checkProperties();

  // Get parameters
  const Geometry::ParameterMap& givParams = m_givingWorkspace->constInstrumentParameters() ;

  // Copy parameters
  m_receivingWorkspace->replaceInstrumentParameters( givParams );

}


/** Retrieves the properties and checks that they have valid values.
 *  @throw std::invalid_argument If either workspace has no instrument or the instruments have different base instruments.
 */
void TransferInstrument::checkProperties()
{

  // Check that both workspaces have an instrument
  Instrument_const_sptr inst = m_givingWorkspace->getInstrument();
  if( !inst )
  {
      throw std::invalid_argument("Giving workspace has no instrument");
  }
  Instrument_const_sptr inst2 = m_receivingWorkspace->getInstrument();
  if( !inst2 )
  {
      throw std::invalid_argument("Receiving workspace has no instrument");
  }

  Instrument_const_sptr baseInstGiv = inst->baseInstrument();
  Instrument_const_sptr baseInstRec = inst2->baseInstrument();

  // Check that both workspaces have the same instrument name
  if( baseInstRec != baseInstGiv )
  {
      throw std::invalid_argument("The base instrument in the receiving workspace if not the same as the base instrument in the giving workspace.");
  }

}

} // namespace Algorithms
} // namespace Mantid
