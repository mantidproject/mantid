#include "MantidAlgorithms/Dummy.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MultipleFileProperty.h"

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
    declareProperty("ShowStuff", false);
    for (int i=1; i<10; i++)
    {
      std::string pname = "Int" + Strings::toString(i);
      declareProperty(pname, 1, new BoundedValidator<int>(100, 200));
      setPropertySettings(pname, new VisibleWhenProperty(this, "ShowStuff", IS_EQUAL_TO, "1") );
    }


  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void Dummy::exec()
  {
  }



} // namespace Mantid
} // namespace Algorithms

