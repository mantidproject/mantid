// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECT_IQTTEMPLATEBROWSER_H_
#define INDIRECT_IQTTEMPLATEBROWSER_H_

#include "DllConfig.h"
#include "FunctionTemplateBrowser.h"
#include "IqtTemplatePresenter.h"

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
class MANTIDQT_INDIRECT_DLL IqtTemplateBrowser
    : public FunctionTemplateBrowser {
  Q_OBJECT
public:
  IqtTemplateBrowser(QWidget *parent = nullptr);
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
  double getParameterPropertyValue(QtProperty *prop) const;
  void setParameterPropertyValue(QtProperty *prop, double value, double error);
  void setGlobalParametersQuiet(const QStringList &globals);
  void setTieIntensitiesQuiet(bool on);
  void updateState();

  QtProperty *m_numberOfExponentials;
  QtProperty *m_exp1Height = nullptr;
  QtProperty *m_exp1Lifetime = nullptr;
  QtProperty *m_exp2Height = nullptr;
  QtProperty *m_exp2Lifetime = nullptr;
  QtProperty *m_stretchExponential;
  QtProperty *m_stretchExpHeight = nullptr;
  QtProperty *m_stretchExpLifetime = nullptr;
  QtProperty *m_stretchExpStretching = nullptr;
  QtProperty *m_background;
  QtProperty *m_A0 = nullptr;
  QtProperty *m_tieIntensities = nullptr;
  QMap<QtProperty *, int> m_parameterMap;
  QMap<QtProperty *, QString> m_actualParameterNames;
  QMap<QtProperty *, std::string> m_parameterDescriptions;

private:
  IqtTemplatePresenter m_presenter;
  bool m_emitParameterValueChange = true;
  bool m_emitIntChange = true;
  bool m_emitBoolChange = true;
  bool m_emitEnumChange = true;
  friend class IqtTemplatePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*INDIRECT_IQTTEMPLATEBROWSER_H_*/
