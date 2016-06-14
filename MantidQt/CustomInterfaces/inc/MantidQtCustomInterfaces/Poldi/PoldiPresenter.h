#ifndef MANTIDQTCUSTOMINTERFACES_POLDIPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_POLDIPRESENTER_H_

#include "MantidQtCustomInterfaces/Poldi/IPoldiPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/WorkspaceReceiver.h"

using MantidQt::MantidWidgets::DataProcessorPresenter;
using MantidQt::MantidWidgets::WorkspaceReceiver;

namespace MantidQt {
namespace CustomInterfaces {

class IPoldiView;

/** @class PoldiPresenter

PoldiPresenter is a concrete presenter for the Poldi interface. It handles any
interface functionality and model manipulation.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class PoldiPresenter : public IPoldiPresenter, public WorkspaceReceiver {
public:
  PoldiPresenter(IPoldiView *view,
                 boost::shared_ptr<DataProcessorPresenter> presenter);
  ~PoldiPresenter() override;
  void notify(IPoldiPresenter::Flag flag) override;
	void notify(WorkspaceReceiver::Flag flag) override;
private:
  // Populates a demo table
  void loadDemoTable();
	// Pushes the list of table commands
	void updateToolBar();

  // The view we're managing
  IPoldiView *m_view;
  // The DataProcessor presenter
  boost::shared_ptr<DataProcessorPresenter> m_presenter;
};
}
}

#endif /*MANTIDQTCUSTOMINTERFACES_POLDIPRESENTER_H_*/
