#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITOUTPUT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITOUTPUT_H_

#include "IndirectFitData.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <boost/optional.hpp>

#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

struct ParameterValue {
  ParameterValue(double val) : value(val) {}
  ParameterValue(double val, double err) : value(val), error(err) {}
  double value;
  boost::optional<double> error;
};

struct ResultLocation {
  ResultLocation(boost::weak_ptr<WorkspaceGroup> group, std::size_t i)
      : result(group), index(i) {}
  boost::weak_ptr<WorkspaceGroup> result;
  std::size_t index;
};

using ParameterValues =
    std::unordered_map<std::size_t,
                       std::unordered_map<std::string, ParameterValue>>;

using ResultLocations = std::unordered_map<std::size_t, ResultLocation>;

class IndirectFitOutput {
public:
  IndirectFitOutput(
      Mantid::API::WorkspaceGroup_sptr resultGroup,
      Mantid::API::ITableWorkspace_sptr parameterTable,
      Mantid::API::MatrixWorkspace_sptr resultWorkspace,
      const std::vector<std::unique_ptr<IndirectFitData>> &fitData);

  IndirectFitOutput(
      Mantid::API::WorkspaceGroup_sptr resultGroup,
      Mantid::API::ITableWorkspace_sptr parameterTable,
      Mantid::API::MatrixWorkspace_sptr resultWorkspace,
      const std::vector<std::unique_ptr<IndirectFitData>> &fitData,
      const std::unordered_map<std::string, std::string> &parameterNameChanges);

  std::unordered_map<std::string, ParameterValue>
  getParameters(IndirectFitData *fitData, std::size_t spectra) const;

  boost::optional<ResultLocation> getResultLocation(IndirectFitData *fitData,
                                                    std::size_t spectra) const;
  Mantid::API::MatrixWorkspace_sptr getLastResultWorkspace() const;
  Mantid::API::WorkspaceGroup_sptr getLastResultGroup() const;

  void addOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::MatrixWorkspace_sptr resultWorkspace,
                 const std::vector<std::unique_ptr<IndirectFitData>> &fitData);
  void addOutput(
      Mantid::API::WorkspaceGroup_sptr resultGroup,
      Mantid::API::ITableWorkspace_sptr parameterTable,
      Mantid::API::MatrixWorkspace_sptr resultWorkspace,
      const std::vector<std::unique_ptr<IndirectFitData>> &fitData,
      const std::unordered_map<std::string, std::string> parameterNameChanges);

private:
  void updateParameters(
      Mantid::API::ITableWorkspace_sptr parameterTable,
      const std::vector<std::unique_ptr<IndirectFitData>> &fitData);
  void updateParameters(
      Mantid::API::ITableWorkspace_sptr parameterTable,
      const std::vector<std::unique_ptr<IndirectFitData>> &fitData,
      const std::unordered_map<std::string, std::string> parameterNameChanges);
  void updateFitResults(
      Mantid::API::WorkspaceGroup_sptr resultGroup,
      const std::vector<std::unique_ptr<IndirectFitData>> &fitData);

  boost::weak_ptr<WorkspaceGroup> m_resultGroup;
  boost::weak_ptr<MatrixWorkspace> m_resultWorkspace;
  std::unordered_map<IndirectFitData *, ParameterValues> m_parameters;
  std::unordered_map<IndirectFitData *, ResultLocations>
      m_outputResultLocations;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
