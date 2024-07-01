// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ISymmetriseView.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "ui_SymmetriseTab.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL SymmetriseView : public QWidget, public ISymmetriseView {
  Q_OBJECT
public:
  SymmetriseView(QWidget *parent = nullptr);
  ~SymmetriseView();

  void subscribePresenter(ISymmetrisePresenter *presenter) override;

  void setDefaults() override;
  IRunView *getRunView() const override;
  IOutputPlotOptionsView *getPlotOptions() const override;
  MantidWidgets::DataSelector *getDataSelector() const override;

  void setFBSuffixes(QStringList const &suffix) override;
  void setWSSuffixes(QStringList const &suffix) override;

  double getElow() const override;
  double getEhigh() const override;
  double getPreviewSpec() override;
  std::string getDataName() const override;

  void plotNewData(std::string const &workspaceName) override;
  void updateMiniPlots() override;
  void replotNewSpectrum(double value) override;
  void updateRangeSelectors(std::string const &propName, double value) override;
  bool verifyERange(std::string const &workspaceName) override;
  void setRawPlotWatchADS(bool watchADS) override;

  void previewAlgDone() override;
  void enableSave(bool save) override;
  void showMessageBox(std::string const &message) const override;
  void resetEDefaults(bool isPositive) override;
  void resetEDefaults(bool isPositive, QPair<double, double> range) override;

private slots:
  void notifyDoubleValueChanged(QtProperty *, double);
  void notifyReflectTypeChanged(QtProperty *, int value);
  void notifyDataReady(QString const &);
  void notifyPreviewClicked();
  void notifySaveClicked();
  void notifyXrangeLowChanged(double value);
  void notifyXrangeHighChanged(double value);

private:
  void updateHorizontalMarkers(QPair<double, double> yrange);

  Ui::SymmetriseTab m_uiForm;

  /// Tree of the properties
  std::map<QString, QtTreePropertyBrowser *> m_propTrees;
  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;
  QtDoublePropertyManager *m_dblManager;
  QtGroupPropertyManager *m_grpManager;
  QtEnumPropertyManager *m_enumManager;

  ISymmetrisePresenter *m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt