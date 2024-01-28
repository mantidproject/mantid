// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/ParameterEstimation.h"
#include "DllConfig.h"
#include "MantidAPI/IFunction.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"

#include <map>
#include <string>
#include <vector>

#include <QList>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INELASTIC_DLL ITemplatePresenter {
public:
  virtual void init() = 0;
  virtual void updateAvailableFunctions(const std::map<std::string, std::string> &functionInitialisationStrings) = 0;

  virtual void setNumberOfDatasets(int) = 0;
  virtual int getNumberOfDatasets() const = 0;
  virtual int getCurrentDataset() = 0;

  virtual void setFitType(std::string const &name) = 0;

  virtual void setFunction(std::string const &funStr) = 0;
  virtual Mantid::API::IFunction_sptr getGlobalFunction() const = 0;
  virtual Mantid::API::IFunction_sptr getFunction() const = 0;

  virtual std::vector<std::string> getGlobalParameters() const = 0;
  virtual std::vector<std::string> getLocalParameters() const = 0;
  virtual void setGlobalParameters(std::vector<std::string> const &globals) = 0;
  virtual void setGlobal(std::string const &parameterName, bool on) = 0;

  virtual void updateMultiDatasetParameters(const Mantid::API::IFunction &fun) = 0;
  virtual void updateParameters(const Mantid::API::IFunction &fun) = 0;

  virtual void setCurrentDataset(int i) = 0;
  virtual void setDatasets(const QList<MantidQt::MantidWidgets::FunctionModelDataset> &datasets) = 0;

  virtual EstimationDataSelector getEstimationDataSelector() const = 0;
  virtual void updateParameterEstimationData(DataForParameterEstimationCollection &&data) = 0;
  virtual void estimateFunctionParameters() = 0;

  virtual void handleEditLocalParameter(std::string const &parameterName) = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt