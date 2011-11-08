/*WIKI*

Perform the Not (negation) boolean operation on a [[MDHistoWorkspace]].
The not operation is performed element-by-element.
Any 0.0 signal is changed to 1.0 (meaning true).
Any non-zero signal is changed to 0.0 (meaning false).

== Usage ==

See [[MDHistoWorkspace#Boolean_Operations|this page]] for examples on using boolean operations.

*WIKI*/

#include "MantidMDAlgorithms/NotMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(NotMD)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  NotMD::NotMD()
  {  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  NotMD::~NotMD()
  {  }
  
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string NotMD::name() const { return "NotMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int NotMD::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void NotMD::initDocs()
  {
    this->setWikiSummary("Performs a boolean negation on a [[MDHistoWorkspace]].");
    this->setOptionalMessage("Performs a boolean negation on a MDHistoWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /// Check the inputs and throw if the algorithm cannot be run
  void NotMD::checkInputs()
  {
    if (!m_in_histo)
      throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm on a MDEventWorkspace
  void NotMD::execEvent(Mantid::API::IMDEventWorkspace_sptr /*out*/)
  {
    throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /// NotMD::Run the algorithm with a MDHistoWorkspace
  void NotMD::execHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out)
  {
    out->operatorNot();
  }



} // namespace Mantid
} // namespace MDAlgorithms
