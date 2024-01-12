// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IMomentsView.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "ui_InelasticDataManipulationMomentsTab.h"

namespace MantidQt {
namespace CustomInterfaces {

class IMomentsPresenter;

class MANTIDQT_INELASTIC_DLL InelasticDataManipulationMomentsTabView : public QWidget, public IMomentsView {
  Q_OBJECT

public:
  InelasticDataManipulationMomentsTabView(QWidget *parent = nullptr);
  ~InelasticDataManipulationMomentsTabView();

  void subscribePresenter(IMomentsPresenter *presenter) override;

  void setupProperties() override;
  OutputPlotOptionsView *getPlotOptions() const override;
  std::string getDataName() const override;

  bool validate() override;

  void setFBSuffixes(QStringList const &suffix) override;
  void setWSSuffixes(QStringList const &suffix) override;

  /// Function to set the range limits of the plot
  void setPlotPropertyRange(const QPair<double, double> &bounds) override;
  /// Function to set the range selector on the mini plot
  void setRangeSelector(const QPair<double, double> &bounds) override;
  /// Sets the min of the range selector if it is less than the max
  void setRangeSelectorMin(double newValue) override;
  /// Sets the max of the range selector if it is more than the min
  void setRangeSelectorMax(double newValue) override;
  void plotNewData(std::string const &filename) override;
  void replot() override;
  void plotOutput(std::string const &outputWorkspace) override;

  void showMessageBox(const std::string &message) const override;

private slots:
  void notifyDataReady(QString const &);
  void notifyValueChanged(QtProperty *, double);

  void notifyScaleChanged(int);
  void notifyScaleValueChanged(double);
  void notifyRangeChanged(double, double);

  void notifyRunClicked();
  void notifySaveClicked();

private:
  MantidWidgets::RangeSelector *getRangeSelector();
  Ui::InelasticDataManipulationMomentsTab m_uiForm;

  /// Tree of the properties
  std::map<QString, QtTreePropertyBrowser *> m_propTrees;
  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;
  DoubleEditorFactory *m_dblEdFac;
  QtDoublePropertyManager *m_dblManager;
  IMomentsPresenter *m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt