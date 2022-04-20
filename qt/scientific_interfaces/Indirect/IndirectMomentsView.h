// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "ui_IndirectMoments.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IndirectMomentsView : public QWidget {
  Q_OBJECT

public:
  IndirectMomentsView(QWidget *perent = nullptr);
  ~IndirectMomentsView();

  void setupProperties();
  IndirectPlotOptionsView *getPlotOptions();
  std::string getDataName();
  bool validate();
  void plotNewData(QString const &filename);
  /// Function to set the range limits of the plot
  void setPlotPropertyRange(const QPair<double, double> &bounds);
  /// Function to set the range selector on the mini plot
  void setRangeSelector(const QPair<double, double> &bounds,
                        const boost::optional<QPair<double, double>> &range = boost::none);
  /// Sets the min of the range selector if it is less than the max
  void setRangeSelectorMin(double newValue);
  /// Sets the max of the range selector if it is more than the min
  void setRangeSelectorMax(double newValue);
  void replot();
  void plotOutput(QString outputWorkspace);
  void setFBSuffixes(QStringList const suffix);
  void setWSSuffixes(QStringList const suffix);

signals:
  void valueChanged(QtProperty *, double);
  void dataReady(QString const &);
  void scaleChanged(int);
  void scaleValueChanged(double);
  void runClicked();
  void saveClicked();
  void showMessageBox(const QString &message);

public slots:
  void rangeChanged(double min, double max);
  void updateRunButton(bool enabled, std::string const &enableOutputButtons, QString const &message,
                       QString const &tooltip);

private:
  MantidWidgets::RangeSelector *getRangeSelector();
  Ui::IndirectMoments m_uiForm;

  /// Tree of the properties
  std::map<QString, QtTreePropertyBrowser *> m_propTrees;
  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;
  DoubleEditorFactory *m_dblEdFac;
  QtDoublePropertyManager *m_dblManager;
};
} // namespace CustomInterfaces
} // namespace MantidQt