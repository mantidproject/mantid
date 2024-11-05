// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_ResNorm.h"

#include <MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h>
#include <MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser>
#include <MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h>

namespace MantidQt::CustomInterfaces {
class IResNormPresenter;

class MANTIDQT_INELASTIC_DLL IResNormView {
public:
  virtual ~IResNormView() = default;
  virtual void subscribePresenter(IResNormPresenter *presenter) = 0;
  virtual IRunView *getRunView() const = 0;

  virtual void setup() = 0;

  virtual void setSuffixes(bool filter) = 0;
  virtual void setLoadHistory(bool doLoadHistory) = 0;
  virtual void loadSettings(QSettings const &settings) = 0;

  virtual double getDoubleManagerProperty(QString const &propName) const = 0;
  virtual MantidWidgets::PreviewPlot *getPreviewPlot() const = 0;
  virtual std::string getCurrentDataName(std::string const &selectorName) const = 0;
  virtual MantidWidgets::DataSelector *getDataSelector(std::string const &selectorName) const = 0;
  virtual bool plotHasCurve(std::string const &curveName) const = 0;
  virtual void updateSelectorRange(std::string const &filename) const = 0;
  virtual void setMaximumSpectrum(int maximum) const = 0;
  virtual void watchADS(bool watch) const = 0;

  virtual void addToPlot(std::string const &filename, std::string const &lineName, size_t spectraNo,
                         QColor color = QColor()) = 0;
  virtual void clearPlot() const = 0;

  virtual void setPlotResultEnabled(bool enabled) const = 0;
  virtual void setSaveResultEnabled(bool enabled) const = 0;
  virtual void setButtonsEnabled(bool enabled) const = 0;
  virtual void setPlotResultIsPlotting(bool plotting) const = 0;
};

class MANTIDQT_INELASTIC_DLL ResNormView : public QWidget, public IResNormView {
  Q_OBJECT

public:
  ResNormView(QWidget *parent = nullptr);
  ~ResNormView() override;
  void subscribePresenter(IResNormPresenter *presenter) override;
  IRunView *getRunView() const override;

  void setup() override;

  void setSuffixes(bool filter) override;
  void setLoadHistory(bool doLoadHistory) override;
  void loadSettings(QSettings const &settings) override;

  double getDoubleManagerProperty(QString const &propName) const override;
  MantidWidgets::PreviewPlot *getPreviewPlot() const override;
  std::string getCurrentDataName(std::string const &selectorName) const override;
  MantidWidgets::DataSelector *getDataSelector(std::string const &selectorName) const override;
  bool plotHasCurve(std::string const &curveName) const override;
  void updateSelectorRange(std::string const &filename) const override;
  void setMaximumSpectrum(int maximum) const override;
  void watchADS(bool watch) const override;

  void addToPlot(std::string const &filename, std::string const &lineName, size_t spectraNo,
                 QColor color = QColor()) override;
  void clearPlot() const override;

  void setPlotResultEnabled(bool enabled) const override;
  void setSaveResultEnabled(bool enabled) const override;
  void setButtonsEnabled(bool enabled) const override;
  void setPlotResultIsPlotting(bool plotting) const override;

private slots:
  /// Handle when the vanadium input is ready
  void notifyVanadiumInputReady(const QString &filename);
  /// Handle when the resolution input is ready
  void handleResolutionInputReady(const QString &filename);
  void notifyPlotCurrentPreviewClicked();
  void notifyPlotClicked();
  void notifySaveClicked();
  /// Slot for when the min range on the range selector changes
  void minValueChanged(double min);
  /// Slot for when the min range on the range selector changes
  void maxValueChanged(double max);
  /// Slot to update the guides when the range properties change
  void notifyDoublePropertyChanged(QtProperty *prop, double val);
  /// Slot to handle the preview spectrum being changed
  void notifyPreviewSpecChanged(int value);

private:
  /// Current preview spectrum
  int m_previewSpec;

  // Presenter
  IResNormPresenter *m_presenter;
  /// The ui form
  Ui::ResNorm m_uiForm;
  QtTreePropertyBrowser *m_propTree;
  QtDoublePropertyManager *m_dblManager;
  DoubleEditorFactory *m_dblEdFac;
  QMap<QString, QtProperty *> m_properties;
  QMap<std::string, MantidWidgets::DataSelector *> m_selectors;
};
} // namespace MantidQt::CustomInterfaces
