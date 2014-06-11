/*WIKI*
This algorithm looks for an internally-stored monitor workspace on the input workspace and
sets it as the output workspace if found. The input workspace will no longer hold a reference
to the monitor workspace after this algorithm has been run on it.
If no monitor workspace is present the algorithm will fail.
*WIKI*/

#include "MantidDataHandling/ExtractMonitorWorkspace.h"

namespace Mantid
{
namespace DataHandling
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ExtractMonitorWorkspace)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  ExtractMonitorWorkspace::ExtractMonitorWorkspace()
  {}

  ExtractMonitorWorkspace::~ExtractMonitorWorkspace()
  {}

  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ExtractMonitorWorkspace::name() const { return "ExtractMonitorWorkspace";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int ExtractMonitorWorkspace::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ExtractMonitorWorkspace::category() const { return "DataHandling";}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string  ExtractMonitorWorkspace::summary() const
  {
    return "Retrieves a workspace of monitor data held within the input workspace, if present.";
  }

  /** Initialize the algorithm's properties.
   */
  void ExtractMonitorWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("MonitorWorkspace","",Direction::Output), "An output workspace.");
  }

  /** Execute the algorithm.
   */
  void ExtractMonitorWorkspace::exec()
  {
    MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
    auto monitorWS = inputWS->monitorWorkspace();

    if ( ! monitorWS )
    {
      throw std::invalid_argument("The input workspace does not hold a monitor workspace");
    }

    setProperty("MonitorWorkspace", monitorWS);
    // Now clear off the pointer on the input workspace
    inputWS->setMonitorWorkspace(MatrixWorkspace_sptr());
  }

} // namespace DataHandling
} // namespace Mantid
