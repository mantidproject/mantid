// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MyNameCpp.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace Algorithms {
using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MyNameCpp)

void MyNameCpp::init() { declareProperty("Meaning", 42, "Provides a meaning to the Universe"); }

void MyNameCpp::exec() {
  int value = getProperty("Meaning");
  g_log.error() << "The meaning of the universe is " << value << "\n";
}

} // namespace Algorithms
} // namespace Mantid
