// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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

class MANTIDQT_INDIRECT_DLL InelasticDataManipulationIqtTabView : public QWidget {
  Q_OBJECT

public:
  InelasticDataManipulationIqtTabView(QWidget *parent = nullptr);
  ~InelasticDataManipulationIqtTabView();
  IndirectPlotOptionsView *getPlotOptions();
  void plotInput(MatrixWorkspace_sptr inputWS, int spectrum);
  void setPreviewSpectrumMaximum(int value);
  void updateDisplayedBinParameters();
  void setRangeSelectorDefault(const MatrixWorkspace_sptr inputWorkspace, const QPair<double, double> &range);
  bool validate();
  void setSampleFBSuffixes(QStringList const suffix);
  void setSampleWSSuffixes(QStringList const suffix);
  void setResolutionFBSuffixes(QStringList const suffix);
  void setResolutionWSSuffixes(QStringList const suffix);
  void setRunEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setRunText(bool running);
  void setWatchADS(bool watch);

  // getters for properties
  std::string getSampleName();
  std::string getResolutionName();
  std::string getIterations();
  double getELow();
  double getEHigh();
  double getSampleBinning();
  bool getCalculateErrors();

signals:
  void sampDataReady(const QString &);
  void PreviewSpectrumChanged(int);
  void runClicked();
  void saveClicked();
  void plotCurrentPreview();
  void showMessageBox(const QString &message);

private:
  void setup();
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
  /// Double editor facotry for the properties browser
  DoubleEditorFactory *m_dblEdFac;

private slots:
  void rangeChanged(double min, double max);
  void updateRangeSelector(QtProperty *prop, double val);
  void updateEnergyRange(int state);
  void errorsClicked();
};

} // namespace CustomInterfaces
} // namespace MantidQt