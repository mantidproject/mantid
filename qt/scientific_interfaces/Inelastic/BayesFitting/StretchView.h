// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "StretchData.h"
#include "ui_Stretch.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL IStretchViewSubscriber {
public:
  virtual void notifySaveClicked() = 0;
  virtual void notifyPlotClicked() = 0;
  virtual void notifyPlotContourClicked() = 0;
  virtual void notifyPlotCurrentPreviewClicked() = 0;
  virtual void notifyPreviewSpecChanged(int specNum) = 0;
};

class MANTIDQT_INELASTIC_DLL IStretchView {
public:
  virtual ~IStretchView() = default;

  virtual void subscribePresenter(IStretchViewSubscriber *presenter) = 0;
  virtual void loadSettings(const QSettings &settings) = 0;
  virtual void applySettings(std::map<std::string, QVariant> const &settings) = 0;
  virtual void validateUserInput(IUserInputValidator *validator) const = 0;

  virtual StretchRunData getRunData() const = 0;
  virtual CurrentPreviewData getCurrentPreviewData() const = 0;
  virtual std::string getPlotType() const = 0;
  virtual std::string getPlotContour() const = 0;
  virtual IRunView *getRunWidget() const = 0;

  virtual void setupFitOptions() = 0;
  virtual void setupPropertyBrowser() = 0;
  virtual void setupPlotOptions() = 0;

  virtual void setFileExtensionsByName(bool filter) = 0;
  virtual void setLoadHistory(bool doLoadHistory) = 0;

  virtual void resetPlotContourOptions(const std::vector<std::string> &contourNames) = 0;
  virtual int displaySaveDirectoryMessage() = 0;

  virtual void setPlotADSEnabled(bool enabled) = 0;
  virtual void setPlotResultEnabled(bool enabled) = 0;
  virtual void setPlotContourEnabled(bool enabled) = 0;
  virtual void setSaveResultEnabled(bool enabled) = 0;
  virtual void setButtonsEnabled(bool enabled) = 0;
  virtual void setPlotResultIsPlotting(bool plotting) = 0;
  virtual void setPlotContourIsPlotting(bool plotting) = 0;
};

class MANTIDQT_INELASTIC_DLL StretchView : public QWidget, public IStretchView {
  Q_OBJECT
public:
  explicit StretchView(QWidget *parent = nullptr);
  ~StretchView() override = default;

  void subscribePresenter(IStretchViewSubscriber *presenter) override;
  void loadSettings(const QSettings &settings) override;
  void applySettings(std::map<std::string, QVariant> const &settings) override;
  void validateUserInput(IUserInputValidator *validator) const override;

  StretchRunData getRunData() const override;
  CurrentPreviewData getCurrentPreviewData() const override;
  std::string getPlotType() const override;
  std::string getPlotContour() const override;
  IRunView *getRunWidget() const override;

  void setupFitOptions() override;
  void setupPropertyBrowser() override;
  void setupPlotOptions() override;

  void setFileExtensionsByName(bool filter) override;
  void setLoadHistory(bool doLoadHistory) override;

  void resetPlotContourOptions(const std::vector<std::string> &contourNames) override;
  int displaySaveDirectoryMessage() override;

  void setPlotADSEnabled(bool enabled) override;
  void setPlotResultEnabled(bool enabled) override;
  void setPlotContourEnabled(bool enabled) override;
  void setSaveResultEnabled(bool enabled) override;
  void setButtonsEnabled(bool enabled) override;
  void setPlotResultIsPlotting(bool plotting) override;
  void setPlotContourIsPlotting(bool plotting) override;

private slots:
  void minValueChanged(double min);
  void maxValueChanged(double max);
  void propertiesUpdated(QtProperty *prop, double val);
  void handleSampleInputReady(const QString &filename);
  void previewSpecChanged(int value);

  void saveWorkspacesClicked();
  void plotWorkspacesClicked();
  void plotContourClicked();
  void plotCurrentPreviewClicked();

private:
  void formatTreeWidget(QtTreePropertyBrowser *treeWidget, QMap<QString, QtProperty *> const &properties) const;

private:
  Ui::Stretch m_uiForm;
  QtDoublePropertyManager *m_dblManager;
  QMap<QString, QtProperty *> m_properties;
  IStretchViewSubscriber *m_presenter;
  QtTreePropertyBrowser *m_propTree;
};
} // namespace CustomInterfaces
} // namespace MantidQt
