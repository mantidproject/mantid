// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECT_CONVTEMPLATEBROWSER_H_
#define INDIRECT_CONVTEMPLATEBROWSER_H_

#include "ConvTypes.h"
#include "DllConfig.h"
#include "FunctionTemplateBrowser.h"
#include "ConvTemplatePresenter.h"

#include <QMap>
#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace ConvTypes;
/**
 * Class FunctionTemplateBrowser implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INDIRECT_DLL ConvTemplateBrowser : public FunctionTemplateBrowser {
  Q_OBJECT
public:
  ConvTemplateBrowser(QWidget *parent = nullptr);
  void setFunction(const QString &funStr) override;
  IFunction_sptr getGlobalFunction() const override;
  IFunction_sptr getFunction() const override;
  void setNumberOfDatasets(int) override;
  int getNumberOfDatasets() const override;
  void setDatasetNames(const QStringList &names) override;
  QStringList getGlobalParameters() const override;
  QStringList getLocalParameters() const override;
  void setGlobalParameters(const QStringList &globals) override;
  void updateMultiDatasetParameters(const IFunction & fun) override;
  void updateMultiDatasetParameters(const ITableWorkspace & paramTable) override;
  void updateParameters(const IFunction &fun) override;
  void setCurrentDataset(int i) override;
  void updateParameterNames(const QMap<int, QString> &parameterNames) override;
  void setErrorsEnabled(bool enabled) override;
  void clear() override;
  void updateParameterEstimationData(
      DataForParameterEstimationCollection &&data) override;
  void setBackgroundA0(double value) override;

 protected slots:
  void intChanged(QtProperty *) override;
  void boolChanged(QtProperty *) override;
  void enumChanged(QtProperty *) override;
  void globalChanged(QtProperty *, const QString &, bool) override;
  void parameterChanged(QtProperty *) override;
  void parameterButtonClicked(QtProperty *) override;

private:
  void createProperties() override;
  void popupMenu(const QPoint &);
  void setParameterPropertyValue(QtProperty *prop, double value, double error);
  void setGlobalParametersQuiet(const QStringList &globals);
  void createFunctionParameterProperties();
  void setSubType(size_t subTypeIndex, int typeIndex);

  std::vector<std::unique_ptr<TemplateSubType>> m_templateSubTypes;
  // Map fit type to a list of function parameters (QtProperties for those parameters)
  std::vector<QMap<int, QList<QtProperty *>>> m_subTypeParameters;
  std::vector<QList<QtProperty *>> m_currentSubTypeParameters;
  std::vector<QtProperty *> m_subTypeProperties;

  QtProperty *m_deltaFunctionOn;

  QMap<QtProperty *, ParamID> m_parameterMap;
  QMap<QtProperty*, QString> m_actualParameterNames;
  QMap<QtProperty*, std::string> m_parameterDescriptions;

private:
  ConvTemplatePresenter m_presenter;
  bool m_emitParameterValueChange = true;
  friend class ConvTemplatePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*INDIRECT_CONVTEMPLATEBROWSER_H_*/
