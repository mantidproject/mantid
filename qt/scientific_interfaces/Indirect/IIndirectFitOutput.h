// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include "MantidAPI/ITableWorkspace.h"

#include "MantidAPI/IAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

struct ParameterValue {
  ParameterValue() : value(0) {}
  explicit ParameterValue(double val) : value(val) {}
  ParameterValue(double val, double err) : value(val), error(err) {}
  double value;
  boost::optional<double> error;
};

struct ResultLocationNew {
  ResultLocationNew() = default;
  ResultLocationNew(const Mantid::API::WorkspaceGroup_sptr &group, WorkspaceID i) : result(group), index(i) {}
  std::weak_ptr<Mantid::API::WorkspaceGroup> result;
  WorkspaceID index = WorkspaceID{0};
};
/*
    IIndirectFitData - Specifies an interface for updating, querying and
   accessing the raw data in IndirectFitAnalysisTabs
*/
class MANTIDQT_INDIRECT_DLL IIndirectFitOutput {
public:
  virtual ~IIndirectFitOutput() = default;
  virtual bool isSpectrumFit(FitDomainIndex index) const = 0;

  virtual std::unordered_map<std::string, ParameterValue> getParameters(FitDomainIndex index) const = 0;

  virtual boost::optional<ResultLocationNew> getResultLocation(FitDomainIndex index) const = 0;
  virtual std::vector<std::string> getResultParameterNames() const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getLastResultWorkspace() const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getLastResultGroup() const = 0;

  virtual void clear() = 0;
  virtual bool isEmpty() const = 0;

  virtual void addOutput(const Mantid::API::WorkspaceGroup_sptr &resultGroup,
                         Mantid::API::ITableWorkspace_sptr parameterTable,
                         const Mantid::API::WorkspaceGroup_sptr &resultWorkspace) = 0;

  virtual void addSingleOutput(const Mantid::API::WorkspaceGroup_sptr &resultGroup,
                               Mantid::API::ITableWorkspace_sptr parameterTable,
                               const Mantid::API::WorkspaceGroup_sptr &resultWorkspace,
                               FitDomainIndex fitDomainIndex) = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
