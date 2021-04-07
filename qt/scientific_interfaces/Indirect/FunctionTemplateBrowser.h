// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "ParameterEstimation.h"

#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>

class QtBoolPropertyManager;
class QtIntPropertyManager;
class QtDoublePropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtGroupPropertyManager;
class ParameterPropertyManager;
class QtTreePropertyBrowser;
class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace Mantid::API;
using namespace MantidWidgets;

/**
 * Class FunctionTemplateBrowser implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INDIRECT_DLL FunctionTemplateBrowser : public QWidget {
  Q_OBJECT
public:
  FunctionTemplateBrowser(QWidget *parent);
  virtual ~FunctionTemplateBrowser();
  void init();

  virtual void setFunction(const QString &funStr) = 0;
  virtual IFunction_sptr getGlobalFunction() const = 0;
  virtual IFunction_sptr getFunction() const = 0;
  virtual void setNumberOfDatasets(int) = 0;
  virtual int getNumberOfDatasets() const = 0;
  virtual void setDatasets(const QList<MantidWidgets::FunctionModelDataset> &datasets) = 0;
  virtual QStringList getGlobalParameters() const = 0;
  virtual QStringList getLocalParameters() const = 0;
  virtual void setGlobalParameters(const QStringList &globals) = 0;
  virtual void updateMultiDatasetParameters(const IFunction &fun) = 0;
  virtual void updateMultiDatasetParameters(const ITableWorkspace &paramTable) = 0;
  virtual void updateParameters(const IFunction &fun) = 0;
  virtual void setCurrentDataset(int i) = 0;
  virtual int getCurrentDataset() = 0;
  virtual void updateParameterNames(const QMap<int, QString> &parameterNames) = 0;
  virtual void setErrorsEnabled(bool enabled) = 0;
  virtual void clear() = 0;
  virtual void updateParameterEstimationData(DataForParameterEstimationCollection &&data) = 0;
  virtual void estimateFunctionParameters() = 0;
  virtual void setBackgroundA0(double value) = 0;
  virtual void setResolution(std::string const &name, TableDatasetIndex const &index) = 0;
  virtual void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) = 0;
  virtual void setQValues(const std::vector<double> &qValues) = 0;

signals:
  void functionStructureChanged();
  void localParameterButtonClicked(const QString &paramName);
  void parameterValueChanged(const QString &paramName, double value);

protected slots:
  virtual void intChanged(QtProperty *) {}
  virtual void boolChanged(QtProperty *) {}
  virtual void enumChanged(QtProperty *) {}
  virtual void popupMenu(const QPoint &) = 0;
  virtual void globalChanged(QtProperty *, const QString &, bool) = 0;
  virtual void parameterChanged(QtProperty *) = 0;
  virtual void parameterButtonClicked(QtProperty *) = 0;

private:
  void createBrowser();
  virtual void createProperties() = 0;

protected:
  QtBoolPropertyManager *m_boolManager;
  QtIntPropertyManager *m_intManager;
  QtDoublePropertyManager *m_doubleManager;
  QtStringPropertyManager *m_stringManager;
  QtEnumPropertyManager *m_enumManager;
  QtGroupPropertyManager *m_groupManager;
  ParameterPropertyManager *m_parameterManager;

  /// Qt property browser which displays properties
  QtTreePropertyBrowser *m_browser;

  /// Precision of doubles in m_doubleManager
  const int m_decimals;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
