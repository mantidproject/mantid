#include "HelloWorldAlgorithm.h"

namespace Mantid
{
namespace Algorithms
{

DECLARE_ALGORITHM(HelloWorldAlgorithm)


/**
*/
void HelloWorldAlgorithm::init()
{
}

/**
*/
void HelloWorldAlgorithm::exec()
{
		// g_log is a reference to the logger. It is used to print out information,
		// warning, and error messages
    g_log.information() << "\nHello, World!\n";
}

}
}

