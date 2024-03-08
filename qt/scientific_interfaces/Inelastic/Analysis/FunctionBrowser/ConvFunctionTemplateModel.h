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
#include "FitTypes.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MultiFunctionTemplateModel.h"

#include <QMap>
#include <boost/optional.hpp>

namespace MantidQt {
namespace MantidWidgets {
class ConvolutionFunctionModel;
}
namespace CustomInterfaces {
namespace IDA {

using namespace ConvTypes;
using namespace Mantid::API;
using namespace MantidWidgets;

class MANTIDQT_INELASTIC_DLL ConvFunctionTemplateModel : public MultiFunctionTemplateModel {
public:
  ConvFunctionTemplateModel();

  ConvolutionFunctionModel *model() const;

  void setFunction(IFunction_sptr fun) override;
  void addFunction(std::string const &prefix, std::string const &funStr) override;
  void removeFunction(std::string const &prefix) override;

  void setSubType(std::size_t subTypeIndex, int typeIndex) override;
  std::map<std::size_t, int> getSubTypes() const override;
  std::string setBackgroundA0(double value) override;
  void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;
  void setQValues(const std::vector<double> &qValues) override;

  bool hasDeltaFunction() const;
  bool hasTempCorrection() const;
  void removeBackground();
  bool hasBackground() const;

  EstimationDataSelector getEstimationDataSelector() const override;

private:
  void clearData();
  void setModel();
  boost::optional<std::string> getLor1Prefix() const;
  boost::optional<std::string> getLor2Prefix() const;
  boost::optional<std::string> getFitTypePrefix() const;
  boost::optional<std::string> getDeltaPrefix() const;
  boost::optional<std::string> getBackgroundPrefix() const;
  boost::optional<std::string> getPrefix(ParamID name) const override;
  void applyParameterFunction(const std::function<void(ParamID)> &paramFun) const override;
  std::string buildLorentzianFunctionString() const;
  std::string buildTeixeiraFunctionString() const;
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

  FitType m_fitType = FitType::None;
  LorentzianType m_lorentzianType = LorentzianType::None;
  DeltaType m_deltaType = DeltaType::None;
  TempCorrectionType m_tempCorrectionType = TempCorrectionType::None;
  BackgroundType m_backgroundType = BackgroundType::None;
  BackgroundSubType m_backgroundSubtype;
  std::vector<std::pair<std::string, size_t>> m_fitResolutions;
  std::vector<double> m_qValues;
  bool m_isQDependentFunction = false;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
