// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "FunctionModelDataset.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include <string>
#include <vector>

#include <QList>
#include <QPair>
#include <QString>

namespace MantidQt {
namespace MantidWidgets {

/** IFunctionBrowser: interface for FunctionBrowser

  Abstract base class to be implemented
*/
class EXPORT_OPT_MANTIDQT_COMMON IFunctionBrowser {
public:
  virtual ~IFunctionBrowser() = default;
  virtual std::string getFunctionString() = 0;
  virtual void updateParameters(const Mantid::API::IFunction &fun) = 0;
  virtual void clear() = 0;
  virtual void setErrorsEnabled(bool enabled) = 0;
  virtual void clearErrors() = 0;
  virtual void setFunction(std::string const &funStr) = 0;
  virtual void setNumberOfDatasets(int n) = 0;
  virtual void setDatasets(const std::vector<std::string> &datasetNames) = 0;
  virtual void setDatasets(const QList<FunctionModelDataset> &datasets) = 0;
  virtual Mantid::API::IFunction_sptr getGlobalFunction() = 0;
  virtual void updateMultiDatasetParameters(const Mantid::API::IFunction &fun) = 0;
  virtual void updateMultiDatasetParameters(const Mantid::API::ITableWorkspace &paramTable) = 0;
  virtual bool isLocalParameterFixed(std::string const &parameterName, int i) const = 0;
  virtual double getLocalParameterValue(std::string const &parameterName, int i) const = 0;
  virtual std::string getLocalParameterTie(std::string const &parameterName, int i) const = 0;
  virtual int getNumberOfDatasets() const = 0;
  virtual std::vector<std::string> getDatasetNames() const = 0;
  virtual std::vector<std::string> getDatasetDomainNames() const = 0;
  virtual void setLocalParameterValue(std::string const &parameterName, int i, double value) = 0;
  virtual void setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) = 0;
  virtual void setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) = 0;
  virtual void setCurrentDataset(int i) = 0;
  virtual int getCurrentDataset() const = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
