// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IAlgorithm.h" // for AlgorithmID typedef

namespace Mantid {
namespace PythonInterface {
/**
 * Provides a concrete type to wrap & return AlgorithmIDs that are actually
 * just typedefs for void*
 */
struct AlgorithmIDProxy {
  /// Construct with existing pointer
  explicit AlgorithmIDProxy(API::AlgorithmID p) : id(p) {}

  bool operator==(const AlgorithmIDProxy &rhs) { return (id == rhs.id); }
  /// held ID value
  API::AlgorithmID id;
};
} // namespace PythonInterface
} // namespace Mantid
