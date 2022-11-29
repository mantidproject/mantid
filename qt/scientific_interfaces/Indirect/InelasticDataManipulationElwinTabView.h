// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IAddWorkspaceDialog.h"
#include "IndirectFitDataModel.h"
#include "InelasticDataManipulation.h"
#include "InelasticDataManipulationElwinTabView.h"
#include "InelasticDataManipulationTab.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "ui_InelasticDataManipulationElwinTab.h"

namespace MantidQt {
namespace CustomInterfaces {
using namespace IDA;
using namespace Mantid::API;

class MANTIDQT_INDIRECT_DLL InelasticDataManipulationElwinTabView : public QWidget {
  Q_OBJECT

public:
  InelasticDataManipulationElwinTabView(QWidget *parent = nullptr);
  ~InelasticDataManipulationElwinTabView();
  QTableWidget *getDataTable() const;
  IndirectPlotOptionsView *getPlotOptions();

  void setDefaultResolution(const Mantid::API::MatrixWorkspace_const_sptr &ws, const QPair<double, double> &range);
  void setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum);
  void setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                           const std::vector<WorkspaceIndex>::const_iterator &to);
  bool isLoadHistory();
  bool isGroupInput();
  void setFBSuffixes(QStringList const suffix);
  void newPreviewFileSelected(const MatrixWorkspace_sptr &workspace);
  int getCurrentInputIndex();
  QString getPreviewWorkspaceName(int index);
  QString getPreviewFilename(int index);
  QStringList getInputFilenames();
  void plotInput(MatrixWorkspace_sptr inputWS, int spectrum);
  int getPreviewSpec();
  QString getCurrentPreview();
  void newInputFiles();
  void newInputFilesFromDialog(IAddWorkspaceDialog const *dialog);
  void clearPreviewFile();
  bool validate();
  bool validateFileSuffix();
  void setRunIsRunning(const bool &running);
  void setSaveResultEnabled(const bool &enabled);
  void clearInputFiles();

  // getters for m_properties
  double getIntegrationStart();
  double getIntegrationEnd();
  double getBackgroundStart();
  double getBackgroundEnd();
  bool getBackgroundSubtraction();
  bool getNormalise();
  std::string getLogName();
  std::string getLogValue();

signals:
  void removeDataClicked();
  void addDataClicked();
  void dataAdded();
  void dataRemoved();
  void dataChanged();
  void showMessageBox(const QString &message);
  void runClicked();
  void saveClicked();
  void plotPreviewClicked();
  void selectedSpectrumChanged(int);
  void previewIndexChanged(int);
  void filesFound();
  void valueChanged(QtProperty *, double);
  void valueChanged(QtProperty *, bool);

private:
  void setup();
  void setDefaultSampleLog(const Mantid::API::MatrixWorkspace_const_sptr &ws);

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
  void setPreviewToDefault();
  void setButtonsEnabled(const bool &enabled);
  void setRunEnabled(const bool &enabled);

  virtual std::unique_ptr<IAddWorkspaceDialog> getAddWorkspaceDialog(QWidget *parent) const;

  Ui::InelasticDataManipulationElwinTab m_uiForm;
  QtTreePropertyBrowser *m_elwTree;
  QtDoublePropertyManager *m_dblManager;
  QtBoolPropertyManager *m_blnManager;
  QtGroupPropertyManager *m_grpManager;
  DoubleEditorFactory *m_dblEdFac;
  QtCheckBoxFactory *m_blnEdFac;
  QMap<QString, QtProperty *> m_properties;

private slots:
  void twoRanges(QtProperty *prop, bool enabled);
  void minChanged(double val);
  void maxChanged(double val);
  void updateRS(QtProperty *prop, double val);
};

} // namespace CustomInterfaces
} // namespace MantidQt
