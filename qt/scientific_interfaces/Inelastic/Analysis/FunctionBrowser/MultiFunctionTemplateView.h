// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/FunctionBrowser/FunctionTemplateView.h"
#include "DllConfig.h"
#include "FitTypes.h"

#include <optional>
#include <string>
#include <vector>

#include <QList>
#include <QMap>

class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INELASTIC_DLL MultiFunctionTemplateView : public FunctionTemplateView {
  Q_OBJECT
public:
  explicit MultiFunctionTemplateView(TemplateBrowserCustomizations customizations);
  void updateParameterNames(const QMap<int, std::string> &parameterNames) override;
  void setGlobalParametersQuiet(std::vector<std::string> const &globals) override;

  void setProperty(std::size_t subTypeIndex, int value);
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
  std::optional<std::size_t> propertySubTypeIndex(QtProperty *prop);

  std::vector<std::unique_ptr<TemplateSubType>> m_templateSubTypes;
  // Map fit type to a list of function parameters (QtProperties for those parameters)
  std::vector<QMap<int, QList<QtProperty *>>> m_subTypeParameters;
  std::vector<QList<QtProperty *>> m_currentSubTypeParameters;
  std::vector<QtProperty *> m_subTypeProperties;

  QMap<QtProperty *, ParamID> m_parameterMap;
  QMap<ParamID, QtProperty *> m_parameterReverseMap;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
