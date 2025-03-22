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
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <optional>

namespace MantidQt {
namespace MantidWidgets {
class ConvolutionFunctionModel;
}
namespace CustomInterfaces {
namespace Inelastic {

using namespace ConvTypes;
using namespace Mantid::API;

class MANTIDQT_INELASTIC_DLL ConvFunctionTemplateModel : public MultiFunctionTemplateModel {
public:
  ConvFunctionTemplateModel();

  ConvolutionFunctionModel *model() const;

  void setFunction(IFunction_sptr fun) override;
  void removeFunction(std::string const &prefix) override;
  void addFunction(std::string const &prefix, std::string const &funStr) override;

  void setSubType(std::size_t subTypeIndex, int typeIndex) override;
  std::map<std::size_t, int> getSubTypes() const override;
  std::string setBackgroundA0(double value) override;
  void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;

  bool hasDeltaFunction() const;
  bool hasTempCorrection() const;
  void removeBackground();
  bool hasBackground() const;

  EstimationDataSelector getEstimationDataSelector() const override;

private:
  std::optional<std::string> getPrefix(ParamID name) const override;
  void applyParameterFunction(const std::function<void(ParamID)> &paramFun) const override;

  void clearData();
  void setModel() override;

  void tiePeakCentres();

  std::optional<std::string> getLor1Prefix() const;
  std::optional<std::string> getLor2Prefix() const;
  std::optional<std::string> getFitTypePrefix() const;
  std::optional<std::string> getDeltaPrefix() const;
  std::optional<std::string> getBackgroundPrefix() const;

  std::string buildFunctionString(int const domainIndex) const override {
    (void)domainIndex;
    return "";
  }
  std::string buildLorentzianFunctionString() const;
  std::string buildTeixeiraFunctionString() const;
  std::string buildTeixeiraIqtFTFunctionString() const;
  std::string buildFickFunctionString() const;
  std::string buildChudleyElliotString() const;
  std::string buildHallRossString() const;
  std::string buildPeaksFunctionString() const;
  std::string buildLorentzianPeaksString() const;
  std::string buildFitTypeString() const;
  std::string buildBackgroundFunctionString() const;
  std::string buildStretchExpFTFunctionString() const;
  std::string buildDiffSphereFunctionString() const;
  std::string buildElasticDiffSphereFunctionString() const;
  std::string buildInelasticDiffSphereFunctionString() const;
  std::string buildDiffRotDiscreteCircleFunctionString() const;
  std::string buildInelasticDiffRotDiscreteCircleFunctionString() const;
  std::string buildElasticDiffRotDiscreteCircleFunctionString() const;
  std::string buildIsoRotDiffFunctionString() const;
  std::string buildElasticIsoRotDiffFunctionString() const;
  std::string buildInelasticIsoRotDiffFunctionString() const;

  int getNumberOfPeaks() const;
  void checkConvolution(const IFunction_sptr &fun);
  void checkSingleFunction(const IFunction_sptr &fun, bool &isLorentzianTypeSet, bool &isFiTypeSet);

  ConvTypes::FitType m_fitType = ConvTypes::FitType::None;
  ConvTypes::LorentzianType m_lorentzianType = ConvTypes::LorentzianType::None;
  ConvTypes::DeltaType m_deltaType = ConvTypes::DeltaType::None;
  ConvTypes::TempCorrectionType m_tempCorrectionType = ConvTypes::TempCorrectionType::None;
  ConvTypes::BackgroundType m_backgroundType = ConvTypes::BackgroundType::None;
  ConvTypes::TiePeakCentresType m_tiePeakCentresType = ConvTypes::TiePeakCentresType::False;

  BackgroundSubType m_backgroundSubtype;

  std::vector<std::pair<std::string, size_t>> m_fitResolutions;
  bool m_isQDependentFunction = false;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
