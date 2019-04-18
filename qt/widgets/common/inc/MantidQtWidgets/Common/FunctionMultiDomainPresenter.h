// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRESENTER_H_
#define MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRESENTER_H__

#include "DllOption.h"

#include "MantidAPI/IFunction_fwd.h"
#include "MantidQtWidgets/Common/FunctionModel.h"

#include <boost/optional.hpp>
#include <QObject>
#include <memory>

namespace MantidQt {
namespace MantidWidgets {

class MultiDomainFunctionModel;
class IFunctionView;

using namespace Mantid::API;

class EXPORT_OPT_MANTIDQT_COMMON FunctionMultiDomainPresenter: public QObject {
  Q_OBJECT
public:
  FunctionMultiDomainPresenter(IFunctionView *view);
  void clear();
  void setFunction(IFunction_sptr fun);
  void setFunctionString(const QString &funStr);
  QString getFunctionString() const;
  IFunction_sptr getFunction() const;
  IFunction_sptr getFunctionByIndex(const QString &index);
  IFunction_sptr getFitFunction() const;
  QString getFitFunctionString() const;
  bool hasFunction() const;
  void setParameter(const QString &paramName, double value);
  void setParamError(const QString &paramName, double value);
  double getParameter(const QString &paramName);
  void updateParameters(const IFunction &fun);
  void clearErrors();
  boost::optional<QString> currentFunctionIndex() const;
  void setNumberOfDatasets(int);
  int getNumberOfDatasets() const;
  int getCurrentDataset() const;
  void setCurrentDataset(int);
  void addDatasets(int);
  void removeDatasets(QList<int> indices);

  void setColumnSizes(int s0, int s1, int s2);
  void setErrorsEnabled(bool enabled);
signals:
  void functionStructureChanged();
  void parameterChanged(const QString &funcIndex, const QString &paramName);
private slots:
  void viewChangedParameter(const QString &paramName);
  void viewPastedFunction(const QString &funStr);
private:
  IFunctionView *m_view;
  std::unique_ptr<MultiDomainFunctionModel> m_model;
public:
  IFunctionView *view() const {return m_view;}
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRESENTER_H_
