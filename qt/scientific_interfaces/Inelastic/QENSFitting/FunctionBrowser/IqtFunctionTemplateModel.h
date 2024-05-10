// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../ParameterEstimation.h"
#include "DllConfig.h"
#include "FitTypes.h"
#include "MultiFunctionTemplateModel.h"

#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include <map>
#include <optional>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

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

  int numberOfExponentials() const;
  bool hasExponential() const;
  bool hasFitType() const;
  bool hasFitType(IqtTypes::FitType fitType) const;
  void removeBackground();
  bool hasBackground() const;
  void tieIntensities(bool on);

  EstimationDataSelector getEstimationDataSelector() const override;

private:
  std::optional<std::string> getPrefix(ParamID name) const override;
  void applyParameterFunction(const std::function<void(ParamID)> &paramFun) const override;

  void clearData();
  void setModel() override;

  void tieIntensities();

  std::optional<std::string> getExp1Prefix() const;
  std::optional<std::string> getExp2Prefix() const;
  std::optional<std::string> getFitTypePrefix(IqtTypes::FitType fitType) const;
  std::optional<std::string> getBackgroundPrefix() const;

  std::string buildFunctionString(int const domainIndex) const override;
  std::string buildExpDecayFunctionString() const;
  std::string buildStretchExpFunctionString() const;
  std::string buildTeixeiraWaterIqtFunctionString(int const domainIndex) const;
  std::string buildBackgroundFunctionString() const;

  IqtTypes::ExponentialType m_exponentialType = IqtTypes::ExponentialType::None;
  IqtTypes::FitType m_fitType = IqtTypes::FitType::None;
  IqtTypes::BackgroundType m_backgroundType = IqtTypes::BackgroundType::None;
  IqtTypes::TieIntensitiesType m_tieIntensitiesType = IqtTypes::TieIntensitiesType::False;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
