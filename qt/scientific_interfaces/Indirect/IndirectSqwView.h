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
#include "ui_IndirectSqw.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IndirectSqwView : public QWidget {
  Q_OBJECT

public:
  IndirectSqwView(QWidget *perent = nullptr);
  ~IndirectSqwView();

  IndirectPlotOptionsView *getPlotOptions();
  void setFBSuffixes(QStringList suffix);
  void setWSSuffixes(QStringList suffix);
  std::tuple<double, double> getQRangeFromPlot();
  std::tuple<double, double> getERangeFromPlot();
  std::string getDataName();
  void plotRqwContour(Mantid::API::MatrixWorkspace_sptr rqwWorkspace);
  void setDefaultQAndEnergy();
  void setSaveEnabled(bool enabled);
  bool validate();

signals:
  void valueChanged(QtProperty *, double);
  void dataReady(QString const &);
  void qLowChanged(double);
  void qWidthChanged(double);
  void qHighChanged(double);
  void eLowChanged(double);
  void eWidthChanged(double);
  void eHighChanged(double);
  void rebinEChanged(int);
  void runClicked();
  void saveClicked();
  void showMessageBox(const QString &message);

public slots:
  void updateRunButton(bool enabled, std::string const &enableOutputButtons, QString const &message,
                       QString const &tooltip);

private:
  void setQRange(std::tuple<double, double> const &axisRange);
  void setEnergyRange(std::tuple<double, double> const &axisRange);

  void setRunEnabled(bool enabled);
  Ui::IndirectSqw m_uiForm;

  /// Tree of the properties
  std::map<QString, QtTreePropertyBrowser *> m_propTrees;
  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;
  DoubleEditorFactory *m_dblEdFac;
  QtDoublePropertyManager *m_dblManager;
};
} // namespace CustomInterfaces
} // namespace MantidQt