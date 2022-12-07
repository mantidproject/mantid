// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "ui_InelasticDataManipulationSymmetriseTab.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL InelasticDataManipulationSymmetriseTabView : public QWidget {
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
  double getEMin();
  double getEMax();
  double getPreviewSpec();
  QString getInputName();
  void previewAlgDone();
  void enableSave(bool save);
  void enableRun(bool save);
  void updateRangeSelectors(QtProperty *prop, double value);
  void replotNewSpectrum(double value);
  void verifyERange(QtProperty *prop, double value);

public slots:
  void updateRunButton(bool enabled = true, std::string const &enableOutputButtons = "unchanged",
                       QString const &message = "Run", QString const &tooltip = "");

signals:
  void valueChanged(QtProperty *, double);
  void dataReady(QString const &);
  void previewClicked();
  void runClicked();
  void saveClicked();
  void showMessageBox(const QString &message);

private slots:
  void xRangeMaxChanged(double value);
  void xRangeMinChanged(double value);

private:
  void setRunEnabled(bool enabled);
  void setSaveEnabled(bool enabled);
  Ui::InelasticDataManipulationSymmetriseTab m_uiForm;

  /// Tree of the properties
  std::map<QString, QtTreePropertyBrowser *> m_propTrees;
  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;
  QtDoublePropertyManager *m_dblManager;
  QtGroupPropertyManager *m_grpManager;
};
} // namespace CustomInterfaces
} // namespace MantidQt