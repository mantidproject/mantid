// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

/*
    IIndirectFitData - Specifies an interface for updating, querying and
   accessing the raw data in IndirectFitAnalysisTabs
*/
class MANTIDQT_INDIRECT_DLL IIndirectFittingModel {
public:
  virtual bool isPreviouslyFit(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const = 0;
  virtual boost::optional<std::string> isInvalidFunction() const = 0;
  virtual std::vector<std::string> getFitParameterNames() const = 0;
  virtual Mantid::API::MultiDomainFunction_sptr getFittingFunction() const = 0;
  virtual std::unordered_map<std::string, ParameterValue> getParameterValues(TableDatasetIndex dataIndex,
                                                                             WorkspaceIndex spectrum) const = 0;

  virtual void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) = 0;
  virtual void setDefaultParameterValue(const std::string &name, double value, TableDatasetIndex dataIndex) = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
