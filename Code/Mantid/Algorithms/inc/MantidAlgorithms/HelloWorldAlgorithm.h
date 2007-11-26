#ifndef MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_
#define MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_

#include <iostream>

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/StatusCode.h"

namespace Mantid
{
namespace Algorithms
{
	
class HelloWorldAlgorithm : public API::Algorithm
{
public:
  HelloWorldAlgorithm() : API::Algorithm() {}
  virtual ~HelloWorldAlgorithm() {}
  Mantid::Kernel::StatusCode init() { return Mantid::Kernel::StatusCode::SUCCESS; }
  Mantid::Kernel::StatusCode exec() { std::cout << "\nHello, World!\n"; return Mantid::Kernel::StatusCode::SUCCESS; }
  Mantid::Kernel::StatusCode final() { return Mantid::Kernel::StatusCode::SUCCESS; }
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_*/

