// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "FunctionModel.h"

#include <memory>
#include <optional>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

class EXPORT_OPT_MANTIDQT_COMMON ConvolutionFunctionModel : public FunctionModel {
public:
  void setFunction(IFunction_sptr) override;
  void setModel(const std::string &background, const std::string &workspace, int workspaceIndex,
                const std::string &peaks, bool hasDeltaFunction);
  void setModel(const std::string &background, const std::vector<std::pair<std::string, size_t>> &resolutionWorkspaces,
                const std::string &lorentzianPeaks, const std::string &fitType, bool hasDeltaFunction,
                const std::vector<double> &qValues, const bool isQDependent, bool hasTempCorrection, double tempValue);
  std::optional<std::string> backgroundPrefix() const { return m_backgroundPrefix; }
  std::optional<std::string> convolutionPrefix() const { return m_convolutionPrefix; }
  std::optional<std::string> deltaFunctionPrefix() const { return m_deltaFunctionPrefix; }
  std::optional<std::string> tempFunctionPrefix() const { return m_tempFunctionPrefix; }
  std::optional<std::vector<std::string>> peakPrefixes() const { return m_peakPrefixes; }
  std::optional<std::string> fitTypePrefix() const { return m_fitTypePrefix; }

  const std::string &resolutionWorkspace() const { return m_resolutionWorkspace; }
  int resolutionWorkspaceIndex() const { return m_resolutionWorkspaceIndex; }

private:
  void findComponentPrefixes();
  void iterateThroughFunction(IFunction *func, std::string const &prefix);
  void setPrefix(IFunction *func, std::string const &prefix);

  CompositeFunction_sptr createInnerFunction(const std::string &lorentzianPeaks, const std::string &fitType,
                                             bool hasDeltaFunction, bool isQDependent, double q, bool hasTempCorrection,
                                             double tempValue);
  CompositeFunction_sptr addTempCorrection(const CompositeFunction_sptr &peaksFunction, double tempValue);
  IFunction_sptr createTemperatureCorrection(double correction);
  CompositeFunction_sptr createConvolutionFunction(IFunction_sptr resolutionFunction,
                                                   const IFunction_sptr &innerFunction);
  IFunction_sptr createResolutionFunction(const std::string &workspaceName, size_t workspaceIndex);
  CompositeFunction_sptr addBackground(CompositeFunction_sptr domainFunction, const std::string &background);
  std::optional<std::string> m_backgroundPrefix;
  std::optional<std::string> m_convolutionPrefix;
  std::optional<std::string> m_deltaFunctionPrefix;
  std::optional<std::string> m_tempFunctionPrefix;
  std::optional<std::string> m_fitTypePrefix;
  std::optional<std::vector<std::string>> m_peakPrefixes;
  std::string m_resolutionWorkspace;
  int m_resolutionWorkspaceIndex;
};

} // namespace MantidWidgets
} // namespace MantidQt
