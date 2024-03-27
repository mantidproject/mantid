// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitTypes.h"
#include "ParameterEstimation.h"
#include "MultiFunctionTemplateModel.h"

#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include <map>
#include <optional>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace IqtTypes;
using namespace Mantid::API;

class MANTIDQT_INELASTIC_DLL IqtFunctionTemplateModel : public MultiFunctionTemplateModel {
public:
  IqtFunctionTemplateModel();

  void setFunction(IFunction_sptr fun) override;
  void removeFunction(std::string const &prefix) override;
  void addFunction(std::string const &prefix, std::string const &funStr) override;

  void setSubType(std::size_t, int) override;
  std::map<std::size_t, int> getSubTypes() const override;
  std::string setBackgroundA0(double value) override;
  void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;
  void setQValues(const std::vector<double> &qValues) override;

  int numberOfExponentials() const;
  bool hasExponential() const;
  bool hasStretchExponential() const;
  void removeBackground();
  bool hasBackground() const;
  void tieIntensities(bool on);

  EstimationDataSelector getEstimationDataSelector() const override;

private:
  std::optional<std::string> getPrefix(ParamID name) const override;
  void applyParameterFunction(const std::function<void(ParamID)> &paramFun) const override;

  void clearData();
  void setModel();

  void tieIntensities();

  std::optional<std::string> getExp1Prefix() const;
  std::optional<std::string> getExp2Prefix() const;
  std::optional<std::string> getStretchPrefix() const;
  std::optional<std::string> getBackgroundPrefix() const;

  std::string buildFunctionString() const;
  std::string buildExpDecayFunctionString() const;
  std::string buildStretchExpFunctionString() const;
  std::string buildBackgroundFunctionString() const;

  IqtTypes::ExponentialType m_exponentialType = IqtTypes::ExponentialType::None;
  IqtTypes::StretchExpType m_stretchExpType = IqtTypes::StretchExpType::None;
  IqtTypes::BackgroundType m_backgroundType = IqtTypes::BackgroundType::None;
  IqtTypes::TieIntensitiesType m_tieIntensitiesType = IqtTypes::TieIntensitiesType::False;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
