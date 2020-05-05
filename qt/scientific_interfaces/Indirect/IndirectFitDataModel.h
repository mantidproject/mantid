// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <string>

#include "DllConfig.h"
#include "IIndirectFitData.h"
#include "IndexTypes.h"
#include "IndirectFitdata.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/*
    IIndirectFitData - Specifies an interface for updating, querying and
   accessing the raw data in IndirectFitAnalysisTabs
*/
class MANTIDQT_INDIRECT_DLL IndirectFitDataModel : public IIndirectFitData {
public:
  IndirectFitDataModel();
  virtual ~IndirectFitDataModel() = default;
  bool hasWorkspace(std::string const &workspaceName) const override;
  Mantid::API::MatrixWorkspace_sptr
  getWorkspace(TableDatasetIndex index) const override;
  Spectra getSpectra(TableDatasetIndex index) const override;
  bool isMultiFit() const override;
  TableDatasetIndex numberOfWorkspaces() const override;
  int getNumberOfSpectra(TableDatasetIndex index) const override;
  int getNumberOfDomains() const override;
  FitDomainIndex getDomainIndex(TableDatasetIndex dataIndex,
                                WorkspaceIndex spectrum) const override;
  std::vector<double> getQValuesForData() const;
  std::vector<std::pair<std::string, int>>
  getResolutionsForFit() const override;
  std::vector<std::string> getWorkspaceNames() const override;

  void setSpectra(const std::string &spectra,
                  TableDatasetIndex dataIndex) override;
  void setSpectra(Spectra &&spectra, TableDatasetIndex dataIndex) override;
  void setSpectra(const Spectra &spectra, TableDatasetIndex dataIndex) override;
  void addWorkspace(const std::string &workspaceName) override;
  void addWorkspace(const std::string &workspaceName,
                    const std::string &spectra) override;
  void addWorkspace(const std::string &workspaceName,
                    const Spectra &spectra) override;
  void removeWorkspace(TableDatasetIndex index) override;
  void removeDataByIndex(FitDomainIndex fitDomainIndex) override;

  void clear() override;

  std::pair<double, double>
  getFittingRange(TableDatasetIndex dataIndex,
                  WorkspaceIndex spectrum) const override;
  std::string getExcludeRegion(TableDatasetIndex dataIndex,
                               WorkspaceIndex index) const override;
  std::vector<double>
  getExcludeRegionVector(TableDatasetIndex dataIndex,
                         WorkspaceIndex index) const override;

  void setStartX(double startX, TableDatasetIndex dataIndex,
                 WorkspaceIndex spectrum) override;
  void setStartX(double startX, TableDatasetIndex dataIndex) override;
  void setEndX(double endX, TableDatasetIndex dataIndex,
               WorkspaceIndex spectrum) override;
  void setEndX(double endX, TableDatasetIndex dataIndex) override;
  void setExcludeRegion(const std::string &exclude, TableDatasetIndex dataIndex,
                        WorkspaceIndex spectrum) override;
  void setResolution(const std::string &name, TableDatasetIndex index) override;

  Mantid::API::MatrixWorkspace_sptr
  getWorkspace(FitDomainIndex index) const override;
  std::pair<double, double>
  getFittingRange(FitDomainIndex index) const override;
  int getSpectrum(FitDomainIndex index) const override;
  std::vector<double>
  getExcludeRegionVector(FitDomainIndex index) const override;
  std::string getExcludeRegion(FitDomainIndex index) const override;
  std::pair<TableDatasetIndex, WorkspaceIndex>
      getSubIndices(FitDomainIndex) const override;

  void switchToSingleInputMode() override;
  void switchToMultipleInputMode() override;

protected:
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                            const Spectra &spectra);

private:
  void IndirectFitDataModel::addNewWorkspace(
      const Mantid::API::MatrixWorkspace_sptr &workspace,
      const Spectra &spectra);

  std::vector<IndirectFitData> *m_fittingData;
  std::vector<std::weak_ptr<Mantid::API::MatrixWorkspace>> *m_resolutions;

  std::unique_ptr<std::vector<IndirectFitData>> m_fittingDataSingle;
  std::unique_ptr<std::vector<std::weak_ptr<Mantid::API::MatrixWorkspace>>>
      m_resolutionsSingle;

  std::unique_ptr<std::vector<IndirectFitData>> m_fittingDataMultiple;
  std::unique_ptr<std::vector<std::weak_ptr<Mantid::API::MatrixWorkspace>>>
      m_resolutionsMultiple;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
