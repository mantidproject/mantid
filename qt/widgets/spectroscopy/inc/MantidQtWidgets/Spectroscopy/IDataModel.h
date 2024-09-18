// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <string>
#include <utility>

#include "DllConfig.h"
#include "FitData.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
using namespace MantidWidgets;

using FitDataCollectionType = IndexCollectionType<WorkspaceID, std::unique_ptr<FitData>>;
/*
   IDataModel - Specifies an interface for updating, querying and
   accessing the raw data in Tabs
*/

class MANTID_SPECTROSCOPY_DLL IDataModel {
public:
  virtual ~IDataModel() = default;
  virtual std::vector<FitData> *getFittingData() = 0;
  virtual bool hasWorkspace(std::string const &workspaceName) const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getWorkspace(WorkspaceID workspaceID) const = 0;
  virtual FunctionModelSpectra getSpectra(WorkspaceID workspaceID) const = 0;
  virtual FunctionModelDataset getDataset(WorkspaceID workspaceID) const = 0;
  virtual WorkspaceID getNumberOfWorkspaces() const = 0;
  virtual size_t getNumberOfSpectra(WorkspaceID workspaceID) const = 0;
  virtual size_t getNumberOfDomains() const = 0;
  virtual FitDomainIndex getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual std::vector<double> getQValuesForData() const = 0;
  virtual std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const = 0;

  virtual std::vector<std::string> getWorkspaceNames() const = 0;
  virtual std::string createDisplayName(WorkspaceID workspaceID) const = 0;

  virtual void setSpectra(const std::string &spectra, WorkspaceID workspaceID) = 0;
  virtual void setSpectra(FunctionModelSpectra &&spectra, WorkspaceID workspaceID) = 0;
  virtual void setSpectra(const FunctionModelSpectra &spectra, WorkspaceID workspaceID) = 0;
  virtual void addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) = 0;
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra) = 0;
  virtual void removeWorkspace(WorkspaceID workspaceID) = 0;
  virtual void removeDataByIndex(FitDomainIndex fitDomainIndex) = 0;
  virtual void clear() = 0;

  virtual std::pair<double, double> getFittingRange(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual std::string getExcludeRegion(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual std::vector<double> getExcludeRegionVector(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual void setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) = 0;
  virtual void setStartX(double startX, WorkspaceID workspaceID) = 0;
  virtual void setStartX(double startX, FitDomainIndex fitDomainIndex) = 0;
  virtual void setEndX(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) = 0;
  virtual void setEndX(double endX, WorkspaceID workspaceID) = 0;
  virtual void setEndX(double startX, FitDomainIndex fitDomainIndex) = 0;
  virtual void setExcludeRegion(const std::string &exclude, WorkspaceID workspaceID, WorkspaceIndex spectrum) = 0;
  virtual bool setResolution(const std::string &name) = 0;
  virtual bool setResolution(const std::string &name, WorkspaceID workspaceID) = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getWorkspace(FitDomainIndex index) const = 0;
  virtual std::pair<double, double> getFittingRange(FitDomainIndex index) const = 0;
  virtual size_t getSpectrum(FitDomainIndex index) const = 0;
  virtual std::vector<double> getExcludeRegionVector(FitDomainIndex index) const = 0;
  virtual std::string getExcludeRegion(FitDomainIndex index) const = 0;
  virtual void setExcludeRegion(const std::string &exclude, FitDomainIndex index) = 0;

  virtual std::pair<WorkspaceID, WorkspaceIndex> getSubIndices(FitDomainIndex) const = 0;
  virtual void removeSpecialValues(const std::string &name) = 0;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
