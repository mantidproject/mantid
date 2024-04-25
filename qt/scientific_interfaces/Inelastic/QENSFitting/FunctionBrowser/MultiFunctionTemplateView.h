// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "FitTypes.h"
#include "FunctionTemplateView.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class MANTIDQT_INELASTIC_DLL MultiFunctionTemplateView : public FunctionTemplateView {
  Q_OBJECT
public:
  explicit MultiFunctionTemplateView(TemplateBrowserCustomizations customizations);
  void updateParameterNames(const std::map<int, std::string> &parameterNames) override;
  void setGlobalParametersQuiet(std::vector<std::string> const &globals) override;

  void setSubTypes(std::map<std::size_t, int> const &subTypes);
  void setSubType(std::size_t subTypeIndex, int typeIndex);
  void setParameterValueQuiet(ParamID id, double value, double error);

protected slots:
  void intChanged(QtProperty *) override;
  void boolChanged(QtProperty *) override;
  void enumChanged(QtProperty *) override;
  void parameterChanged(QtProperty *) override;

private:
  void createProperties() override;
  void createFunctionParameterProperties();
  void setProperty(std::size_t subTypeIndex, int value);
  std::optional<std::size_t> propertySubTypeIndex(QtProperty *prop);

  std::vector<std::unique_ptr<TemplateSubType>> m_templateSubTypes;
  std::vector<std::map<int, std::vector<ParamID>>> m_subTypeParamIDs;
  std::vector<std::vector<QtProperty *>> m_currentSubTypeParameters;
  std::vector<QtProperty *> m_subTypeProperties;

  std::map<QtProperty *, ParamID> m_parameterMap;
  std::map<ParamID, QtProperty *> m_parameterReverseMap;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
