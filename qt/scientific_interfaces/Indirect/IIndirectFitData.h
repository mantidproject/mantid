// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <string>

#include "DllConfig.h"
#include "IndexTypes.h"
#include "IndirectFitOutput.h"
#include "IndirectFitdata.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using IndirectFitDataCollectionType =
    IndexCollectionType<TableDatasetIndex, std::unique_ptr<IndirectFitData>>;

struct PrivateFittingData {
  friend class IndirectFittingModel;

public:
  PrivateFittingData();
  PrivateFittingData &operator=(PrivateFittingData &&fittingData);

private:
  PrivateFittingData(PrivateFittingData &&privateData);
  PrivateFittingData(IndirectFitDataCollectionType &&data);
  IndirectFitDataCollectionType m_data;
};

/*
    IIndirectFitData - Specifies an interface for updating, querying and
   accessing the raw data in IndirectFitAnalysisTabs
*/
class MANTIDQT_INDIRECT_DLL IIndirectFitData {
public:
  virtual bool hasWorkspace(std::string const &workspaceName) const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr
  getWorkspace(TableDatasetIndex index) const = 0;
  virtual Spectra getSpectra(TableDatasetIndex index) const = 0;
  virtual bool isMultiFit() const = 0;
  virtual TableDatasetIndex numberOfWorkspaces() const = 0;
  virtual int getNumberOfSpectra(TableDatasetIndex index) const = 0;
  virtual int getNumberOfDomains() const = 0;
  virtual FitDomainIndex getDomainIndex(TableDatasetIndex dataIndex,
                                        WorkspaceIndex spectrum) const = 0;
  virtual std::vector<double> getQValuesForData() const = 0;
  virtual std::vector<std::pair<std::string, int>>
  getResolutionsForFit() const = 0;
  virtual void setFittingData(PrivateFittingData &&fittingData) = 0;
  virtual PrivateFittingData clearWorkspaces() = 0;

  virtual void setSpectra(const std::string &spectra,
                          TableDatasetIndex dataIndex) = 0;
  virtual void setSpectra(Spectra &&spectra, TableDatasetIndex dataIndex) = 0;
  virtual void setSpectra(const Spectra &spectra,
                          TableDatasetIndex dataIndex) = 0;
  virtual void addWorkspace(const std::string &workspaceName) = 0;
  virtual void addWorkspace(const std::string &workspaceName,
                            const std::string &spectra) = 0;
  virtual void addWorkspace(const std::string &workspaceName,
                            const Spectra &spectra) = 0;
  virtual void removeWorkspace(TableDatasetIndex index) = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
