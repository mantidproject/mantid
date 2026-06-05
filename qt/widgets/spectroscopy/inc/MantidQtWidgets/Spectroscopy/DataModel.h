// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "FitData.h"
#include "IDataModel.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
using namespace MantidWidgets;

/*
   DataModel - Specifies an interface for updating, querying and
   accessing the raw data in Tabs
*/
class MANTID_SPECTROSCOPY_DLL DataModel : public IDataModel {
public:
  DataModel();
  virtual ~DataModel() override = default;
  std::vector<FitData> *getFittingData() override;

  void addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) override;
  void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra) override;
  Mantid::API::MatrixWorkspace_sptr getWorkspace(WorkspaceID workspaceID) const override;
  Mantid::API::MatrixWorkspace_sptr getWorkspace(FitDomainIndex index) const override;
  std::vector<std::string> getWorkspaceNames() const override;
  WorkspaceID getNumberOfWorkspaces() const override;
  bool hasWorkspace(std::string const &workspaceName) const override;
  WorkspaceID getWorkspaceID(std::string const &workspaceName) const override;

  void setSpectra(const std::string &spectra, WorkspaceID workspaceID) override;
  void setSpectra(FunctionModelSpectra &&spectra, WorkspaceID workspaceID) override;
  void setSpectra(const FunctionModelSpectra &spectra, WorkspaceID workspaceID) override;
  FunctionModelSpectra getSpectra(WorkspaceID workspaceID) const override;
  FunctionModelDataset getDataset(WorkspaceID workspaceID) const override;
  size_t getSpectrum(FitDomainIndex index) const override;
  size_t getNumberOfSpectra(WorkspaceID workspaceID) const override;

  void clear() override;

  size_t getNumberOfDomains() const override;
  FitDomainIndex getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;
  std::pair<WorkspaceID, WorkspaceIndex> getSubIndices(FitDomainIndex) const override;
  std::vector<double> getQValuesForData() const override;
  std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const override;
  std::string getResolutionName(const WorkspaceID &wsID, const WorkspaceIndex &index) const override;
  std::string createDisplayName(WorkspaceID workspaceID) const override;

  void removeWorkspace(WorkspaceID workspaceID) override;
  void removeDataByIndex(FitDomainIndex fitDomainIndex) override;

  void setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) override;
  void setStartX(double startX, WorkspaceID workspaceID) override;
  void setStartX(double startX, FitDomainIndex fitDomainIndex) override;
  void setEndX(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) override;
  void setEndX(double endX, WorkspaceID workspaceID) override;
  void setEndX(double startX, FitDomainIndex fitDomainIndex) override;
  void setExcludeRegion(const std::string &exclude, WorkspaceID workspaceID, WorkspaceIndex spectrum) override;
  void setExcludeRegion(const std::string &exclude, FitDomainIndex index) override;
  bool setResolution(const std::string &resName, const std::string &wsName,
                     const FunctionModelSpectra &spectra) override;
  bool setResolution(const std::string &resName, WorkspaceID workspaceID, const FunctionModelSpectra &spectra) override;
  void removeResolution(const std::string &resName) override;
  std::set<std::string> getResolutionNames() override;
  std::pair<double, double> getFittingRange(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;
  std::pair<double, double> getFittingRange(FitDomainIndex index) const override;
  std::string getExcludeRegion(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;
  std::string getExcludeRegion(FitDomainIndex index) const override;
  std::vector<double> getExcludeRegionVector(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;
  std::vector<double> getExcludeRegionVector(FitDomainIndex index) const override;
  void removeSpecialValues(const std::string &name) override;
  void removeWorkspaceByName(const std::string &name) override;
  void clearModel() override;
  void updateWorkspaceNames() override;

  std::set<std::string> const &resolutionNames() const override;
  std::set<std::string> const &workspaceNames() const override;

protected:
  void addNewWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace, const FunctionModelSpectra &spectra);

private:
  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;
  std::unique_ptr<std::vector<FitData>> m_fittingData;
  std::set<std::string> m_uniqueWsNames;
  std::set<std::string> m_uniqueResWsNames;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
