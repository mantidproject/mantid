// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IIndirectFitData.h"
#include "IIndirectFitOutput.h"
#include "IIndirectFitResult.h"
#include "IndexTypes.h"
#include "IndirectFitData.h"
#include "IndirectWorkspaceNames.h"
#include "ParameterEstimation.h"

#include "DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class FittingMode { SEQUENTIAL, SIMULTANEOUS };
extern std::unordered_map<FittingMode, std::string> fitModeToName;

class IndirectFittingModel;

using DefaultParametersType =
    IndexCollectionType<TableDatasetIndex,
                        std::unordered_map<std::string, ParameterValue>>;

/*
    IndirectFittingModel - Provides methods for specifying and
    performing a QENS fit, as well as accessing the results of the fit.
*/
class MANTIDQT_INDIRECT_DLL IndirectFittingModel : public IIndirectFitResult {
public:
  IndirectFittingModel();
  virtual ~IndirectFittingModel() = default;

  // IIndirectFitData
  bool hasWorkspace(std::string const &workspaceName) const;
  Mantid::API::MatrixWorkspace_sptr getWorkspace(TableDatasetIndex index) const;
  Spectra getSpectra(TableDatasetIndex index) const;
  virtual bool isMultiFit() const;
  TableDatasetIndex numberOfWorkspaces() const;
  int getNumberOfSpectra(TableDatasetIndex index) const;
  int getNumberOfDomains() const;
  FitDomainIndex getDomainIndex(TableDatasetIndex dataIndex,
                                WorkspaceIndex spectrum) const;
  std::vector<double> getQValuesForData() const;
  virtual std::vector<std::pair<std::string, int>> getResolutionsForFit() const;
  void clearWorkspaces();
  void clear();

  void setSpectra(const std::string &spectra, TableDatasetIndex dataIndex);
  void setSpectra(Spectra &&spectra, TableDatasetIndex dataIndex);
  void setSpectra(const Spectra &spectra, TableDatasetIndex dataIndex);
  void addWorkspace(const std::string &workspaceName);
  void addWorkspace(const std::string &workspaceName,
                    const std::string &spectra);
  void addWorkspace(const std::string &workspaceName, const Spectra &spectra);
  virtual void removeWorkspace(TableDatasetIndex index);

  // IIndirectFitRegion
  virtual std::pair<double, double>
  getFittingRange(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  virtual std::string getExcludeRegion(TableDatasetIndex dataIndex,
                                       WorkspaceIndex index) const;

  void setStartX(double startX, TableDatasetIndex dataIndex,
                 WorkspaceIndex spectrum);
  void setStartX(double startX, TableDatasetIndex dataIndex);
  void setEndX(double endX, TableDatasetIndex dataIndex,
               WorkspaceIndex spectrum);
  void setEndX(double endX, TableDatasetIndex dataIndex);
  void setExcludeRegion(const std::string &exclude, TableDatasetIndex dataIndex,
                        WorkspaceIndex spectrum);

  // Functions concerned with naming

  // IIndirectFitResult
  bool isPreviouslyFit(TableDatasetIndex dataIndex,
                       WorkspaceIndex spectrum) const override;
  virtual boost::optional<std::string> isInvalidFunction() const override;
  std::vector<std::string> getFitParameterNames() const override;
  virtual Mantid::API::MultiDomainFunction_sptr
  getFittingFunction() const override;
  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) override;
  void setDefaultParameterValue(const std::string &name, double value,
                                TableDatasetIndex dataIndex) override;
  std::unordered_map<std::string, ParameterValue>
  getParameterValues(TableDatasetIndex dataIndex,
                     WorkspaceIndex spectrum) const override;
  std::unordered_map<std::string, ParameterValue>
  getFitParameters(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  std::unordered_map<std::string, ParameterValue>
  getDefaultParameters(TableDatasetIndex dataIndex) const;

  // IIndirectFitOutput
  void addSingleFitOutput(const Mantid::API::IAlgorithm_sptr &fitAlgorithm,
                          TableDatasetIndex index);
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  // Generic
  void setFittingMode(FittingMode mode);
  FittingMode getFittingMode() const;

  void setFitTypeString(const std::string &fitType);
  boost::optional<ResultLocationNew>
  getResultLocation(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const;
  Mantid::API::WorkspaceGroup_sptr getResultGroup() const;
  virtual Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const;
  Mantid::API::IAlgorithm_sptr getSingleFit(TableDatasetIndex dataIndex,
                                            WorkspaceIndex spectrum) const;
  Mantid::API::IFunction_sptr getSingleFunction(TableDatasetIndex dataIndex,
                                                WorkspaceIndex spectrum) const;
  std::string getOutputBasename() const;
  virtual std::string createDisplayName(TableDatasetIndex dataIndex) const;

  void cleanFailedRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm);
  void
  cleanFailedSingleRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm,
                       TableDatasetIndex index);
  DataForParameterEstimationCollection
  getDataForParameterEstimation(const EstimationDataSelector &selector) const;
  std::unique_ptr<IIndirectFitData> m_fitDataModel;

protected:
  std::string createOutputName(std::string fitMode) const;
  Mantid::API::IAlgorithm_sptr getFittingAlgorithm(FittingMode mode) const;
  Mantid::API::IAlgorithm_sptr
  createSequentialFit(Mantid::API::IFunction_sptr function) const;
  Mantid::API::IAlgorithm_sptr createSimultaneousFit(
      const Mantid::API::MultiDomainFunction_sptr &function) const;
  virtual Mantid::API::MultiDomainFunction_sptr getMultiDomainFunction() const;
  virtual std::unordered_map<std::string, std::string>
  mapDefaultParameterNames() const;
  std::string createSingleFitOutputName(const std::string &formatString,
                                        TableDatasetIndex index,
                                        WorkspaceIndex spectrum) const;
  void removeFittingData(TableDatasetIndex index);
  std::string m_fitType = "FitType";
  std::string m_fitString = "FitString";

private:
  std::vector<std::string> getWorkspaceNames() const;
  std::vector<double> getExcludeRegionVector(TableDatasetIndex dataIndex,
                                             WorkspaceIndex index) const;

  void removeWorkspaceFromFittingData(TableDatasetIndex const &index);

  Mantid::API::IAlgorithm_sptr
  createSequentialFit(const Mantid::API::IFunction_sptr &function,
                      const std::string &input) const;
  virtual Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const;
  virtual Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const;
  virtual std::string sequentialFitOutputName() const;
  virtual std::string simultaneousFitOutputName() const;
  virtual std::string singleFitOutputName(TableDatasetIndex index,
                                          WorkspaceIndex spectrum) const = 0;
  virtual std::unordered_map<std::string, ParameterValue>
  createDefaultParameters(TableDatasetIndex index) const;

  virtual std::string getResultXAxisUnit() const;
  virtual std::string getResultLogName() const;

  bool isPreviousModelSelected() const;

  std::unique_ptr<IIndirectFitOutput> m_fitOutput;
  Mantid::API::MultiDomainFunction_sptr m_activeFunction;
  // stores the single domain function
  Mantid::API::IFunction_sptr m_fitFunction;
  DefaultParametersType m_defaultParameters;
  // Stores whether the current fit function is the same as
  bool m_previousModelSelected;
  FittingMode m_fittingMode;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
