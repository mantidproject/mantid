#ifndef MANTID_VATES_MD_LOADING_VIEW_SIMPLE_H
#define MANTID_VATES_MD_LOADING_VIEW_SIMPLE_H

#include "MantidVatesAPI/MDLoadingView.h"

namespace Mantid {
namespace VATES {

/** MDLoadingViewSimple : Provides an almost hollow MDLoadingView
which is used by non-paraview based implementations such as the
SaveMDWorkspaceToVTK algorithm.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport MDLoadingViewSimple : public MDLoadingView {
public:
  double getTime() const override;
  void setTime(double time);

  size_t getRecursionDepth() const override;
  void setRecursionDepth(size_t recursionDepth);

  bool getLoadInMemory() const override;
  void setLoadInMemory(bool loadInMemory);

private:
  double m_time = 0.0;
  size_t m_recursionDepth = 5;
  bool m_loadInMemory = true;
};
} // namespace VATES
} // namespace Mantid

#endif
