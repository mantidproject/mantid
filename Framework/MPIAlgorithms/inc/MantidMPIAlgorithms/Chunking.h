// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cstddef>

namespace Mantid::MPIAlgorithms::Chunking {

constexpr std::size_t DEFAULT_TARGET_CHUNK_BYTES = 100 * 1024 * 1024;

std::size_t chooseChunkSize(int userChunkSize, std::size_t numSpec, std::size_t numBins);

} // namespace Mantid::MPIAlgorithms::Chunking
