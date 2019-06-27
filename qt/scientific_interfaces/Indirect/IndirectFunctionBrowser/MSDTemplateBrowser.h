// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECT_MSDTEMPLATEBROWSER_H_
#define INDIRECT_MSDTEMPLATEBROWSER_H_

#include "DllConfig.h"
#include "FunctionTemplateBrowser.h"
#include "MSDTemplatePresenter.h"

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
class MANTIDQT_INDIRECT_DLL MSDTemplateBrowser
    : public FunctionTemplateBrowser {
  Q_OBJECT
public:
  MSDTemplateBrowser(QWidget *parent = nullptr);
  void addGaussian();
  void removeGaussian();
  void addPeters();
  void removePeters();
  void addYi();
  void removeYi();

  void setGaussianHeight(double, double);
  void setGaussianMsd(double, double);
  void setPetersHeight(double, double);
  void setPetersMsd(double, double);
  void setPetersBeta(double, double);
  void setYiHeight(double, double);
  void setYiMsd(double, double);
  void setYiSigma(double, double);

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
  void updateParameterDescriptions(
      const QMap<int, std::string> &parameterNames) override;
  void setErrorsEnabled(bool enabled) override;
  void clear() override;
  void updateParameterEstimationData(
      DataForParameterEstimationCollection &&data) override;

protected slots:
  void enumChanged(QtProperty *) override;
  void globalChanged(QtProperty *, const QString &, bool) override;
  void parameterChanged(QtProperty *) override;
  void parameterButtonClicked(QtProperty *) override;

private:
  void createProperties() override;
  void popupMenu(const QPoint &);
  void setParameterPropertyValue(QtProperty *prop, double value, double error);
  void setGlobalParametersQuiet(const QStringList &globals);

  QtProperty *m_fitType;
  QtProperty *m_gaussianHeight = nullptr;
  QtProperty *m_gaussianMsd = nullptr;
  QtProperty *m_petersHeight = nullptr;
  QtProperty *m_petersMsd = nullptr;
  QtProperty *m_petersBeta = nullptr;
  QtProperty *m_yiHeight = nullptr;
  QtProperty *m_yiMsd = nullptr;
  QtProperty *m_yiSigma = nullptr;
  QMap<QtProperty *, int> m_parameterMap;
  QMap<QtProperty *, QString> m_actualParameterNames;
  QMap<QtProperty *, std::string> m_parameterDescriptions;

private:
  MSDTemplatePresenter m_presenter;
  bool m_emitParameterValueChange = true;
  bool m_emitBoolChange = true;
  bool m_emitEnumChange = true;
  friend class MSDTemplatePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*INDIRECT_MSDTEMPLATEBROWSER_H_*/
