// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/ParameterEstimation.h"
#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <QMap>
#include <boost/optional.hpp>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IDAFunctionParameterEstimation;

using namespace Mantid::API;
using namespace MantidWidgets;

class MANTIDQT_INELASTIC_DLL SingleFunctionTemplateModel : public FunctionModel {
public:
  SingleFunctionTemplateModel();
  SingleFunctionTemplateModel(std::unique_ptr<IDAFunctionParameterEstimation> parameterEstimation);
  void setFunction(IFunction_sptr fun) override;
  void updateAvailableFunctions(const std::map<std::string, std::string> &functionInitialisationStrings);

  void setFitType(const std::string &name);
  std::string getFitType();
  std::vector<std::string> getFunctionList();
  int getEnumIndex();
  void setGlobal(std::string const &parameterName, bool isGlobal) override;

  EstimationDataSelector getEstimationDataSelector() const;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data);

  void estimateFunctionParameters();

private:
  std::string m_fitType;
  DataForParameterEstimationCollection m_estimationData;
  QMap<std::string, IFunction_sptr> m_fitTypeToFunctionStore;
  QMap<std::string, std::vector<std::string>> m_globalParameterStore;
  std::vector<std::string> m_fitTypeList;
  boost::optional<std::string> findFitTypeForFunctionName(const std::string &name) const;
  // Parameter estimation
  std::unique_ptr<IDAFunctionParameterEstimation> m_parameterEstimation;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
