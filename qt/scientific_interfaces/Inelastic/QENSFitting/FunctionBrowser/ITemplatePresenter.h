// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../ParameterEstimation.h"
#include "DllConfig.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"

#include <map>
#include <string>
#include <vector>

#include <QList>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class FunctionTemplateView;

class MANTIDQT_INELASTIC_DLL ITemplatePresenter {
public:
  virtual ~ITemplatePresenter() = default;

  virtual FunctionTemplateView *browser() = 0;

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
  virtual void updateMultiDatasetParameters(const Mantid::API::ITableWorkspace &table) = 0;
  virtual void updateParameters(const Mantid::API::IFunction &fun) = 0;

  virtual void setCurrentDataset(int i) = 0;
  virtual void setDatasets(const QList<MantidQt::MantidWidgets::FunctionModelDataset> &datasets) = 0;

  virtual EstimationDataSelector getEstimationDataSelector() const = 0;
  virtual void updateParameterEstimationData(DataForParameterEstimationCollection &&data) = 0;
  virtual void estimateFunctionParameters() = 0;

  virtual void setErrorsEnabled(bool enabled) = 0;

  virtual void setNumberOfExponentials(int nExponentials) = 0;
  virtual void setStretchExponential(bool on) = 0;
  virtual void setBackground(std::string const &name) = 0;
  virtual void tieIntensities(bool on) = 0;
  virtual bool canTieIntensities() const = 0;

  virtual void setSubType(std::size_t subTypeIndex, int typeIndex) = 0;
  virtual void setDeltaFunction(bool on) = 0;
  virtual void setTempCorrection(bool on) = 0;
  virtual void setBackgroundA0(double value) = 0;
  virtual void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) = 0;
  virtual void setQValues(const std::vector<double> &qValues) = 0;

  virtual void handleEditLocalParameter(std::string const &parameterName) = 0;
  virtual void handleParameterValueChanged(std::string const &parameterName, double value) = 0;
  virtual void handleEditLocalParameterFinished(std::string const &parameterName, QList<double> const &values,
                                                QList<bool> const &fixes, QStringList const &ties,
                                                QStringList const &constraints) = 0;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt