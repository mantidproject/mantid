#ifndef MANTID_MDALGORITHMS_WS_SELECTOR_H
#define MANTID_MDALGORITHMS_WS_SELECTOR_H

#include "MantidMDAlgorithms/ConvToMDBase.h"

namespace Mantid {
namespace MDAlgorithms {
/** small class to select proper solver as function of the workspace kind and
  (possibly, in a future) other workspace parameters.
  * may be replaced by usual mantid factory in a future;
  *
  *
  * See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation
  for detailed description of this
  * class place in the algorithms hierarchy.
  *
  * @date 25-05-2012

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

class DLLExport ConvToMDSelector {
public:
  /// function which selects the convertor depending on workspace type and
  /// (possibly, in a future) some workspace properties
  boost::shared_ptr<ConvToMDBase>
  convSelector(API::MatrixWorkspace_sptr inputWS,
               boost::shared_ptr<ConvToMDBase> &currentSolver) const;
};
} // namespace MDAlgorithms
} // namespace Mantid

#endif
