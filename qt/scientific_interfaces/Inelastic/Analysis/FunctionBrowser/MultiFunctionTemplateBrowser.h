// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/FunctionBrowser/FunctionTemplateBrowser.h"
#include "DllConfig.h"
#include "FitTypes.h"
#include "MultiFunctionTemplatePresenter.h"

#include <QMap>
#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace ConvTypes;

class MANTIDQT_INELASTIC_DLL MultiFunctionTemplateBrowser : public FunctionTemplateBrowser {
  Q_OBJECT
public:
  explicit MultiFunctionTemplateBrowser(TemplateBrowserCustomizations customizations);
  void updateParameterNames(const QMap<int, std::string> &parameterNames) override;
  void setGlobalParametersQuiet(std::vector<std::string> const &globals) override;

  void setBackgroundA0(double value) override;
  void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;
  void setQValues(const std::vector<double> &qValues) override;
  void setEnum(size_t subTypeIndex, int fitType);
  void setBool(size_t subTypeIndex, int enumIndex);
  void setInt(size_t subTypeIndex, int val);

protected slots:
  void intChanged(QtProperty *) override;
  void boolChanged(QtProperty *) override;
  void enumChanged(QtProperty *) override;
  void parameterChanged(QtProperty *) override;

private:
  void createProperties() override;
  void createFunctionParameterProperties();
  void setSubType(size_t subTypeIndex, int typeIndex);
  void setParameterValueQuiet(ParamID id, double value, double error);
  std::optional<std::size_t> propertySubTypeIndex(QtProperty *prop);

  std::vector<std::unique_ptr<TemplateSubType>> m_templateSubTypes;
  // Map fit type to a list of function parameters (QtProperties for those
  // parameters)
  std::vector<QMap<int, QList<QtProperty *>>> m_subTypeParameters;
  std::vector<QList<QtProperty *>> m_currentSubTypeParameters;
  std::vector<QtProperty *> m_subTypeProperties;

  QMap<QtProperty *, ParamID> m_parameterMap;
  QMap<ParamID, QtProperty *> m_parameterReverseMap;

private:
  friend class MultiFunctionTemplatePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
