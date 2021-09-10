// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "HelloWorldAlgorithm.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(HelloWorldAlgorithm)

void HelloWorldAlgorithm::init() { declareProperty("WhatKindOfWorld", "Mantid"); }

void HelloWorldAlgorithm::exec() {
  // g_log is a reference to the logger. It is used to print out information,
  // warning, and error messages
  std::string boevs = getProperty("WhatKindOfWorld");
  g_log.information() << "\nHello " + boevs + " World!\n";
}
} // namespace Algorithms
} // namespace Mantid
