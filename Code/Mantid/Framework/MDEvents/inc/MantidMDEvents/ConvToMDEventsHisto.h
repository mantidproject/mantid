#ifndef H_CONVERT_TO_MDEVENTS_HISTO_WS
#define H_CONVERT_TO_MDEVENTS_HISTO_WS


#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"

#include "MantidMDEvents/MDEventWSWrapper.h"
#include "MantidMDEvents/MDEvent.h"

#include "MantidMDEvents/ConvToMDEventsBase.h"
#include "MantidMDEvents/ConvToMDPreprocDet.h"
// coordinate transformation
#include "MantidMDEvents/MDTransfInterface.h"

namespace Mantid
{
namespace MDEvents
{
/** The templated class to transform matrix workspace into MDEvent workspace when matrix workspace is ragged
   *
   * @date 11-10-2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

        File/ change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

// service variable used for efficient filling of the MD event WS  -> should be moved to configuration?
#define SPLIT_LEVEL  8192
//-----------------------------------------------
class ConvToMDEventsHisto: public ConvToMDEventsBase
{

public:
    size_t  initialize(const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper);

    void runConversion(API::Progress *pProg);
private:
  // conversion chunk; it does not used at the moment but can be provided to thread pull to do the job
   size_t conversionChunk(size_t job_ID){UNUSED_ARG(job_ID); return 0;}

};

 
} // endNamespace MDAlgorithms
} // endNamespace Mantid

#endif
