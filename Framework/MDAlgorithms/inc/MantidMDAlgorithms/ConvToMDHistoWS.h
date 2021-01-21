// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMDAlgorithms/ConvToMDBase.h"
#include "MantidMDAlgorithms/MDEventWSWrapper.h"
#include "MantidMDAlgorithms/MDTransfInterface.h"

namespace Mantid {

// Forward declaration
namespace API {
class Progress;
}
namespace MDAlgorithms {
/** The class to transform matrix workspace into MDEvent workspace when matrix
  workspace is ragged 2D workspace
  *
  *
  * See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation
  for detailed description of this
  * class place in the algorithms hierarchy.
  *
  * @date 11-10-2011
*/

//-----------------------------------------------
class ConvToMDHistoWS : public ConvToMDBase {

public:
  /// Default constructor
  ConvToMDHistoWS();

  size_t initialize(const MDWSDescription &WSD, std::shared_ptr<MDEventWSWrapper> inWSWrapper,
                    bool ignoreZeros) override;

  void runConversion(API::Progress *pProgress) override;

private:
  // the number of spectra to process by single computational thread;
  size_t m_spectraChunk;
  // the size of temporary buffer, each thread stores data in before adding
  // these data to target MD workspace;
  size_t m_bufferSize;
  // internal function used to identify m_spectraChunk and m_bufferSize
  void estimateThreadWork(size_t nThreads, size_t specSize, size_t nPointsToProcess);
  // the function does a chunk of work. Expected to run on a thread.
  size_t conversionChunk(size_t startSpectra) override;
};

} // namespace MDAlgorithms
} // namespace Mantid
