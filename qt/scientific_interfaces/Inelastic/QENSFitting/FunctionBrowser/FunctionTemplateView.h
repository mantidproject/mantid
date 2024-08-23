// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../ParameterEstimation.h"
#include "DllConfig.h"
#include "TemplateSubType.h"

#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <vector>

#include <QList>
#include <QMap>
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
namespace MantidWidgets {
class EditLocalParameterDialog;
}
namespace CustomInterfaces {
namespace Inelastic {

using namespace Mantid::API;
using namespace MantidWidgets;

class ITemplatePresenter;

class MANTIDQT_INELASTIC_DLL FunctionTemplateView : public QWidget {
  Q_OBJECT
public:
  FunctionTemplateView();
  virtual ~FunctionTemplateView();
  void init();
  void subscribePresenter(ITemplatePresenter *presenter);

  void setEnumSilent(QtProperty *prop, int fitType);
  void setIntSilent(QtProperty *prop, int value);
  void setBoolSilent(QtProperty *prop, bool value);
  void setParameterSilent(QtProperty *prop, double value, double error);
  void setErrorsEnabled(bool enabled);

  void setFunction(std::string const &funStr);
  IFunction_sptr getGlobalFunction() const;
  IFunction_sptr getFunction() const;

  int getCurrentDataset();
  void setCurrentDataset(int i);
  void setNumberOfDatasets(int);
  int getNumberOfDatasets() const;
  void setDatasets(const QList<MantidWidgets::FunctionModelDataset> &datasets);

  std::vector<std::string> getGlobalParameters() const;
  std::vector<std::string> getLocalParameters() const;
  void setGlobalParameters(std::vector<std::string> const &globals);

  void updateMultiDatasetParameters(const IFunction &fun);
  void updateMultiDatasetParameters(const ITableWorkspace &table);
  void updateParameters(const IFunction &fun);
  virtual void updateParameterNames(const std::map<int, std::string> &parameterNames) = 0;
  virtual void setGlobalParametersQuiet(std::vector<std::string> const &globals) = 0;

  virtual void clear();
  virtual EstimationDataSelector getEstimationDataSelector() const;
  virtual void updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  virtual void estimateFunctionParameters();

  void emitFunctionStructureChanged() { emit functionStructureChanged(); }

  void openEditLocalParameterDialog(std::string const &parameterName, std::vector<std::string> const &datasetNames,
                                    std::vector<std::string> const &domainNames, QList<double> const &values,
                                    QList<bool> const &fixes, QStringList const &ties, QStringList const &constraints);

signals:
  void functionStructureChanged();

protected slots:
  virtual void intChanged(QtProperty *) {}
  virtual void boolChanged(QtProperty *) {}
  virtual void enumChanged(QtProperty *) {}
  virtual void popupMenu(const QPoint &) {}
  virtual void globalChanged(QtProperty *, const QString &, bool) {}
  virtual void parameterChanged(QtProperty *) = 0;
  void parameterButtonClicked(QtProperty *);
  void editLocalParameterFinished(int result, EditLocalParameterDialog *dialog);

private:
  void createBrowser();
  virtual void createProperties() = 0;

protected:
  bool m_emitParameterValueChange = true;
  bool m_emitBoolChange = true;
  bool m_emitEnumChange = true;
  bool m_emitIntChange = true;

  QtBoolPropertyManager *m_boolManager;
  QtIntPropertyManager *m_intManager;
  QtDoublePropertyManager *m_doubleManager;
  QtStringPropertyManager *m_stringManager;
  QtEnumPropertyManager *m_enumManager;
  QtGroupPropertyManager *m_groupManager;
  ParameterPropertyManager *m_parameterManager;

  QMap<QtProperty *, std::string> m_parameterNames;

  /// Qt property browser which displays properties
  QtTreePropertyBrowser *m_browser;

  /// The corresponding template presenter
  ITemplatePresenter *m_presenter;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
