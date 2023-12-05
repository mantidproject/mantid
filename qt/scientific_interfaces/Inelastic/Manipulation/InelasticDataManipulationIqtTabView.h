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
#include "ui_InelasticDataManipulationIqtTab.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

class IIqtPresenter;

class MANTIDQT_INELASTIC_DLL InelasticDataManipulationIqtTabView : public QWidget, public IIqtView {
  Q_OBJECT

public:
  InelasticDataManipulationIqtTabView(QWidget *parent = nullptr);
  ~InelasticDataManipulationIqtTabView();

  void subscribePresenter(IIqtPresenter *presenter) override;

  OutputPlotOptionsView *getPlotOptions() const override;
  void plotInput(MatrixWorkspace_sptr inputWS, int spectrum) override;
  void setPreviewSpectrumMaximum(int value) override;
  void updateDisplayedBinParameters() override;
  void setRangeSelectorDefault(const MatrixWorkspace_sptr inputWorkspace, const QPair<double, double> &range) override;
  bool validate() override;
  void setSampleFBSuffixes(const QStringList &suffix) override;
  void setSampleWSSuffixes(const QStringList &suffix) override;
  void setResolutionFBSuffixes(const QStringList &suffix) override;
  void setResolutionWSSuffixes(const QStringList &suffix) override;
  void setRunEnabled(bool enabled) override;
  void setSaveResultEnabled(bool enabled) override;
  void setRunText(bool running) override;
  void setWatchADS(bool watch) override;
  void setup() override;
  void showMessageBox(const std::string &message) const override;

  // getters for properties
  std::string getSampleName() const override;

private slots:
  void notifySampDataReady(const QString &filename);
  void notifyResDataReady(const QString &resFilename);
  void notifyIterationsChanged(int iterations);
  void notifyRunClicked();
  void notifySaveClicked();
  void notifyPlotCurrentPreview();
  void notifyErrorsClicked(int state);
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

  Ui::InelasticDataManipulationIqtTab m_uiForm;
  QtTreePropertyBrowser *m_iqtTree;
  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;
  /// Double manager to create properties
  QtDoublePropertyManager *m_dblManager;
  /// Double editor factory for the properties browser
  DoubleEditorFactory *m_dblEdFac;
  // Presenter
  IIqtPresenter *m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt