#ifndef MANTID_MDALGORITHMS_CONV_TOMD_HISTOWS_H
#define MANTID_MDALGORITHMS_CONV_TOMD_HISTOWS_H

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/MDEvent.h"
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

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        File/ change history is stored at:
  <https://github.com/mantidproject/mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

//-----------------------------------------------
class ConvToMDHistoWS : public ConvToMDBase {

public:
  size_t
  initialize(const MDWSDescription &WSD,
             boost::shared_ptr<MDEventWSWrapper> inWSWrapper,
             bool ignoreZeros);

  void runConversion(API::Progress *pProgress);

private:
  // the number of spectra to process by single computational thread;
  size_t m_spectraChunk;
  // the size of temporary buffer, each thread stores data in before adding
  // these data to target MD workspace;
  size_t m_bufferSize;
  // internal function used to identify m_spectraChunk and m_bufferSize
  void estimateThreadWork(size_t nThreads, size_t specSize,
                          size_t nPointsToProcess);
  // the function does a chunk of work. Expected to run on a thread.
  size_t conversionChunk(size_t job_ID);
};

} // endNamespace MDAlgorithms
} // endNamespace Mantid

#endif
