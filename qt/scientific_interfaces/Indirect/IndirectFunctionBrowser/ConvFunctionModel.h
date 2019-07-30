// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECT_CONVFUNCTIONMODEL_H_
#define MANTIDQT_INDIRECT_CONVFUNCTIONMODEL_H_

#include "ConvTypes.h"
#include "IndexTypes.h"
#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
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
  double getParameter(const QString &paramName) const override;
  double getParameterError(const QString &paramName) const override;
  QString getParameterDescription(const QString &paramName) const override;
  QStringList getParameterNames() const override;
  IFunction_sptr getSingleFunction(int index) const override;
  IFunction_sptr getCurrentFunction() const override;
  void setNumberDomains(int) override;
  void setDatasetNames(const QStringList &names) override;
  QStringList getDatasetNames() const override;
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
  QString setBackgroundA0(double value) override;

  void updateMultiDatasetParameters(const ITableWorkspace &paramTable);

  void setFitType(FitType fitType);
  void setDeltaFunction(bool);
  bool hasDeltaFunction() const;
  void setBackground(BackgroundType bgType);
  void removeBackground();
  bool hasBackground() const;
  void
  updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void setResolution(std::string const &name, DatasetIndex const &index);

  QMap<ParamID, double> getCurrentValues() const;
  QMap<ParamID, double> getCurrentErrors() const;
  QMap<int, QString> getParameterNameMap() const;

private:
  void clearData();
  QString buildFunctionString() const;
  boost::optional<QString> getLor1Prefix() const;
  boost::optional<QString> getLor2Prefix() const;
  boost::optional<QString> getDeltaPrefix() const;
  boost::optional<QString> getBackgroundPrefix() const;
  void setParameter(ParamID name, double value);
  boost::optional<double> getParameter(ParamID name) const;
  boost::optional<double> getParameterError(ParamID name) const;
  boost::optional<QString> getParameterName(ParamID name) const;
  boost::optional<QString> getParameterDescription(ParamID name) const;
  boost::optional<QString> getPrefix(ParamID name) const;
  void setCurrentValues(const QMap<ParamID, double> &);
  void applyParameterFunction(std::function<void(ParamID)> paramFun) const;
  boost::optional<ParamID> getParameterId(const QString &parName);
  std::string buildLorentzianFunctionString() const;
  std::string buildDeltaFunctionString() const;
  std::string buildBackgroundFunctionString() const;
  void addGlobal(const QString &parName);
  void removeGlobal(const QString &parName);
  QStringList makeGlobalList() const;
  int getNumberOfPeaks() const;

  FunctionModel m_model;
  FitType m_fitType = FitType::None;
  BackgroundType m_backgroundType = BackgroundType::None;
  //std::string m_background;
  bool m_hasDeltaFunction = false;
  DataForParameterEstimationCollection m_estimationData;
  QList<ParamID> m_globals;
  FitSubType m_fitSubType;
  BackgroundSubType m_backgroundSubtype;
  std::string m_resolutionName;
  DatasetIndex m_resolutionIndex;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_INDIRECT_CONVFUNCTIONMODEL_H_ */
