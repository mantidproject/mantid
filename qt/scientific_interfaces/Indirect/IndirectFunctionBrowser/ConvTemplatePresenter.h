// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ConvFunctionModel.h"
#include "DllConfig.h"

#include <QMap>
#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace MantidWidgets {
class EditLocalParameterDialog;
}
namespace CustomInterfaces {
namespace IDA {

class ConvTemplateBrowser;

/**
 * Class FunctionTemplateBrowser implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INDIRECT_DLL ConvTemplatePresenter : public QObject {
  Q_OBJECT
public:
  explicit ConvTemplatePresenter(ConvTemplateBrowser *view);
  void setSubType(size_t subTypeIndex, int typeIndex);
  void setDeltaFunction(bool);
  void setTempCorrection(bool);
  void setNumberOfDatasets(int);
  int getNumberOfDatasets() const;
  int getCurrentDataset();
  void setFunction(const QString &funStr);
  IFunction_sptr getGlobalFunction() const;
  IFunction_sptr getFunction() const;
  QStringList getGlobalParameters() const;
  QStringList getLocalParameters() const;
  void setGlobalParameters(const QStringList &globals);
  void setGlobal(const QString &parName, bool on);
  void updateMultiDatasetParameters(const IFunction &fun);
  void updateMultiDatasetParameters(const ITableWorkspace &paramTable);
  void updateParameters(const IFunction &fun);
  void setCurrentDataset(int i);
  void setDatasets(const QList<FunctionModelDataset> &datasets);
  void setErrorsEnabled(bool enabled);
  void setResolution(std::string const &name, TableDatasetIndex const &index);
  void setResolution(
      const std::vector<std::pair<std::string, size_t>> &fitResolutions);
  void setBackgroundA0(double value);
  void setQValues(const std::vector<double> &qValues);

signals:
  void functionStructureChanged();

private slots:
  void editLocalParameter(const QString &parName);
  void editLocalParameterFinish(int result);
  void viewChangedParameterValue(const QString &parName, double value);

private:
  void updateViewParameters();
  QStringList getDatasetNames() const;
  QStringList getDatasetDomainNames() const;
  double getLocalParameterValue(const QString &parName, int i) const;
  bool isLocalParameterFixed(const QString &parName, int i) const;
  QString getLocalParameterTie(const QString &parName, int i) const;
  QString getLocalParameterConstraint(const QString &parName, int i) const;
  void setLocalParameterValue(const QString &parName, int i, double value);
  void setLocalParameterFixed(const QString &parName, int i, bool fixed);
  void setLocalParameterTie(const QString &parName, int i, const QString &tie);
  void updateViewParameterNames();
  ConvTemplateBrowser *m_view;
  ConvFunctionModel m_model;
  EditLocalParameterDialog *m_editLocalParameterDialog;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
