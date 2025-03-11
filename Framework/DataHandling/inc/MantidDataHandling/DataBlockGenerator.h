// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DataBlock.h"
#include <optional>
#include <vector>

namespace Mantid {
namespace DataHandling {

class DataBlock;

/** DataBlockGenerator: The DataBlockGenerator class provides increasing
    int64_t numbers from a collection of intervals which are being input
    into the generator at construction.
*/
class MANTID_DATAHANDLING_DLL DataBlockGenerator {
public:
  DataBlockGenerator(std::vector<SpectrumPair> intervals);
  class DataBlock;
  DataBlockGenerator &operator++();
  DataBlockGenerator operator++(int);
  bool isDone();
  specnum_t getValue();
  void next();

public:
  std::vector<SpectrumPair> m_intervals;
  specnum_t m_currentSpectrum;

  std::optional<size_t> m_currentIntervalIndex;
};

} // namespace DataHandling
} // namespace Mantid
