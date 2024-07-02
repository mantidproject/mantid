// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IIqtView.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "ui_IqtTab.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

class IIqtPresenter;

class MANTIDQT_INELASTIC_DLL IqtView : public QWidget, public IIqtView {
  Q_OBJECT

public:
  IqtView(QWidget *parent = nullptr);
  ~IqtView();

  void subscribePresenter(IIqtPresenter *presenter) override;

  IRunView *getRunView() const override;
  IOutputPlotOptionsView *getPlotOptions() const override;
  MantidWidgets::DataSelector *getDataSelector(const std::string &selectorName) const override;
  void plotInput(MatrixWorkspace_sptr inputWS, int spectrum) override;
  void setPreviewSpectrumMaximum(int value) override;
  void updateDisplayedBinParameters() override;
  void setRangeSelectorDefault(const MatrixWorkspace_sptr inputWorkspace, const QPair<double, double> &range) override;
  void setSampleFBSuffixes(const QStringList &suffix) override;
  void setSampleWSSuffixes(const QStringList &suffix) override;
  void setResolutionFBSuffixes(const QStringList &suffix) override;
  void setResolutionWSSuffixes(const QStringList &suffix) override;
  void setSaveResultEnabled(bool enabled) override;
  void setWatchADS(bool watch) override;
  void setup() override;
  void showMessageBox(const std::string &message) const override;

  // getters for properties
  std::string getSampleName() const override;

private slots:
  void notifySampDataReady(const QString &filename);
  void notifyResDataReady(const QString &resFilename);
  void notifyIterationsChanged(int iterations);
  void notifySaveClicked();
  void notifyPlotCurrentPreview();
  void notifyErrorsClicked(int state);
  void notifyEnableNormalizationClicked(int state);
  void notifyPreviewSpectrumChanged(int spectra);
  void notifyUpdateEnergyRange(int state);
  void notifyValueChanged(QtProperty *prop, double value);
  void notifyRangeChanged(double min, double max);
  void notifyUpdateRangeSelector(QtProperty *prop, double val);

private:
  void setRangeSelectorMax(QtProperty *minProperty, QtProperty *maxProperty, RangeSelector *rangeSelector,
                           double newValue);
  void setRangeSelectorMin(QtProperty *minProperty, QtProperty *maxProperty, RangeSelector *rangeSelector,
                           double newValue);

  Ui::IqtTab m_uiForm;

  // Presenter
  IIqtPresenter *m_presenter;
  QtTreePropertyBrowser *m_iqtTree;
  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;
  /// Double manager to create properties
  QtDoublePropertyManager *m_dblManager;
  /// Double editor factory for the properties browser
  DoubleEditorFactory *m_dblEdFac;
};

} // namespace CustomInterfaces
} // namespace MantidQt