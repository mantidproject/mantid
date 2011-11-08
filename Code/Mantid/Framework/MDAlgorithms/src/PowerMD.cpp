/*WIKI*

This executes the power function on a MDHistoWorkspace.

The signal <math>a</math> becomes <math> f = f = a^b </math>

The error <math>da</math> becomes <math> df^2 = f^2 * b^2 * (da^2 / a^2) </math>

This algorithm cannot be run on a [[MDEventWorkspace]]. Its equivalent on a [[MatrixWorkspace]] is called [[Power]].

*WIKI*/

#include "MantidMDAlgorithms/PowerMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PowerMD)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PowerMD::PowerMD()
  {  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PowerMD::~PowerMD()
  {  }

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string PowerMD::name() const { return "PowerMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int PowerMD::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PowerMD::initDocs()
  {
    this->setWikiSummary("Raise a [[MDHistoWorkspace]] to a power.");
    this->setOptionalMessage("Raise a MDHistoWorkspace to a power");
  }

  //----------------------------------------------------------------------------------------------
  /// Optional method to be subclassed to add properties
  void PowerMD::initExtraProperties()
  {
    declareProperty("Exponent", 2.0,
        "Power to which to raise the values. Default 2.0.");
  }

  //----------------------------------------------------------------------------------------------
  /// Check the inputs and throw if the algorithm cannot be run
  void PowerMD::checkInputs()
  {
    if (!m_in_histo)
      throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm on a MDEventWorkspace
  void PowerMD::execEvent(Mantid::API::IMDEventWorkspace_sptr /*out*/)
  {
    throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /// PowerMD::Run the algorithm with a MDHistoWorkspace
  void PowerMD::execHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out)
  {
    double exponent = getProperty("Exponent");
    out->power(exponent);
  }



} // namespace Mantid
} // namespace MDAlgorithms
