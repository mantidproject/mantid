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
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"
#include "ui_SqwTab.h"

namespace MantidQt {
namespace CustomInterfaces {

class ISqwPresenter;

class MANTIDQT_INELASTIC_DLL SqwView : public QWidget, public ISqwView {
  Q_OBJECT

public:
  SqwView(QWidget *perent = nullptr);
  ~SqwView();

  void subscribePresenter(ISqwPresenter *presenter) override;

  IRunView *getRunView() const override;
  IOutputPlotOptionsView *getPlotOptions() const override;

  void setFBSuffixes(QStringList const &suffix) override;
  void setWSSuffixes(QStringList const &suffix) override;
  std::tuple<double, double> getQRangeFromPlot() const override;
  std::tuple<double, double> getERangeFromPlot() const override;
  std::string getDataName() const override;
  void plotRqwContour(Mantid::API::MatrixWorkspace_sptr rqwWorkspace) override;
  void setDefaultQAndEnergy() override;
  bool validate() override;
  void showMessageBox(std::string const &message) const override;
  void setEnableOutputOptions(bool const enable) override;

private slots:
  void notifyDataReady(QString const &dataName);
  void notifyQLowChanged(double value);
  void notifyQWidthChanged(double value);
  void notifyQHighChanged(double value);
  void notifyELowChanged(double value);
  void notifyEWidthChanged(double value);
  void notifyEHighChanged(double value);
  void notifyRebinEChanged(int value);
  void notifySaveClicked();

private:
  void setQRange(std::tuple<double, double> const &axisRange);
  void setEnergyRange(std::tuple<double, double> const &axisRange);

  Ui::SqwTab m_uiForm;

  /// Tree of the properties
  std::map<QString, QtTreePropertyBrowser *> m_propTrees;
  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;
  ISqwPresenter *m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt