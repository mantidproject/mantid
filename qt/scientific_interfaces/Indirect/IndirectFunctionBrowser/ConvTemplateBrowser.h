// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECT_CONVTEMPLATEBROWSER_H_
#define INDIRECT_CONVTEMPLATEBROWSER_H_

#include "DllConfig.h"
#include "FunctionTemplateBrowser.h"
#include "ConvTemplatePresenter.h"

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
class MANTIDQT_INDIRECT_DLL ConvTemplateBrowser : public FunctionTemplateBrowser {
  Q_OBJECT
public:
  ConvTemplateBrowser(QWidget *parent = nullptr);
  void addExponentialOne();
  void removeExponentialOne();
  void addExponentialTwo();
  void removeExponentialTwo();
  void addStretchExponential();
  void removeStretchExponential();
  void addFlatBackground();
  void removeBackground();

  void setExp1Height(double, double);
  void setExp1Lifetime(double, double);
  void setExp2Height(double, double);
  void setExp2Lifetime(double, double);
  void setStretchHeight(double, double);
  void setStretchLifetime(double, double);
  void setStretchStretching(double, double);
  void setA0(double, double);

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
  void updateParameterDescriptions(const QMap<int, std::string> &parameterNames) override;
  void setErrorsEnabled(bool enabled) override;
  void clear() override;

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
  QList<QtProperty *>
  createFunctionParameterProperties(const QString &funStr) const;

  QStringList m_fitTypeNames;
  QtProperty *m_fitType;
  // Map fit type to a list of function parameters (QtProperties for those parameters)
  QMap<int, QList<QtProperty*>> m_peakParameters;
  QtProperty *m_deltaFunctionOn;
  QList<QtProperty *> m_deltaFunction;
  QtProperty *m_backgroundType;
  QList<QtProperty *> m_background;

  QMap<QtProperty*, int> m_parameterMap;
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
