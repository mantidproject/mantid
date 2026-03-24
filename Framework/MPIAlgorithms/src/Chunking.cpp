// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMPIAlgorithms/Chunking.h"
#include <algorithm>

namespace Mantid::MPIAlgorithms::Chunking {

/**
 * Choose a chunk size based on user input, if auto(0) is selected we will
 * calculate optimal chunk size based on workspace dimensions and target memory usage
 * @param userChunkSize Input parameter from the algorithm
 * @param numSpec Total number of spectra
 * @param numBins Number of bins per spectrum
 * @return Number of spectra per chunk
 */
std::size_t chooseChunkSize(int userChunkSize, std::size_t numSpec, std::size_t numBins) {
  if (userChunkSize == -1)
    return numSpec;

  if (userChunkSize == 0) {
    const std::size_t bytesPerSpectrum = numBins * sizeof(double) * 2;
    if (bytesPerSpectrum == 0)
      return numSpec;

    const std::size_t chunk = DEFAULT_TARGET_CHUNK_BYTES / bytesPerSpectrum;
    return std::clamp(chunk, std::size_t{1}, numSpec);
  }

  return std::clamp(static_cast<std::size_t>(userChunkSize), std::size_t{1}, numSpec);
}

} // namespace Mantid::MPIAlgorithms::Chunking
