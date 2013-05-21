/*WIKI*

This algorithm appends the spectra of two workspaces together.

The output workspace from this algorithm will be a copy of the first
input workspace, to which the data from the second input workspace
will be appended.

Workspace data members other than the data (e.g. instrument etc.) will be copied
from the first input workspace (but if they're not identical anyway,
then you probably shouldn't be using this algorithm!).

==== Restrictions on the input workspace ====

For [[EventWorkspace]]s, there are no restrictions on the input workspaces if ValidateInputs=false.

For [[Workspace2D]]s, the number of bins must be the same in both inputs.

If ValidateInputs is selected, then the input workspaces must also:
* Come from the same instrument
* Have common units
* Have common bin boundaries

==== Spectrum Numbers ====

If there is an overlap in the spectrum numbers of both inputs, then the output
workspace will have its spectrum numbers reset starting at 0 and increasing by
1 for each spectrum.

==== See Also ====

* [[ConjoinWorkspaces]] for joining parts of the same workspace.

*WIKI*/

#include "MantidAlgorithms/AppendSpectra.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/SingletonHolder.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace Algorithms
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(AppendSpectra)
  
  /** Constructor
   */
  AppendSpectra::AppendSpectra() : WorkspaceJoiners()
  {}

  /** Destructor
   */
  AppendSpectra::~AppendSpectra()
  {
  }

  /// Algorithm's name for identification. @see Algorithm::name
  const std::string AppendSpectra::name() const { return "AppendSpectra";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int AppendSpectra::version() const { return 1;};
  
  /** Initialize the algorithm's properties.
   */
  void AppendSpectra::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace1",
      "", Direction::Input, boost::make_shared<CommonBinsValidator>()),
      "The name of the first input workspace");
    declareProperty(new WorkspaceProperty<>("InputWorkspace2",
      "", Direction::Input, boost::make_shared<CommonBinsValidator>()),
      "The name of the second input workspace");

    declareProperty("ValidateInputs", true,
      "Perform a set of checks that the two input workspaces are compatible.");

    declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the output workspace");
  }

  /** Execute the algorithm.
   */
  void AppendSpectra::exec()
  {
    // Retrieve the input workspaces
    MatrixWorkspace_const_sptr ws1 = getProperty("InputWorkspace1");
    MatrixWorkspace_const_sptr ws2 = getProperty("InputWorkspace2");
    event_ws1 = boost::dynamic_pointer_cast<const EventWorkspace>(ws1);
    event_ws2 = boost::dynamic_pointer_cast<const EventWorkspace>(ws2);

    //Make sure that we are not mis-matching EventWorkspaces and other types of workspaces
    if (((event_ws1) && (!event_ws2)) || ((!event_ws1) && (event_ws2)))
    {
      const std::string message("Only one of the input workspaces are of type EventWorkspace; please use matching workspace types (both EventWorkspace's or both Workspace2D's).");
      g_log.error(message);
      throw std::invalid_argument(message);
    }

    bool ValidateInputs = this->getProperty("ValidateInputs");
    if (ValidateInputs)
    {
      // Check that the input workspaces meet the requirements for this algorithm
      this->validateInputs(ws1,ws2);
    }

    if (event_ws1 && event_ws2)
    {
      //Both are event workspaces. Use the special method
      MatrixWorkspace_sptr output = this->execEvent();
      // Set the output workspace
      setProperty("OutputWorkspace", output );
      return;
    }
    // So it is a workspace 2D.

    // The only restriction, even with ValidateInputs=false
    if (ws1->blocksize() != ws2->blocksize())
      throw std::runtime_error("Workspace2D's must have the same number of bins.");

    MatrixWorkspace_sptr output = execWS2D(ws1, ws2);

    // Set the output workspace
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(output) );
  }

  /** If there is an overlap in spectrum numbers between ws1 and ws2,
   * then the spectrum numbers are reset as a simple 1-1 correspondence
   * with the workspace index.
   *
   * @param ws1 The first workspace supplied to the algorithm.
   * @param ws2 The second workspace supplied to the algorithm.
   * @param output The workspace that is going to be returned by the algorithm.
   */
  void AppendSpectra::fixSpectrumNumbers(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2,
                                             API::MatrixWorkspace_sptr output)
  {
    specid_t ws1min;
    specid_t ws1max;
    getMinMax(ws1, ws1min, ws1max);

    specid_t ws2min;
    specid_t ws2max;
    getMinMax(ws2, ws2min, ws2max);

    // is everything possibly ok?
    if (ws2min > ws1max)
      return;

    // change the axis by adding the maximum existing spectrum number to the current value
    for (size_t i = 0; i < output->getNumberHistograms(); i++)
      output->getSpectrum(i)->setSpectrumNo( specid_t(i) );
  }

} // namespace Mantid
} // namespace Algorithms
