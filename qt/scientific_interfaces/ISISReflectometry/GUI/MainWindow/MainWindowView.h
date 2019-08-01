// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_MAINWINDOWVIEW_H
#define MANTID_ISISREFLECTOMETRY_MAINWINDOWVIEW_H

#include "GUI/Common/IMessageHandler.h"
#include "GUI/Common/IPythonRunner.h"
#include "IMainWindowPresenter.h"
#include "IMainWindowView.h"
#include "MainWindowPresenter.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_MainWindowWidget.h"

#include <QCloseEvent>

namespace MantidQt {
namespace CustomInterfaces {

/** @class MainWindowView

MainWindowView is the concrete main window view implementing the
functionality defined by the interface IMainWindowView
*/
class MainWindowView : public MantidQt::API::UserSubWindow,
                       public IMainWindowView,
                       public IMessageHandler,
                       public IPythonRunner {
  Q_OBJECT
public:
  explicit MainWindowView(QWidget *parent = nullptr);
  void subscribe(MainWindowSubscriber *notifyee) override;

  static std::string name() { return "ISIS Reflectometry"; }
  static QString categoryInfo() { return "Reflectometry"; }
  std::string runPythonAlgorithm(const std::string &pythonCode) override;

  virtual std::vector<IBatchView *> batches() const override;

  void closeEvent(QCloseEvent *event) override;

  IBatchView *newBatch() override;
  void removeBatch(int batchIndex) override;

  void giveUserCritical(const std::string &prompt,
                        const std::string &title) override;
  void giveUserInfo(const std::string &prompt,
                    const std::string &title) override;
  bool askUserYesNo(const std::string &prompt,
                    const std::string &title) override;

public slots:
  void helpPressed();
  void onTabCloseRequested(int tabIndex);
  void onNewBatchRequested(bool);
  void onLoadBatchRequested(bool);
  void onSaveBatchRequested(bool);

private:
  /// Initializes the interface
  void initLayout() override;
  /// Interface definition with widgets for the main interface window
  Ui::MainWindowWidget m_ui;
  MainWindowSubscriber *m_notifyee;
  /// The presenter handling this view. It is not normal in MVP for a view to
  /// have ownership of its presenter, but due to the way interfaces get
  /// instantiated this is currently necessary for MainWindowView. Direct use
  /// of m_presenter should be avoided - use m_notifyee instead.
  std::unique_ptr<MainWindowPresenter> m_presenter;
  std::vector<IBatchView *> m_batchViews;

  friend class Encoder;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_MAINWINDOWVIEW_H */
