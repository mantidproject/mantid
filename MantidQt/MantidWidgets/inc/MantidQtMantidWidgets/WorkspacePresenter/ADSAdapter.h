#ifndef MANTID_MANTIDWIDGETS_ADSADAPTER_H_
#define MANTID_MANTIDWIDGETS_ADSADAPTER_H_

#include "MantidQtMantidWidgets/WorkspacePresenter/IADSAdapter.h"
#include <MantidQtAPI/WorkspaceObserver.h>

namespace MantidQt {
namespace MantidWidgets {
/**
\class  ADSAdapter
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
class ADSAdapter : public IADSAdapter, public MantidQt::API::WorkspaceObserver {
public:
  void registerPresenter(Presenter_wptr presenter) override;
  Mantid::API::Workspace_sptr getWorkspace(const std::string &wsname) const override;

private:
  Presenter_wptr m_presenter;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif //MANTID_MANTIDWIDGETS_ADSADAPTER_H_