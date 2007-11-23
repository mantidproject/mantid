#ifndef MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_
#define MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_

#include <iostream>

#include "Algorithm.h"
#include "MantidKernel/StatusCode.h"

namespace Mantid
{
namespace Algorithms
{
	
class HelloWorldAlgorithm : public Mantid::Kernel::Algorithm
{
public:
  HelloWorldAlgorithm() : Mantid::Kernel::Algorithm() {}
  virtual ~HelloWorldAlgorithm() {}
  Mantid::Kernel::StatusCode init() { return Mantid::Kernel::StatusCode::SUCCESS; }
  Mantid::Kernel::StatusCode exec() { std::cout << "Hello, World!\n"; return Mantid::Kernel::StatusCode::SUCCESS; }
  Mantid::Kernel::StatusCode final() { return Mantid::Kernel::StatusCode::SUCCESS; }
};

DECLARE_ALGORITHM(HelloWorldAlgorithm)

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_*/

