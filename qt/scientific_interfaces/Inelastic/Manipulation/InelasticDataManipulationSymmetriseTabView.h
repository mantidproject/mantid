// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "ui_InelasticDataManipulationSymmetriseTab.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL InelasticDataManipulationSymmetriseTabView : public QWidget {
  Q_OBJECT

public:
  InelasticDataManipulationSymmetriseTabView(QWidget *perent = nullptr);
  ~InelasticDataManipulationSymmetriseTabView();
  void setDefaults();
  IndirectPlotOptionsView *getPlotOptions();
  void setFBSuffixes(QStringList const suffix);
  void setWSSuffixes(QStringList const suffix);
  void plotNewData(QString const &workspaceName);
  void updateMiniPlots();
  bool validate();
  void setRawPlotWatchADS(bool watchADS);
  double getElow() const;
  double getEhigh() const;
  double getPreviewSpec();
  QString getInputName();
  void previewAlgDone();
  void enableSave(bool save);
  void enableRun(bool save);
  void updateRangeSelectors(QtProperty *prop, double value);
  void replotNewSpectrum(double value);
  bool verifyERange(QString const &workspaceName);

signals:
  void doubleValueChanged(QtProperty *, double);
  void enumValueChanged(QtProperty *, int);
  void dataReady(QString const &);
  void previewClicked();
  void runClicked();
  void saveClicked();
  void showMessageBox(const QString &message);

private slots:
  void xRangeLowChanged(double value);
  void xRangeHighChanged(double value);
  void reflectTypeChanged(QtProperty *, int value);

private:
  void resetEDefaults(bool isPositive, QPair<double, double> range);
  void updateHorizontalMarkers(QPair<double, double> yrange);

  Ui::InelasticDataManipulationSymmetriseTab m_uiForm;

  /// Tree of the properties
  std::map<QString, QtTreePropertyBrowser *> m_propTrees;
  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;
  QtDoublePropertyManager *m_dblManager;
  QtGroupPropertyManager *m_grpManager;
  QtEnumPropertyManager *m_enumManager;
};
} // namespace CustomInterfaces
} // namespace MantidQt