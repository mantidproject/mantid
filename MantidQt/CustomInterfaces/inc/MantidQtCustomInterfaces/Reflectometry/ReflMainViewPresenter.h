#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSearcher.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTransferStrategy.h"
#include "MantidQtCustomInterfaces/Reflectometry/WorkspaceReceiver.h"

#include <Poco/AutoPtr.h>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
// Forward decs
class ProgressableView;
class IReflTablePresenter;

/** @class ReflMainViewPresenter

ReflMainViewPresenter is a presenter class for the Reflectometry Interface. It
handles any interface functionality and model manipulation.

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
class MANTIDQT_CUSTOMINTERFACES_DLL ReflMainViewPresenter
    : public IReflPresenter,
      public WorkspaceReceiver {
public:
  ReflMainViewPresenter(ReflMainView *mainView,
                        IReflTablePresenter *tablePresenter,
                        ProgressableView *progressView,
                        boost::shared_ptr<IReflSearcher> searcher =
                            boost::shared_ptr<IReflSearcher>());
  ~ReflMainViewPresenter() override;
  void notify(IReflPresenter::Flag flag) override;
  void notify(WorkspaceReceiver::Flag flag) override;

protected:
  // the search model
  ReflSearchModel_sptr m_searchModel;
  // the main view we're managing
  ReflMainView *m_view;
  // The table view's presenter
  IReflTablePresenter *m_tablePresenter;
  // The progress view
  ProgressableView *m_progressView;
  // the search implementation
  boost::shared_ptr<IReflSearcher> m_searcher;
  // searching
  void search();
  void populateSearch(Mantid::API::IAlgorithm_sptr searchAlg);
  void transfer();
  void pushCommands();

private:
  static const std::string LegacyTransferMethod;
  static const std::string MeasureTransferMethod;

  std::unique_ptr<ReflTransferStrategy> getTransferStrategy();
};
}
}
#endif