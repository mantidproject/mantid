// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IDAFunctionParameterEstimation.h"
#include "IFQFitObserver.h"
#include "ParameterEstimation.h"
#include "SingleFunctionTemplateModel.h"

#include <QMap>
#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace MantidWidgets {
class EditLocalParameterDialog;
}
namespace CustomInterfaces {
namespace IDA {

class SingleFunctionTemplateBrowser;

/**
 * Class FunctionTemplateBrowser implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INDIRECT_DLL SingleFunctionTemplatePresenter : public QObject {
  Q_OBJECT
public:
  explicit SingleFunctionTemplatePresenter(SingleFunctionTemplateBrowser *view,
                                           const std::map<std::string, std::string> &functionInitialisationStrings,
                                           std::unique_ptr<IDAFunctionParameterEstimation> parameterEstimation);
  void updateAvailableFunctions(const std::map<std::string, std::string> &functionInitialisationStrings);
  void setFitType(const QString &name);

  void init();

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
  void updateParameters(const IFunction &fun);
  void setCurrentDataset(int i);
  void setDatasets(const QList<FunctionModelDataset> &datasets);
  void setErrorsEnabled(bool enabled);
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void estimateFunctionParameters();

signals:
  void functionStructureChanged();

private slots:
  void editLocalParameter(const QString &parName);
  void editLocalParameterFinish(int result);
  void viewChangedParameterValue(const QString &parName, double value);

private:
  QStringList getDatasetNames() const;
  QStringList getDatasetDomainNames() const;
  double getLocalParameterValue(const QString &parName, int i) const;
  bool isLocalParameterFixed(const QString &parName, int i) const;
  QString getLocalParameterTie(const QString &parName, int i) const;
  QString getLocalParameterConstraint(const QString &parName, int i) const;
  void setLocalParameterValue(const QString &parName, int i, double value);
  void setLocalParameterFixed(const QString &parName, int i, bool fixed);
  void setLocalParameterTie(const QString &parName, int i, const QString &tie);
  void updateView();
  SingleFunctionTemplateBrowser *m_view;
  SingleFunctionTemplateModel m_model;
  EditLocalParameterDialog *m_editLocalParameterDialog;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
