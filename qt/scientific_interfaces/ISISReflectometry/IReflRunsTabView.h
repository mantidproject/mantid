// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNSTABVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNSTABVIEW_H

#include "DllConfig.h"
#include "GUI/RunsTable/IRunsTableView.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include <boost/shared_ptr.hpp>
#include <set>
#include <string>
#include <vector>

namespace MantidQt {

namespace MantidWidgets {
namespace DataProcessor {
class Command;
}
} // namespace MantidWidgets
namespace API {
class AlgorithmRunner;
}

namespace CustomInterfaces {

namespace DataProcessor = MantidWidgets::DataProcessor;
class IReflRunsTabPresenter;
class ReflSearchModel;

/** @class IReflRunsTabView

IReflRunsTabView is the base view class for the Reflectometry Interface. It
contains no QT specific functionality as that should be handled by a subclass.
*/

class MANTIDQT_ISISREFLECTOMETRY_DLL IReflRunsTabView
    : public MantidQt::MantidWidgets::ProgressableView {
public:
  virtual ~IReflRunsTabView() = default;

  virtual void subscribe(IReflRunsTabPresenter *presenter) = 0;
  virtual IRunsTableView *table() const = 0;

  // Connect the model
  virtual void showSearch(boost::shared_ptr<ReflSearchModel> model) = 0;

  // Setter methods
  virtual void setInstrumentList(const std::vector<std::string> &instruments,
                                 int defaultInstrumentIndex) = 0;
  virtual void updateMenuEnabledState(bool isProcessing) = 0;
  virtual void setAutoreduceButtonEnabled(bool enabled) = 0;
  virtual void setAutoreducePauseButtonEnabled(bool enabled) = 0;
  virtual void setTransferButtonEnabled(bool enabled) = 0;
  virtual void setInstrumentComboEnabled(bool enabled) = 0;
  virtual void setSearchTextEntryEnabled(bool enabled) = 0;
  virtual void setSearchButtonEnabled(bool enabled) = 0;
  virtual void setStartMonitorButtonEnabled(bool enabled) = 0;
  virtual void setStopMonitorButtonEnabled(bool enabled) = 0;

  // Accessor methods
  virtual std::set<int> getSelectedSearchRows() const = 0;
  virtual std::set<int> getAllSearchRows() const = 0;
  virtual std::string getSearchInstrument() const = 0;
  virtual std::string getSearchString() const = 0;

  virtual IReflRunsTabPresenter *getPresenter() const = 0;
  virtual boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getAlgorithmRunner() const = 0;
  virtual boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getMonitorAlgorithmRunner() const = 0;

  // Timer methods
  virtual void startTimer(const int millisecs) = 0;
  virtual void stopTimer() = 0;
  virtual void loginFailed(std::string const &fullError) = 0;

  virtual void noActiveICatSessions() = 0;
  virtual void missingRunsToTransfer() = 0;

  // Start an ICAT search
  virtual void startIcatSearch() = 0;

  // Start live data monitoring
  virtual void startMonitor() = 0;
  virtual void stopMonitor() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLRUNSTABVIEW_H */
