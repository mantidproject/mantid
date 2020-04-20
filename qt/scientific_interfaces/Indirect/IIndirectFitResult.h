// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IndexTypes.h"
#include "MantidAPI/IFunction_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
/*
    IIndirectFitData - Specifies an interface for updating, querying and
   accessing the raw data in IndirectFitAnalysisTabs
*/
class MANTIDQT_INDIRECT_DLL IIndirectFitResult {
public:
  virtual bool isPreviouslyFit(TableDatasetIndex dataIndex,
                               WorkspaceIndex spectrum) const = 0;
  virtual boost::optional<std::string> isInvalidFunction() const = 0;
  virtual std::vector<std::string> getFitParameterNames() const = 0;
  virtual Mantid::API::MultiDomainFunction_sptr getFittingFunction() const = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt