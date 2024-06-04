// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DataManipulation.h"
#include "ElwinPresenter.h"
#include "IElwinView.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IAddWorkspaceDialog.h"
#include "QENSFitting/FitDataModel.h"
#include "ui_ElwinTab.h"

namespace MantidQt {
namespace CustomInterfaces {
using namespace Inelastic;
using namespace Mantid::API;

class MANTIDQT_INELASTIC_DLL ElwinView : public QWidget, public IElwinView {
  Q_OBJECT

public:
  ElwinView(QWidget *parent = nullptr);
  ~ElwinView();

  void subscribePresenter(IElwinPresenter *presenter) override;
  void setup() override;

  IOutputPlotOptionsView *getPlotOptions() const override;

  void setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) override;
  void setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                           const std::vector<WorkspaceIndex>::const_iterator &to) override;

  void plotInput(MatrixWorkspace_sptr inputWS, int spectrum) override;
  void newInputDataFromDialog(std::vector<std::string> const &names) override;
  void clearPreviewFile() override;
  void setRunIsRunning(const bool running) override;
  void setSaveResultEnabled(const bool enabled) override;
  int getPreviewSpec() const override;
  std::string getPreviewWorkspaceName(int index) const override;
  std::string getPreviewFilename(int index) const override;
  std::string getCurrentPreview() const override;

  // controls for dataTable
  void clearDataTable() override;
  void addTableEntry(int row, std::string const &name, std::string const &wsIndexes) override;
  QModelIndexList getSelectedData() override;
  void selectAllRows() override;

  // boolean flags for LoadHistory/GroupInput Checkboxes
  bool isGroupInput() const override;
  bool isRowCollapsed() const override;
  bool isTableEmpty() const override;

  // getters/setters for m_properties
  bool getNormalise() override;
  bool getBackgroundSubtraction() override;
  std::string getLogName() override;
  std::string getLogValue() override;
  void setIntegrationStart(double value) override;
  void setIntegrationEnd(double value) override;
  void setBackgroundStart(double value) override;
  void setBackgroundEnd(double value) override;

  double getIntegrationStart() override;
  double getIntegrationEnd() override;
  double getBackgroundStart() override;
  double getBackgroundEnd() override;

  void showMessageBox(std::string const &message) const override;

private slots:
  void notifyMinChanged(double val);
  void notifyMaxChanged(double val);
  void notifyDoubleValueChanged(QtProperty *, double);
  void notifyCheckboxValueChanged(QtProperty *, bool);
  void notifyPlotPreviewClicked();
  void notifyRunClicked();
  void notifySaveClicked();
  void notifySelectedSpectrumChanged(int);
  void notifyPreviewIndexChanged(int);
  void notifyRowModeChanged();
  void notifyAddWorkspaceDialog();
  void notifyAddData(MantidWidgets::IAddWorkspaceDialog *dialog);
  void notifyRemoveDataClicked();
  void notifySelectAllClicked();

private:
  void setHorizontalHeaders();
  void setDefaultSampleLog(const Mantid::API::MatrixWorkspace_const_sptr &ws);

  void disconnectSignals();
  void connectSignals();

  /// Function to set the range selector on the mini plot
  void setRangeSelector(MantidWidgets::RangeSelector *rs, QtProperty *lower, QtProperty *upper,
                        const QPair<double, double> &range,
                        const boost::optional<QPair<double, double>> &bounds = boost::none);
  /// Sets the min of the range selector if it is less than the max
  void setRangeSelectorMin(QtProperty *minProperty, QtProperty *maxProperty,
                           MantidWidgets::RangeSelector *rangeSelector, double newValue);
  /// Sets the max of the range selector if it is more than the min
  void setRangeSelectorMax(QtProperty *minProperty, QtProperty *maxProperty,
                           MantidWidgets::RangeSelector *rangeSelector, double newValue);
  void showAddWorkspaceDialog();
  void setPreviewToDefault();
  void setButtonsEnabled(const bool enabled);
  void setRunEnabled(const bool enabled);
  void setCell(std::unique_ptr<QTableWidgetItem> cell, int row, int column);
  void addData(MantidWidgets::IAddWorkspaceDialog const *dialog);

  IElwinPresenter *m_presenter;
  QtTreePropertyBrowser *m_elwTree;

  Ui::ElwinTab m_uiForm;
  QtDoublePropertyManager *m_dblManager;
  QtBoolPropertyManager *m_blnManager;
  QtGroupPropertyManager *m_grpManager;
  DoubleEditorFactory *m_dblEdFac;
  QtCheckBoxFactory *m_blnEdFac;
  QMap<QString, QtProperty *> m_properties;
};
} // namespace CustomInterfaces
} // namespace MantidQt
