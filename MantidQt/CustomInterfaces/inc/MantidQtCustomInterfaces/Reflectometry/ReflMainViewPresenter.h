#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H

#include "MantidAPI/IAlgorithm.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/WorkspaceReceiver.h"
#include <boost/shared_ptr.hpp>

namespace MantidQt {

namespace MantidWidgets {
// Forward decs
class ProgressableView;
class DataProcessorPresenter;
}

namespace CustomInterfaces {

// Forward decs
class ReflMainView;
class ReflSearchModel;
class IReflSearcher;
class ReflTransferStrategy;

using MantidWidgets::DataProcessorPresenter;
using MantidWidgets::ProgressableView;

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
      public MantidQt::MantidWidgets::WorkspaceReceiver {
public:
  ReflMainViewPresenter(
      ReflMainView *mainView, ProgressableView *progressView,
      boost::shared_ptr<DataProcessorPresenter> tablePresenter,
      boost::shared_ptr<IReflSearcher> searcher =
          boost::shared_ptr<IReflSearcher>());
  ~ReflMainViewPresenter() override;
  void notify(IReflPresenter::Flag flag) override;
  void notify(WorkspaceReceiver::Flag flag) override;

protected:
  // the search model
  boost::shared_ptr<ReflSearchModel> m_searchModel;
  // the main view we're managing
  ReflMainView *m_view;
  // The table view's presenter
  boost::shared_ptr<DataProcessorPresenter> m_tablePresenter;
  // The progress view
  ProgressableView *m_progressView;
  // the search implementation
  boost::shared_ptr<IReflSearcher> m_searcher;
  // searching
  void search();
  void populateSearch(Mantid::API::IAlgorithm_sptr searchAlg);
  void transfer();
  void pushCommands();
  void checkForUnsavedChangesOnExit();

private:
  static const std::string LegacyTransferMethod;
  static const std::string MeasureTransferMethod;

  std::unique_ptr<ReflTransferStrategy> getTransferStrategy();
};
}
}
#endif
