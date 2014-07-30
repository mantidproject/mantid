#include "MantidAlgorithms/ClearInstrumentParameters.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ClearInstrumentParameters)

  using namespace Kernel;
  using namespace API;
  using namespace Geometry;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ClearInstrumentParameters::ClearInstrumentParameters()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ClearInstrumentParameters::~ClearInstrumentParameters()
  {
  }

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ClearInstrumentParameters::name() const { return "ClearInstrumentParameters";};

  /// Algorithm's version for identification. @see Algorithm::version
  int ClearInstrumentParameters::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ClearInstrumentParameters::category() const { return "DataHandling\\Instrument";}

  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ClearInstrumentParameters::init()
  {
    declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut,boost::make_shared<InstrumentValidator>()),
    "Workspace whose instrument parameters are to be cleared.");

    declareProperty("LocationParameters", true, "Clear the location parameters used to calibrate the instrument.", Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ClearInstrumentParameters::exec()
  {
  }

} // namespace Algorithms
} // namespace Mantid
