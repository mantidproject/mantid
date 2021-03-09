// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ConvTypes.h"
#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/ConvolutionFunctionModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "ParameterEstimation.h"

#include <QMap>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace ConvTypes;
using namespace Mantid::API;
using namespace MantidWidgets;

class MANTIDQT_INDIRECT_DLL ConvFunctionModel : public IFunctionModel {
public:
  ConvFunctionModel();
  void setFunction(IFunction_sptr fun) override;
  IFunction_sptr getFitFunction() const override;
  bool hasFunction() const override;
  void addFunction(const QString &prefix, const QString &funStr) override;
  void removeFunction(const QString &prefix) override;
  void setParameter(const QString &paramName, double value) override;
  void setParameterError(const QString &paramName, double value) override;
  FitType getFitType() const;
  LorentzianType getLorentzianType() const;
  BackgroundType getBackgroundType() const;
  double getParameter(const QString &paramName) const override;
  double getParameterError(const QString &paramName) const override;
  QString getParameterDescription(const QString &paramName) const override;
  QStringList getParameterNames() const override;
  IFunction_sptr getSingleFunction(int index) const override;
  IFunction_sptr getCurrentFunction() const override;
  void setNumberDomains(int) override;
  void setDatasets(const QList<FunctionModelDataset> &datasets) override;
  QStringList getDatasetNames() const override;
  QStringList getDatasetDomainNames() const override;
  int getNumberDomains() const override;
  void setCurrentDomainIndex(int i) override;
  int currentDomainIndex() const override;
  void changeTie(const QString &paramName, const QString &tie) override;
  void addConstraint(const QString &functionIndex,
                     const QString &constraint) override;
  void removeConstraint(const QString &paramName) override;
  QStringList getGlobalParameters() const override;
  void setGlobalParameters(const QStringList &globals) override;
  bool isGlobal(const QString &parName) const override;
  void setGlobal(const QString &parName, bool on);
  QStringList getLocalParameters() const override;
  void updateMultiDatasetParameters(const IFunction &fun) override;
  void updateParameters(const IFunction &fun) override;

  double getLocalParameterValue(const QString &parName, int i) const override;
  bool isLocalParameterFixed(const QString &parName, int i) const override;
  QString getLocalParameterTie(const QString &parName, int i) const override;
  QString getLocalParameterConstraint(const QString &parName,
                                      int i) const override;
  void setLocalParameterValue(const QString &parName, int i,
                              double value) override;
  void setLocalParameterValue(const QString &parName, int i, double value,
                              double error) override;
  void setLocalParameterFixed(const QString &parName, int i,
                              bool fixed) override;
  void setLocalParameterTie(const QString &parName, int i,
                            const QString &tie) override;
  void setLocalParameterConstraint(const QString &parName, int i,
                                   const QString &constraint) override;
  void setGlobalParameterValue(const QString &paramName, double value) override;
  QString setBackgroundA0(double value) override;

  void updateMultiDatasetParameters(const ITableWorkspace &paramTable);

  void setFitType(FitType fitType);
  void setLorentzianType(LorentzianType lorentzianType);
  void setDeltaFunction(bool);
  bool hasDeltaFunction() const;
  void setTempCorrection(bool, double value);
  bool hasTempCorrection() const;
  double getTempValue() const;
  void setBackground(BackgroundType bgType);
  void removeBackground();
  bool hasBackground() const;
  void
  updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void setResolution(std::string const &name, TableDatasetIndex const &index);
  void setResolution(
      const std::vector<std::pair<std::string, size_t>> &fitResolutions);
  void setQValues(const std::vector<double> &qValues);

  QMap<ParamID, double> getCurrentValues() const;
  QMap<ParamID, double> getCurrentErrors() const;
  QMap<int, QString> getParameterNameMap() const;

private:
  void clearData();
  void setModel();
  boost::optional<QString> getLor1Prefix() const;
  boost::optional<QString> getLor2Prefix() const;
  boost::optional<QString> getFitTypePrefix() const;
  boost::optional<QString> getDeltaPrefix() const;
  boost::optional<QString> getBackgroundPrefix() const;
  void setParameter(ParamID name, double value);
  boost::optional<double> getParameter(ParamID name) const;
  boost::optional<double> getParameterError(ParamID name) const;
  boost::optional<QString> getParameterName(ParamID name) const;
  boost::optional<QString> getParameterDescription(ParamID name) const;
  boost::optional<QString> getPrefix(ParamID name) const;
  void setCurrentValues(const QMap<ParamID, double> &);
  void
  applyParameterFunction(const std::function<void(ParamID)> &paramFun) const;
  boost::optional<ParamID> getParameterId(const QString &parName);
  std::string buildLorentzianFunctionString() const;
  std::string buildTeixeiraFunctionString() const;
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
  void addGlobal(const QString &parName);
  void removeGlobal(const QString &parName);
  QStringList makeGlobalList() const;
  int getNumberOfPeaks() const;
  void checkConvolution(const IFunction_sptr &fun);
  void checkSingleFunction(const IFunction_sptr &fun, bool &isLorentzianTypeSet,
                           bool &isFiTypeSet);

  ConvolutionFunctionModel m_model;
  FitType m_fitType = FitType::None;
  LorentzianType m_lorentzianType = LorentzianType::None;
  BackgroundType m_backgroundType = BackgroundType::None;
  bool m_hasDeltaFunction = false;
  bool m_hasTempCorrection = false;
  double m_tempValue = 100.0;
  DataForParameterEstimationCollection m_estimationData;
  QList<ParamID> m_globals;
  FitSubType m_fitSubType;
  BackgroundSubType m_backgroundSubtype;
  std::string m_resolutionName;
  TableDatasetIndex m_resolutionIndex;
  std::vector<std::pair<std::string, size_t>> m_fitResolutions;
  std::vector<double> m_qValues;
  bool m_isQDependentFunction = false;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
