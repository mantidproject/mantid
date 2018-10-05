// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H
#define MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H

#include "IReflMainWindowView.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_ReflMainWindowWidget.h"

#include <QCloseEvent>

namespace MantidQt {
namespace CustomInterfaces {

class IReflEventTabPresenter;
class IReflMainWindowPresenter;
class IReflRunsTabPresenter;
class IReflSettingsTabPresenter;
class IReflSaveTabPresenter;

/** @class ReflMainWindowView

ReflMainWindowView is the concrete main window view implementing the
functionality defined by the interface IReflMainWindowView
*/
class QtReflMainWindowView : public MantidQt::API::UserSubWindow,
                             public IReflMainWindowView {
  Q_OBJECT
public:
  /// Constructor
  explicit QtReflMainWindowView(QWidget *parent = nullptr);
  /// Destructor
  ~QtReflMainWindowView() override;
  /// Name of the interface
  static std::string name() { return "ISIS Reflectometry"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Reflectometry"; }

  /// Dialog to show an error message
  void giveUserCritical(const std::string &prompt,
                        const std::string &title) override;
  /// Dialog to show information
  void giveUserInfo(const std::string &prompt,
                    const std::string &title) override;
  /// Run a python algorithm
  std::string runPythonAlgorithm(const std::string &pythonCode) override;

  /// Close window handler
  void closeEvent(QCloseEvent *event) override;

public slots:
  void helpPressed();

private:
  /// Initializes the interface
  void initLayout() override;
  /// Creates the 'Runs' tab
  std::unique_ptr<IReflRunsTabPresenter> createRunsTab();
  /// Creates the 'Event Handling' tab
  IReflEventTabPresenter *createEventTab();
  /// Creates the 'Settings' tab
  IReflSettingsTabPresenter *createSettingsTab();
  /// Creates the 'Save ASCII' tab
  std::unique_ptr<IReflSaveTabPresenter> createSaveTab();

  /// Interface definition with widgets for the main interface window
  Ui::RelMainWindowWidget m_ui;
  /// The presenter handling this view
  std::unique_ptr<IReflMainWindowPresenter> m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H */
