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
#include "IndirectFitdata.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

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
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
