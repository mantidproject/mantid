#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTER_H

#include "MantidAPI/IAlgorithm.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflRunsTabPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMainPresenter.h"
#include <boost/shared_ptr.hpp>

namespace MantidQt {

namespace MantidWidgets {
// Forward decs
class ProgressableView;
class DataProcessorPresenter;
}

namespace CustomInterfaces {

// Forward decs
class IReflMainWindowPresenter;
class IReflRunsTabView;
class IReflSearcher;
class ReflSearchModel;
class ReflTransferStrategy;

using MantidWidgets::DataProcessorPresenter;
using MantidWidgets::ProgressableView;

/** @class ReflRunsTabPresenter

ReflRunsTabPresenter is a presenter class for the Reflectometry Interface. It
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
class MANTIDQT_CUSTOMINTERFACES_DLL ReflRunsTabPresenter
    : public IReflRunsTabPresenter,
      public MantidQt::MantidWidgets::DataProcessorMainPresenter {
public:
  ReflRunsTabPresenter(IReflRunsTabView *mainView,
                       ProgressableView *progressView,
                       std::vector<DataProcessorPresenter *> tablePresenter,
                       boost::shared_ptr<IReflSearcher> searcher =
                           boost::shared_ptr<IReflSearcher>());
  ~ReflRunsTabPresenter() override;
  void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) override;
  void notify(IReflRunsTabPresenter::Flag flag) override;
  void notifyADSChanged(const QSet<QString> &workspaceList) override;
  QString getPreprocessingProperties() const override;
  /// Handle data reduction paused/resumed
  /// Global options (inherited from DataProcessorMainPresenter)
  QString getPreprocessingOptionsAsString() const override;
  QString getProcessingOptions() const override;
  QString getPostprocessingOptions() const override;
  QString getTimeSlicingValues() const override;
  QString getTimeSlicingType() const override;
  /// Handle data reduction paused/resumed
  void pause() const override;
  void resume() const override;
  /// Reduction paused/resumed confirmation handler
  void confirmReductionPaused() const override;
  void confirmReductionResumed() const override;

private:
  /// The search model
  boost::shared_ptr<ReflSearchModel> m_searchModel;
  /// The main view we're managing
  IReflRunsTabView *m_view;
  /// The progress view
  ProgressableView *m_progressView;
  /// The data processor presenters stored in a vector
  std::vector<DataProcessorPresenter *> m_tablePresenters;
  /// The main presenter
  IReflMainWindowPresenter *m_mainPresenter;
  /// The search implementation
  boost::shared_ptr<IReflSearcher> m_searcher;
  /// The current transfer method
  std::string m_currentTransferMethod;
  /// Legacy transfer method
  static const std::string LegacyTransferMethod;
  /// Measure transfer method
  static const std::string MeasureTransferMethod;

  /// searching
  void search();
  void populateSearch(Mantid::API::IAlgorithm_sptr searchAlg);
  void transfer();
  void pushCommands();
  /// transfer strategy
  std::unique_ptr<ReflTransferStrategy> getTransferStrategy();
  /// change the instrument
  void changeInstrument();
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTER_H */
