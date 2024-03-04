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
#include "MantidQtWidgets/Common/ConvolutionFunctionModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <QMap>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace ConvTypes;
using namespace Mantid::API;
using namespace MantidWidgets;

class MANTIDQT_INELASTIC_DLL MultiFunctionTemplateModel : public IFunctionModel {
public:
  MultiFunctionTemplateModel();
  void setFunction(IFunction_sptr fun) override;
  IFunction_sptr getFullFunction() const override;
  IFunction_sptr getFitFunction() const override;
  bool hasFunction() const override;
  void addFunction(std::string const &prefix, std::string const &funStr) override;
  void removeFunction(std::string const &prefix) override;
  void setParameter(std::string const &parameterName, double value) override;
  void setParameterError(std::string const &parameterName, double value) override;
  FitType getFitType() const;
  LorentzianType getLorentzianType() const;
  DeltaType getDeltaType() const;
  TempCorrectionType getTempCorrectionType() const;
  BackgroundType getBackgroundType() const;
  double getParameter(std::string const &parameterName) const override;
  double getParameterError(std::string const &parameterName) const override;
  std::string getParameterDescription(std::string const &parameterName) const override;
  std::vector<std::string> getParameterNames() const override;
  IFunction_sptr getSingleFunction(int index) const override;
  IFunction_sptr getCurrentFunction() const override;
  void setNumberDomains(int) override;
  void setDatasets(const QList<FunctionModelDataset> &datasets) override;
  QStringList getDatasetNames() const override;
  QStringList getDatasetDomainNames() const override;
  int getNumberDomains() const override;
  void setCurrentDomainIndex(int i) override;
  int currentDomainIndex() const override;
  void changeTie(std::string const &parameterName, std::string const &tie) override;
  void addConstraint(std::string const &functionIndex, std::string const &constraint) override;
  void removeConstraint(std::string const &parameterName) override;
  std::vector<std::string> getGlobalParameters() const override;
  void setGlobalParameters(std::vector<std::string> const &globals) override;
  bool isGlobal(std::string const &parameterName) const override;
  void setGlobal(std::string const &parameterName, bool on) override;
  std::vector<std::string> getLocalParameters() const override;
  void updateMultiDatasetParameters(const IFunction &fun) override;
  void updateMultiDatasetParameters(const ITableWorkspace &paramTable) override;
  void updateParameters(const IFunction &fun) override;

  double getLocalParameterValue(std::string const &parameterName, int i) const override;
  bool isLocalParameterFixed(std::string const &parameterName, int i) const override;
  std::string getLocalParameterTie(std::string const &parameterName, int i) const override;
  std::string getLocalParameterConstraint(std::string const &parameterName, int i) const override;
  void setLocalParameterValue(std::string const &parameterName, int i, double value) override;
  void setLocalParameterValue(std::string const &parameterName, int i, double value, double error) override;
  void setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) override;
  void setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) override;
  void setLocalParameterConstraint(std::string const &parameterName, int i, std::string const &constraint) override;
  void setGlobalParameterValue(std::string const &parameterName, double value) override;
  std::string setBackgroundA0(double value) override;
  void setSubType(std::size_t subTypeIndex, int typeIndex);
  void setFitType(FitType fitType);
  void setLorentzianType(LorentzianType lorentzianType);
  void setDeltaType(DeltaType deltaType);
  void setTempCorrectionType(TempCorrectionType tempCorrectionType);
  bool hasDeltaFunction() const;
  bool hasTempCorrection() const;
  void setBackground(BackgroundType bgType);
  void removeBackground();
  bool hasBackground() const;
  EstimationDataSelector getEstimationDataSelector() const;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void estimateFunctionParameters();
  void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions);
  void setQValues(const std::vector<double> &qValues);

  QMap<ParamID, double> getCurrentValues() const;
  QMap<ParamID, double> getCurrentErrors() const;
  QMap<int, std::string> getParameterNameMap() const;

private:
  void clearData();
  void setModel();
  boost::optional<std::string> getLor1Prefix() const;
  boost::optional<std::string> getLor2Prefix() const;
  boost::optional<std::string> getFitTypePrefix() const;
  boost::optional<std::string> getDeltaPrefix() const;
  boost::optional<std::string> getBackgroundPrefix() const;
  void setParameter(ParamID name, double value);
  boost::optional<double> getParameter(ParamID name) const;
  boost::optional<double> getParameterError(ParamID name) const;
  boost::optional<std::string> getParameterName(ParamID name) const;
  boost::optional<std::string> getParameterDescription(ParamID name) const;
  boost::optional<std::string> getPrefix(ParamID name) const;
  void setCurrentValues(const QMap<ParamID, double> &);
  void applyParameterFunction(const std::function<void(ParamID)> &paramFun) const;
  boost::optional<ParamID> getParameterId(std::string const &parameterName);
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
  void addGlobal(std::string const &parameterName);
  void removeGlobal(std::string const &parameterName);
  std::vector<std::string> makeGlobalList() const;
  int getNumberOfPeaks() const;
  void checkConvolution(const IFunction_sptr &fun);
  void checkSingleFunction(const IFunction_sptr &fun, bool &isLorentzianTypeSet, bool &isFiTypeSet);

  ConvolutionFunctionModel m_model;
  FitType m_fitType = FitType::None;
  LorentzianType m_lorentzianType = LorentzianType::None;
  DeltaType m_deltaType = DeltaType::None;
  TempCorrectionType m_tempCorrectionType = TempCorrectionType::None;
  BackgroundType m_backgroundType = BackgroundType::None;
  DataForParameterEstimationCollection m_estimationData;
  QList<ParamID> m_globals;
  FitSubType m_fitSubType;
  BackgroundSubType m_backgroundSubtype;
  std::vector<std::pair<std::string, size_t>> m_fitResolutions;
  std::vector<double> m_qValues;
  bool m_isQDependentFunction = false;
  std::unique_ptr<IDAFunctionParameterEstimation> m_parameterEstimation;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
