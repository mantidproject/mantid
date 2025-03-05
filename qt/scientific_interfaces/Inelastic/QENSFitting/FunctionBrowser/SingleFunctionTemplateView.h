// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "FunctionTemplateView.h"
#include "SingleFunctionTemplatePresenter.h"

#include <QMap>
#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class FunctionParameterEstimation;
/**
 * Class FunctionTemplateView implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INELASTIC_DLL SingleFunctionTemplateView : public FunctionTemplateView {
  Q_OBJECT
public:
  explicit SingleFunctionTemplateView(TemplateBrowserCustomizations customizations);
  virtual ~SingleFunctionTemplateView() override = default;

  void updateParameterNames(const std::map<int, std::string> &parameterNames) override;
  void setGlobalParametersQuiet(std::vector<std::string> const &globals) override;
  void clear() override;
  void addParameter(std::string const &parameterName, std::string const &parameterDescription);
  void setParameterValue(std::string const &parameterName, double parameterValue, double parameterError);
  void setParameterValueQuietly(std::string const &parameterName, double parameterValue, double parameterError);
  void setDataType(std::vector<std::string> const &allowedFunctionsList);
  void setEnumValue(int enumIndex);
  void updateAvailableFunctions(const std::map<std::string, std::string> &functionInitialisationStrings);

protected slots:
  void enumChanged(QtProperty *) override;
  void parameterChanged(QtProperty *) override;

private:
  void createProperties() override;

  QtProperty *m_fitType;
  QMap<std::string, QtProperty *> m_parameterMap;

private:
  friend class SingleFunctionTemplatePresenter;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
