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
    declareProperty("WhatKindOfWorld","Mantid");
}

/**
*/
void HelloWorldAlgorithm::exec()
{
	// g_log is a reference to the logger. It is used to print out information,
  // warning, and error messages
  std::string boevs = getProperty("WhatKindOfWorld");
  g_log.information() << "\nHello " + boevs + " World!\n";
}

}
}

