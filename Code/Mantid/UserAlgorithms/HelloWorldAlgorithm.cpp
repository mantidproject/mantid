#include "HelloWorldAlgorithm.h"
#include "MantidAPI/AlgorithmFactory.h"

namespace Mantid
{
namespace Algorithms
{

DECLARE_ALGORITHM(HelloWorldAlgorithm);

// Get a reference to the logger. It is used to print out information,
// warning, and error messages
Mantid::Kernel::Logger& HelloWorldAlgorithm::g_log = Mantid::Kernel::Logger::get("HelloWorldAlgorithm");

/**  
*/
void HelloWorldAlgorithm::init()
{
}

/**  
*/
void HelloWorldAlgorithm::exec() 
{ 
    g_log.information() << "\nHello, World!\n"; 
}

}
}

