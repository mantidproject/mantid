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
  ParameterValue() : value(0) {}
  explicit ParameterValue(double val) : value(val) {}
  ParameterValue(double val, double err) : value(val), error(err) {}
  double value;
  boost::optional<double> error;
};

struct ResultLocation {
  ResultLocation() : result(), index(0) {}
  ResultLocation(boost::weak_ptr<Mantid::API::WorkspaceGroup> group,
                 std::size_t i)
      : result(group), index(i) {}
  boost::weak_ptr<Mantid::API::WorkspaceGroup> result;
  std::size_t index;
};

using ParameterValues =
    std::unordered_map<std::size_t,
                       std::unordered_map<std::string, ParameterValue>>;

using ResultLocations = std::unordered_map<std::size_t, ResultLocation>;

using FitDataIterator =
    std::vector<std::unique_ptr<IndirectFitData>>::const_iterator;

class IndirectFitOutput {
public:
  IndirectFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                    Mantid::API::ITableWorkspace_sptr parameterTable,
                    Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                    const FitDataIterator &fitDataBegin,
                    const FitDataIterator &fitDataEnd);

  IndirectFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                    Mantid::API::ITableWorkspace_sptr parameterTable,
                    Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                    IndirectFitData const *fitData, std::size_t spectrum);

  bool isSpectrumFit(IndirectFitData const *fitData,
                     std::size_t spectrum) const;

  std::unordered_map<std::string, ParameterValue>
  getParameters(IndirectFitData const *fitData, std::size_t spectrum) const;

  boost::optional<ResultLocation> getResultLocation(IndirectFitData *fitData,
                                                    std::size_t spectrum) const;
  std::vector<std::string> getResultParameterNames() const;
  Mantid::API::WorkspaceGroup_sptr getLastResultWorkspace() const;
  Mantid::API::WorkspaceGroup_sptr getLastResultGroup() const;

  void mapParameterNames(
      const std::unordered_map<std::string, std::string> &parameterNameChanges,
      const FitDataIterator &fitDataBegin, const FitDataIterator &fitDataEnd);
  void mapParameterNames(
      const std::unordered_map<std::string, std::string> &parameterNameChanges,
      IndirectFitData const *fitData, std::size_t spectrum);

  void addOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                 const FitDataIterator &fitDataBegin,
                 const FitDataIterator &fitDataEnd);
  void addOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                 IndirectFitData const *fitData, std::size_t spectrum);

  void removeOutput(IndirectFitData const *fitData);

private:
  void updateParameters(Mantid::API::ITableWorkspace_sptr parameterTable,
                        const FitDataIterator &fitDataBegin,
                        const FitDataIterator &fitDataEnd);
  void updateFitResults(Mantid::API::WorkspaceGroup_sptr resultGroup,
                        const FitDataIterator &fitDataBegin,
                        const FitDataIterator &fitDataEnd);

  boost::weak_ptr<Mantid::API::WorkspaceGroup> m_resultGroup;
  boost::weak_ptr<Mantid::API::WorkspaceGroup> m_resultWorkspace;
  std::unordered_map<IndirectFitData const *, ParameterValues> m_parameters;
  std::unordered_map<IndirectFitData const *, ResultLocations>
      m_outputResultLocations;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
