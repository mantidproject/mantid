#ifndef MANTID_MDALGORITHMS_CONV_TOMD_EVENTSWS_H
#define MANTID_MDALGORITHMS_CONV_TOMD_EVENTSWS_H
//
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Algorithm.h"
#include <vector>

#include "MantidDataObjects/EventWorkspace.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/PhysicalConstants.h"

#include "MantidDataObjects/MDEventWSWrapper.h"
#include "MantidDataObjects/MDEvent.h"

#include "MantidDataObjects/ConvToMDBase.h"
// coordinate transformation
#include "MantidDataObjects/MDTransfFactory.h"

namespace Mantid {
namespace MDAlgorithms {
/** The class specializes ConvToDataObjectsBase for the case when the conversion
  occurs from Events WS to the MD events WS
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

// Class to process event workspace by direct conversion:

class ConvToDataObjectsWS : public ConvToMDBase {
public:
  size_t initialize(const DataObjects::MDWSDescription &WSD,
                    boost::shared_ptr<DataObjects::MDEventWSWrapper> inWSWrapper,
                    bool ignoreZeros);
  void runConversion(API::Progress *pProgress);

private:
  // function runs the conversion on
  virtual size_t conversionChunk(size_t workspaceIndex);
  // the pointer to the source event workspace as event ws does not work through
  // the public Matrix WS interface
  DataObjects::EventWorkspace_const_sptr m_EventWS;

  /**function converts particular type of events into MD space and add these
   * events to the workspace itself    */
  template <class T> size_t convertEventList(size_t workspaceIndex);
};

} // endNamespace DataObjects
} // endNamespace Mantid

#endif
