/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidMDAlgorithms/MultiplyMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MultiplyMD)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MultiplyMD::MultiplyMD()
  {  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MultiplyMD::~MultiplyMD()
  {  }
  
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MultiplyMD::name() const { return "MultiplyMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int MultiplyMD::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MultiplyMD::initDocs()
  {
    this->setWikiSummary("Multiply a [[MDHistoWorkspace]] by another one or a scalar.");
    this->setOptionalMessage("Multiply a MDHistoWorkspace by another one or a scalar.");
  }


  //----------------------------------------------------------------------------------------------
  /// Is the operation commutative?
  bool MultiplyMD::commutative() const
  { return true; }

  //----------------------------------------------------------------------------------------------
  /// Check the inputs and throw if the algorithm cannot be run
  void MultiplyMD::checkInputs()
  {
    if (m_lhs_event || m_rhs_event)
      throw std::runtime_error("Cannot multiply a MDEventWorkspace at this time.");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with an MDEventWorkspace as output
  void MultiplyMD::execEvent()
  {
    throw std::runtime_error("Cannot multiply a MDEventWorkspace at this time.");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void MultiplyMD::execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::MDEvents::MDHistoWorkspace_const_sptr operand)
  {
    out->multiply(*operand);
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  void MultiplyMD::execHistoScalar(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar)
  {
    out->multiply(scalar->dataY(0)[0], scalar->dataE(0)[0]);
  }



} // namespace Mantid
} // namespace MDAlgorithms
