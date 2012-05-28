#ifndef H_CONV_TOMDEVENTS_SELECTOR
#define H_CONV_TOMDEVENTS_SELECTOR


#include "MantidMDAlgorithms/ConvToMDEventsEvents.h"
#include "MantidMDAlgorithms/ConvToMDEventsHisto.h"

namespace Mantid
{
namespace MDAlgorithms
{
/** small class to select proper solver as function of the workspace kind and (possibly, in a future) other workspace parameters. 
   * may be replaced by usual mantid factory in a future; 
   *
   * @date 25-05-2012

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

 class DLLExport ConvToMDEventsSelector
 {
 public:
 /// function which selects the convertor depending on workspace type and (possibly, in a future) some workspace properties
   boost::shared_ptr<ConvToMDEventsBase> convSelector(API::MatrixWorkspace_sptr inputWS,
                     boost::shared_ptr<ConvToMDEventsBase> currentSptr = boost::shared_ptr<ConvToMDEventsBase>())const;
 };
} // end MDAlgorithms Namespace
} // end Mantid Namespace


#endif