// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "FunctionModelDataset.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

class EXPORT_OPT_MANTIDQT_COMMON IFunctionModel {
public:
  virtual ~IFunctionModel() = default;
  void setFunctionString(std::string const &funStr);
  std::string getFunctionString() const;
  std::string getFitFunctionString() const;
  void clear();
  int getNumberLocalFunctions() const;
  virtual void setFunction(IFunction_sptr fun) = 0;
  virtual IFunction_sptr getFullFunction() const = 0;
  virtual IFunction_sptr getFitFunction() const = 0;
  virtual bool hasFunction() const = 0;
  virtual void addFunction(std::string const &prefix, std::string const &funStr) = 0;
  virtual void removeFunction(std::string const &functionIndex) = 0;
  virtual void setParameter(std::string const &parameterName, double value) = 0;
  virtual void setParameterError(std::string const &parameterName, double value) = 0;
  virtual double getParameter(std::string const &parameterName) const = 0;
  virtual double getParameterError(std::string const &parameterName) const = 0;
  virtual std::string getParameterDescription(std::string const &parameterName) const = 0;
  virtual std::vector<std::string> getParameterNames() const = 0;
  virtual IFunction_sptr getSingleFunction(int index) const = 0;
  virtual IFunction_sptr getCurrentFunction() const = 0;
  virtual void setNumberDomains(int) = 0;
  virtual void setDatasets(const QList<FunctionModelDataset> &datasets) = 0;
  virtual std::vector<std::string> getDatasetNames() const = 0;
  virtual std::vector<std::string> getDatasetDomainNames() const = 0;
  virtual int getNumberDomains() const = 0;
  virtual int currentDomainIndex() const = 0;
  virtual void setCurrentDomainIndex(int) = 0;
  virtual void changeTie(std::string const &parameterName, std::string const &tie) = 0;
  virtual void addConstraint(std::string const &functionIndex, std::string const &constraint) = 0;
  virtual void removeConstraint(std::string const &parameterName) = 0;
  virtual std::vector<std::string> getGlobalParameters() const = 0;
  virtual void setGlobal(std::string const &parameterName, bool on) = 0;
  virtual void setGlobalParameters(const std::vector<std::string> &globals) = 0;
  virtual bool isGlobal(std::string const &parameterName) const = 0;
  virtual std::vector<std::string> getLocalParameters() const = 0;
  virtual void updateMultiDatasetParameters(const IFunction &fun) = 0;
  virtual void updateMultiDatasetParameters(const ITableWorkspace &paramTable) = 0;
  virtual void updateParameters(const IFunction &fun) = 0;
  virtual double getLocalParameterValue(std::string const &parameterName, int i) const = 0;
  virtual bool isLocalParameterFixed(std::string const &parameterName, int i) const = 0;
  virtual std::string getLocalParameterTie(std::string const &parameterName, int i) const = 0;
  virtual std::string getLocalParameterConstraint(std::string const &parameterName, int i) const = 0;
  virtual void setLocalParameterValue(std::string const &parameterName, int i, double value) = 0;
  virtual void setLocalParameterValue(std::string const &parameterName, int i, double value, double error) = 0;
  virtual void setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) = 0;
  virtual void setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) = 0;
  virtual void setLocalParameterConstraint(std::string const &parameterName, int i, std::string const &constraint) = 0;
  virtual void setGlobalParameterValue(std::string const &parameterName, double value) = 0;
  virtual std::string setBackgroundA0(double value) = 0;
  virtual void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) = 0;
  virtual void setQValues(const std::vector<double> &qValues) = 0;

protected:
  static void copyParametersAndErrors(const IFunction &funFrom, IFunction &funTo);
  void copyParametersAndErrorsToAllLocalFunctions(const IFunction &fun);
};

} // namespace MantidWidgets
} // namespace MantidQt
