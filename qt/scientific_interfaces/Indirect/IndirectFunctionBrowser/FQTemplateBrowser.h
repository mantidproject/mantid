// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECT_FQTEMPLATEBROWSER_H_
#define INDIRECT_FQTEMPLATEBROWSER_H_

#include "DllConfig.h"
#include "FQTemplatePresenter.h"
#include "FunctionTemplateBrowser.h"
#include "IFQFitObserver.h"

#include <QMap>
#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Class FunctionTemplateBrowser implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INDIRECT_DLL FQTemplateBrowser : public FunctionTemplateBrowser,
                                                public IFQFitObserver {
  Q_OBJECT
public:
  explicit FQTemplateBrowser(QWidget *parent = nullptr);
  virtual ~FQTemplateBrowser() = default;

  void setFunction(const QString &funStr) override;
  IFunction_sptr getGlobalFunction() const override;
  IFunction_sptr getFunction() const override;
  void setNumberOfDatasets(int) override;
  int getNumberOfDatasets() const override;
  void setDatasetNames(const QStringList &names) override;
  QStringList getGlobalParameters() const override;
  QStringList getLocalParameters() const override;
  void setGlobalParameters(const QStringList &globals) override;
  void updateMultiDatasetParameters(const IFunction &fun) override;
  void updateMultiDatasetParameters(const ITableWorkspace &paramTable) override;
  void updateParameters(const IFunction &fun) override;
  void setCurrentDataset(int i) override;
  void updateParameterNames(const QMap<int, QString> &parameterNames) override;
  void
  updateParameterDescriptions(const QMap<int, std::string> &parameterNames);
  void setErrorsEnabled(bool enabled) override;
  void clear() override;
  void updateParameterEstimationData(
      DataForParameterEstimationCollection &&data) override;
  void setBackgroundA0(double) override;
  void setResolution(std::string const &, TableDatasetIndex const &) override;
  void setResolution(const std::vector<std::pair<std::string, int>> &) override;
  void setQValues(const std::vector<double> &) override;
  int getCurrentDataset() override;
  void updateDataType(DataType) override;
  void spectrumChanged(int) override;
  void addParameter(QString parameterName, QString parameterDescription);
  void setParameterValue(QString parameterName, double parameterValue,
                         double parameterError);
  void setDataType(QStringList allowedFunctionsList);
  void setEnumValue(int enumIndex);

signals:
  void dataTypeChanged(DataType dataType);

protected slots:
  void enumChanged(QtProperty *) override;
  void globalChanged(QtProperty *, const QString &, bool) override;
  void parameterChanged(QtProperty *) override;
  void parameterButtonClicked(QtProperty *) override;

private:
  void createProperties() override;
  void popupMenu(const QPoint &) override;
  void setParameterPropertyValue(QtProperty *prop, double value, double error);
  void setGlobalParametersQuiet(const QStringList &globals);

  QtProperty *m_fitType;
  QMap<QString, QtProperty *> m_parameterMap;
  QMap<QtProperty *, QString> m_parameterNames;

private:
  FQTemplatePresenter m_presenter;
  bool m_emitParameterValueChange = true;
  bool m_emitBoolChange = true;
  bool m_emitEnumChange = true;
  friend class FQTemplatePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*INDIRECT_FQTEMPLATEBROWSER_H_*/
