#include "MantidAlgorithms/Dummy.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  //DECLARE_ALGORITHM(Dummy)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  Dummy::Dummy()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Dummy::~Dummy()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void Dummy::initDocs()
  {
    this->setOptionalMessage("Dummy algorithm for testing");

  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void Dummy::init()
  {
    declareProperty("IntNumber", 123);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void Dummy::exec()
  {
  }



} // namespace Mantid
} // namespace Algorithms

