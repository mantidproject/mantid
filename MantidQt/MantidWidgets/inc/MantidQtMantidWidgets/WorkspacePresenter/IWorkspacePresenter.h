#ifndef MANTID_MANTIDWIDGETS_IWORKSPACEPRESENTER_H_
#define MANTID_MANTIDWIDGETS_IWORKSPACEPRESENTER_H_

#include "MantidQtMantidWidgets/WorkspacePresenter/ADSNotifiable.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/ViewNotifiable.h"

namespace MantidQt {
namespace MantidWidgets {
/**
\class  IWorkspacePresenter
\brief  Interfact for Presenter class for Workspace dock in MantidPlot UI
\author Lamar Moore
\date   24-08-2016
\version 1.0


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
*/

class IWorkspacePresenter : public ADSNotifiable, public ViewNotifiable {
public:
	virtual ~IWorkspacePresenter() = default;
};

} // namespace MantidWidgets
} // namespace MantidQt
#endif //MANTID_MANTIDWIDGETS_IWORKSPACEPRESENTER_H_