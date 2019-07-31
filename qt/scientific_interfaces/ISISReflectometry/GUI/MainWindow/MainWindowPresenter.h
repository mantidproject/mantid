// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_MAINWINDOWPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_MAINWINDOWPRESENTER_H

#include "Common/DllConfig.h"
#include "GUI/Batch/BatchPresenterFactory.h"
#include "IMainWindowPresenter.h"
#include "IMainWindowView.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class IMainWindowView;
class IMessageHandler;

/** @class MainWindowPresenter

MainWindowPresenter is the concrete main window presenter implementing the
functionality defined by the interface IMainWindowPresenter.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL MainWindowPresenter
    : public MainWindowSubscriber,
      public IMainWindowPresenter {
public:
  /// Constructor
  MainWindowPresenter(IMainWindowView *view, IMessageHandler *messageHandler,
                      BatchPresenterFactory batchPresenterFactory);

  // IMainWindowPresenter overrides
  bool isAnyBatchProcessing() const override;
  bool isAnyBatchAutoreducing() const override;
  void notifyAutoreductionResumed() override;
  void notifyAutoreductionPaused() override;

  // MainWindowSubscriber overrides
  void notifyHelpPressed() override;
  void notifyNewBatchRequested() override;
  void notifyCloseBatchRequested(int batchIndex) override;

private:
  void showHelp();
  void addNewBatch(IBatchView *batchView);

  IMainWindowView *m_view;
  IMessageHandler *m_messageHandler;
  BatchPresenterFactory m_batchPresenterFactory;
  std::vector<std::shared_ptr<IBatchPresenter>> m_batchPresenters;

  friend class ISISReflectometryEncoder;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_MAINWINDOWPRESENTER_H */
