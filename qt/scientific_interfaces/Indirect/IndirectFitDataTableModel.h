// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <string>

#include "DllConfig.h"
#include "IIndirectFitDataTableModel.h"
#include "IndirectFitData.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

/*
   IndirectFitDataTableModel - Specifies an interface for updating, querying and
   accessing the raw data in IndirectFitAnalysisTabs
*/
class MANTIDQT_INDIRECT_DLL IndirectFitDataTableModel : public IIndirectFitDataTableModel {
public:
  IndirectFitDataTableModel();
  virtual ~IndirectFitDataTableModel() = default;
  bool hasWorkspace(std::string const &workspaceName) const override;
  Mantid::API::MatrixWorkspace_sptr getWorkspace(WorkspaceID workspaceID) const override;
  FunctionModelSpectra getSpectra(WorkspaceID workspaceID) const override;
  bool isMultiFit() const override;
  WorkspaceID getNumberOfWorkspaces() const override;
  size_t getNumberOfSpectra(WorkspaceID workspaceID) const override;
  size_t getNumberOfDomains() const override;
  FitDomainIndex getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;
  std::vector<double> getQValuesForData() const override;
  std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const override;
  std::vector<std::string> getWorkspaceNames() const override;

  void setSpectra(const std::string &spectra, WorkspaceID workspaceID) override;
  void setSpectra(FunctionModelSpectra &&spectra, WorkspaceID workspaceID) override;
  void setSpectra(const FunctionModelSpectra &spectra, WorkspaceID workspaceID) override;
  void addWorkspace(const std::string &workspaceName) override;
  void addWorkspace(const std::string &workspaceName, const std::string &spectra) override;
  void addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) override;
  void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra) override;
  void removeWorkspace(WorkspaceID workspaceID) override;
  void removeDataByIndex(FitDomainIndex fitDomainIndex) override;

  void clear() override;

  std::pair<double, double> getFittingRange(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;
  std::string getExcludeRegion(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;
  std::vector<double> getExcludeRegionVector(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;

  void setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) override;
  void setStartX(double startX, WorkspaceID workspaceID) override;
  void setEndX(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) override;
  void setEndX(double endX, WorkspaceID workspaceID) override;
  void setExcludeRegion(const std::string &exclude, WorkspaceID workspaceID, WorkspaceIndex spectrum) override;
  void setResolution(const std::string &name, WorkspaceID workspaceID) override;

  Mantid::API::MatrixWorkspace_sptr getWorkspace(FitDomainIndex index) const override;
  std::pair<double, double> getFittingRange(FitDomainIndex index) const override;
  size_t getSpectrum(FitDomainIndex index) const override;
  std::vector<double> getExcludeRegionVector(FitDomainIndex index) const override;
  std::string getExcludeRegion(FitDomainIndex index) const override;
  void setExcludeRegion(const std::string &exclude, FitDomainIndex index) override;
  std::pair<WorkspaceID, WorkspaceIndex> getSubIndices(FitDomainIndex) const override;

private:
  void addNewWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace, const FunctionModelSpectra &spectra);

  std::vector<IndirectFitData> *m_fittingData;
  std::vector<std::weak_ptr<Mantid::API::MatrixWorkspace>> *m_resolutions;

  std::unique_ptr<std::vector<IndirectFitData>> m_fittingDataMultiple;
  std::unique_ptr<std::vector<std::weak_ptr<Mantid::API::MatrixWorkspace>>> m_resolutionsMultiple;
  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
