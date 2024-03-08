// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/IDAFunctionParameterEstimation.h"
#include "Analysis/ParameterEstimation.h"
#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "ParamID.h"

#include <QMap>
#include <optional>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace Mantid::API;
using namespace MantidWidgets;

class MANTIDQT_INELASTIC_DLL MultiFunctionTemplateModel : public IFunctionModel {
public:
  MultiFunctionTemplateModel(std::unique_ptr<FunctionModel> model,
                             std::unique_ptr<IDAFunctionParameterEstimation> estimators);
  IFunction_sptr getFullFunction() const override;
  IFunction_sptr getFitFunction() const override;
  bool hasFunction() const override;
  void setParameter(std::string const &parameterName, double value) override;
  void setParameterError(std::string const &parameterName, double value) override;
  double getParameter(std::string const &parameterName) const override;
  double getParameterError(std::string const &parameterName) const override;
  std::string getParameterDescription(std::string const &parameterName) const override;
  std::vector<std::string> getParameterNames() const override;
  IFunction_sptr getSingleFunction(int index) const override;
  IFunction_sptr getCurrentFunction() const override;
  void setNumberDomains(int) override;
  void setDatasets(const QList<FunctionModelDataset> &datasets) override;
  QStringList getDatasetNames() const override;
  QStringList getDatasetDomainNames() const override;
  int getNumberDomains() const override;
  void setCurrentDomainIndex(int i) override;
  int currentDomainIndex() const override;
  void changeTie(std::string const &parameterName, std::string const &tie) override;
  void addConstraint(std::string const &functionIndex, std::string const &constraint) override;
  void removeConstraint(std::string const &parameterName) override;
  std::vector<std::string> getGlobalParameters() const override;
  void setGlobalParameters(std::vector<std::string> const &globals) override;
  bool isGlobal(std::string const &parameterName) const override;
  void setGlobal(std::string const &parameterName, bool on) override;
  std::vector<std::string> getLocalParameters() const override;
  void updateMultiDatasetParameters(const IFunction &fun) override;
  void updateMultiDatasetParameters(const ITableWorkspace &paramTable) override;
  void updateParameters(const IFunction &fun) override;

  double getLocalParameterValue(std::string const &parameterName, int i) const override;
  bool isLocalParameterFixed(std::string const &parameterName, int i) const override;
  std::string getLocalParameterTie(std::string const &parameterName, int i) const override;
  std::string getLocalParameterConstraint(std::string const &parameterName, int i) const override;
  void setLocalParameterValue(std::string const &parameterName, int i, double value) override;
  void setLocalParameterValue(std::string const &parameterName, int i, double value, double error) override;
  void setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) override;
  void setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) override;
  void setLocalParameterConstraint(std::string const &parameterName, int i, std::string const &constraint) override;
  void setGlobalParameterValue(std::string const &parameterName, double value) override;

  virtual void setSubType(std::size_t subTypeIndex, int typeIndex) = 0;
  virtual std::map<std::size_t, int> getSubTypes() const = 0;
  virtual std::string setBackgroundA0(double value) = 0;
  virtual void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) = 0;
  virtual void setQValues(const std::vector<double> &qValues) = 0;

  virtual EstimationDataSelector getEstimationDataSelector() const = 0;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void estimateFunctionParameters();

  QMap<ParamID, double> getCurrentValues() const;
  QMap<ParamID, double> getCurrentErrors() const;
  QMap<int, std::string> getParameterNameMap() const;

protected:
  void setParameter(ParamID name, double value);
  std::optional<std::string> getParameterName(ParamID name) const;
  void setCurrentValues(const QMap<ParamID, double> &);
  std::vector<std::string> makeGlobalList() const;

  std::unique_ptr<FunctionModel> m_model;
  QList<ParamID> m_globals;

private:
  std::optional<double> getParameter(ParamID name) const;
  std::optional<double> getParameterError(ParamID name) const;
  std::optional<std::string> getParameterDescription(ParamID name) const;
  virtual std::optional<std::string> getPrefix(ParamID name) const = 0;
  virtual void applyParameterFunction(const std::function<void(ParamID)> &paramFun) const = 0;
  std::optional<ParamID> getParameterId(std::string const &parameterName);

  void addGlobal(std::string const &parameterName);
  void removeGlobal(std::string const &parameterName);

  DataForParameterEstimationCollection m_estimationData;
  std::unique_ptr<IDAFunctionParameterEstimation> m_parameterEstimation;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
