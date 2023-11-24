// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ISqwView.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "ui_InelasticDataManipulationSqwTab.h"

namespace MantidQt {
namespace CustomInterfaces {

class ISqwPresenter;

class MANTIDQT_INELASTIC_DLL InelasticDataManipulationSqwTabView : public QWidget, public ISqwView {
  Q_OBJECT

public:
  InelasticDataManipulationSqwTabView(QWidget *perent = nullptr);
  ~InelasticDataManipulationSqwTabView();

  void subscribePresenter(ISqwPresenter *presenter) override;

  IndirectPlotOptionsView *getPlotOptions() override;
  void setFBSuffixes(QStringList suffix) override;
  void setWSSuffixes(QStringList suffix) override;
  std::tuple<double, double> getQRangeFromPlot() override;
  std::tuple<double, double> getERangeFromPlot() override;
  std::string getDataName() override;
  void plotRqwContour(Mantid::API::MatrixWorkspace_sptr rqwWorkspace) override;
  void setDefaultQAndEnergy() override;
  void setSaveEnabled(bool enabled) override;
  bool validate() override;
  void showMessageBox(const std::string &message) const override;
  void updateRunButton(bool enabled, std::string const &enableOutputButtons, std::string const &message,
                       std::string const &tooltip) override;

signals:
  void valueChanged(QtProperty *, double);

private slots:
  void notifyDataReady(QString const &dataName);
  void notifyQLowChanged(double value);
  void notifyQWidthChanged(double value);
  void notifyQHighChanged(double value);
  void notifyELowChanged(double value);
  void notifyEWidthChanged(double value);
  void notifyEHighChanged(double value);
  void notifyRebinEChanged(int value);
  void notifyRunClicked();
  void notifySaveClicked();

private:
  void setQRange(std::tuple<double, double> const &axisRange);
  void setEnergyRange(std::tuple<double, double> const &axisRange);

  void setRunEnabled(bool enabled);
  Ui::InelasticDataManipulationSqwTab m_uiForm;

  /// Tree of the properties
  std::map<QString, QtTreePropertyBrowser *> m_propTrees;
  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;
  ISqwPresenter *m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt