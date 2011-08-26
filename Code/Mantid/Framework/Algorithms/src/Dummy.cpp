#include "MantidAlgorithms/Dummy.h"
#include "MantidKernel/System.h"
#include "MantidKernel/EnabledWhenProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(Dummy)
  


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
    declareProperty("MyIntProp", 123);
    // Make another property that is only enabled when the first one is NOT the default
    declareProperty("OtherProp", 123,
        new EnabledWhenProperty<int>(this, "MyIntProp", IS_NOT_DEFAULT) );

    declareProperty("AdvancedStuff", false);
    declareProperty("AdvancedInt", 123,
        new EnabledWhenProperty<int>(this, "AdvancedStuff", IS_NOT_DEFAULT) );

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void Dummy::exec()
  {
  }



} // namespace Mantid
} // namespace Algorithms

