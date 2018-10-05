// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H
#define MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H

#include "IReflMainWindowPresenter.h"
#include "IReflMainWindowView.h"
#include "IReflMessageHandler.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ReflMainWindowPresenter.h"
#include "ui_ReflMainWindowWidget.h"

#include <QCloseEvent>

namespace MantidQt {
namespace CustomInterfaces {

/** @class ReflMainWindowView

ReflMainWindowView is the concrete main window view implementing the
functionality defined by the interface IReflMainWindowView
*/
class QtReflMainWindowView : public MantidQt::API::UserSubWindow,
                             public IReflMainWindowView,
                             public IReflMessageHandler {
  Q_OBJECT
public:
  explicit QtReflMainWindowView(QWidget *parent = nullptr);
  void subscribe(ReflMainWindowSubscriber *notifyee) override;

  static std::string name() { return "ISIS Reflectometry"; }
  static QString categoryInfo() { return "Reflectometry"; }
  std::string runPythonAlgorithm(const std::string &pythonCode) override;

  virtual std::vector<IReflBatchView *> batches() const override;

  void closeEvent(QCloseEvent *event) override;

  IReflBatchView *newBatch() override;
  void removeBatch(int batchIndex) override;

  void giveUserCritical(const std::string &prompt,
                        const std::string &title) override;
  void giveUserInfo(const std::string &prompt,
                    const std::string &title) override;

public slots:
  void helpPressed();
  void onTabCloseRequested(int tabIndex);
  void onNewBatchRequested(bool);

private:
  /// Initializes the interface
  void initLayout() override;
  /// Interface definition with widgets for the main interface window
  Ui::ReflMainWindowWidget m_ui;
  /// The presenter handling this view
  ReflMainWindowSubscriber *m_notifyee;
  boost::optional<ReflMainWindowPresenter> m_presenter;
  std::vector<IReflBatchView *> m_batchViews;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H */
