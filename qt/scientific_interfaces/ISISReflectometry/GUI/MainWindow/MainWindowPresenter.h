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
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class IMainWindowView;

/** @class MainWindowPresenter

MainWindowPresenter is the concrete main window presenter implementing the
functionality defined by the interface IMainWindowPresenter.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL MainWindowPresenter
    : public IMainWindowPresenter {
public:
  /// Constructor
  MainWindowPresenter(IMainWindowView *view,
                      BatchPresenterFactory batchPresenterFactory);
  bool isProcessing() const override;
  void notifyHelpPressed() override;
  void notifyNewBatchRequested() override;
  void notifyCloseBatchRequested(int batchIndex) override;

private:
  void showHelp();
  IMainWindowView *m_view;
  BatchPresenterFactory m_batchPresenterFactory;
  std::vector<std::shared_ptr<IBatchPresenter>> m_batchPresenters;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_MAINWINDOWPRESENTER_H */
