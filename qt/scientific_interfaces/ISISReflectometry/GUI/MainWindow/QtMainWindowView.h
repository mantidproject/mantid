// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Common/IFileHandler.h"
#include "GUI/Common/IPythonRunner.h"
#include "GUI/Common/IReflMessageHandler.h"
#include "IMainWindowPresenter.h"
#include "IMainWindowView.h"
#include "MainWindowPresenter.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_MainWindowWidget.h"

#include <QCloseEvent>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class QtOptionsDialogView;

/** @class QtMainWindowView

MainWindowView is the concrete main window view implementing the
functionality defined by the interface IMainWindowView
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL QtMainWindowView : public MantidQt::API::UserSubWindow,
                                                        public IMainWindowView,
                                                        public IReflMessageHandler,
                                                        public IFileHandler,
                                                        public IPythonRunner {
  Q_OBJECT
public:
  explicit QtMainWindowView(QWidget *parent = nullptr);
  ~QtMainWindowView();
  QtMainWindowView(QtMainWindowView const &) = delete;
  QtMainWindowView(QtMainWindowView &&) = delete;
  QtMainWindowView &operator=(QtMainWindowView const &) = delete;
  QtMainWindowView &operator=(QtMainWindowView &&) = delete;

  void subscribe(MainWindowSubscriber *notifyee) override;

  // cppcheck-suppress returnTempReference
  static std::string name() { return "ISIS Reflectometry"; }
  static QString categoryInfo() { return "Reflectometry"; }
  std::string runPythonAlgorithm(const std::string &pythonCode) override;

  virtual std::vector<IBatchView *> batches() const override;

  void closeEvent(QCloseEvent *event) override;
  void acceptCloseEvent() override;
  void ignoreCloseEvent() override;

  IBatchView *newBatch() override;
  void removeBatch(int batchIndex) override;

  void giveUserCritical(const std::string &prompt, const std::string &title) override;
  void giveUserWarning(const std::string &prompt, const std::string &title) override;
  void giveUserInfo(const std::string &prompt, const std::string &title) override;
  bool askUserOkCancel(const std::string &prompt, const std::string &title) override;
  std::string askUserForLoadFileName(std::string const &filter) override;
  std::string askUserForSaveFileName(std::string const &filter) override;

  void disableSaveAndLoadBatch() override;
  void enableSaveAndLoadBatch() override;

  // TODO Remove Qt types from this interface - conversion should be done in
  // QtJSONUtils if possible
  void saveJSONToFile(std::string const &filename, QMap<QString, QVariant> const &map) override;
  QMap<QString, QVariant> loadJSONFromFile(std::string const &filename) override;
  void saveCSVToFile(std::string const &filename, std::string const &content) const override;
  bool fileExists(std::string const &filepath) const override;
  std::string getFullFilePath(std::string const &filename) const override;
  int getTabIndex() const override;

public slots:
  void helpPressed();
  void onTabCloseRequested(int tabIndex);
  void onNewBatchRequested(bool);
  void onLoadBatchRequested(bool);
  void onSaveBatchRequested(bool);
  void onShowOptionsRequested(bool);
  void onShowSlitCalculatorRequested(bool);

private:
  /// Initializes the interface
  void initLayout() override;
  /// Interface definition with widgets for the main interface window
  Ui::MainWindowWidget m_ui;
  MainWindowSubscriber *m_notifyee;
  /// The presenter handling this view. It is not normal in MVP for a view to
  /// have ownership of its presenter, but due to the way interfaces get
  /// instantiated this is currently necessary for QtMainWindowView. Direct use
  /// of m_presenter should be avoided - use m_notifyee instead.
  std::unique_ptr<MainWindowPresenter> m_presenter;
  std::unique_ptr<QtOptionsDialogView> m_optionsDialogView;
  std::vector<IBatchView *> m_batchViews;
  int m_batchIndex;
  QCloseEvent *m_closeEvent;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
  friend class DecoderTest;
  friend class EncoderTest;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
