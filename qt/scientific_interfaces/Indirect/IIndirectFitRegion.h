// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <utility>

#include "DllConfig.h"
#include "IndexTypes.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/*
    IIndirectFitData - Specifies an interface for updating, querying and
   accessing the raw data in IndirectFitAnalysisTabs
*/
class MANTIDQT_INDIRECT_DLL IIndirectFitRegion {
public:
  virtual std::pair<double, double>
  getFittingRange(TableDatasetIndex dataIndex,
                  WorkspaceIndex spectrum) const = 0;
  virtual std::string getExcludeRegion(TableDatasetIndex dataIndex,
                                       WorkspaceIndex index) const = 0;

  virtual void setStartX(double startX, TableDatasetIndex dataIndex,
                         WorkspaceIndex spectrum) = 0;
  virtual void setStartX(double startX, TableDatasetIndex dataIndex) = 0;
  virtual void setEndX(double endX, TableDatasetIndex dataIndex,
                       WorkspaceIndex spectrum) = 0;
  virtual void setEndX(double endX, TableDatasetIndex dataIndex) = 0;
  virtual void setExcludeRegion(const std::string &exclude,
                                TableDatasetIndex dataIndex,
                                WorkspaceIndex spectrum) = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt