// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITOUTPUT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITOUTPUT_H_

#include "IndirectFitData.h"

#include "DllConfig.h"
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
  ResultLocation() = default;
  ResultLocation(Mantid::API::WorkspaceGroup_sptr group, GroupIndex i)
      : result(group), index(i) {}
  boost::weak_ptr<Mantid::API::WorkspaceGroup> result;
  GroupIndex index = GroupIndex{ 0 };
};

using ParameterValues =
    std::map<WorkspaceIndex,
                       std::unordered_map<std::string, ParameterValue>>;

using ResultLocations = std::map<WorkspaceIndex, ResultLocation>;

using FitDataIterator =
    std::vector<std::unique_ptr<IndirectFitData>>::const_iterator;

/*
    IndirectFitOutput - Stores the output of a QENS fit and provides
    convenient access to the output parameters.
*/
class MANTIDQT_INDIRECT_DLL IndirectFitOutput {
public:
  IndirectFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                    Mantid::API::ITableWorkspace_sptr parameterTable,
                    Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                    const FitDataIterator &fitDataBegin,
                    const FitDataIterator &fitDataEnd);

  IndirectFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                    Mantid::API::ITableWorkspace_sptr parameterTable,
                    Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                    IndirectFitData const *fitData, WorkspaceIndex spectrum);

  bool isSpectrumFit(IndirectFitData const *fitData, WorkspaceIndex spectrum) const;

  std::unordered_map<std::string, ParameterValue>
  getParameters(IndirectFitData const *fitData, WorkspaceIndex spectrum) const;

  boost::optional<ResultLocation>
  getResultLocation(IndirectFitData const *fitData, WorkspaceIndex spectrum) const;
  std::vector<std::string> getResultParameterNames() const;
  Mantid::API::WorkspaceGroup_sptr getLastResultWorkspace() const;
  Mantid::API::WorkspaceGroup_sptr getLastResultGroup() const;

  void mapParameterNames(
      const std::unordered_map<std::string, std::string> &parameterNameChanges,
      const FitDataIterator &fitDataBegin, const FitDataIterator &fitDataEnd);
  void mapParameterNames(
      const std::unordered_map<std::string, std::string> &parameterNameChanges,
      IndirectFitData const *fitData);
  void mapParameterNames(
      const std::unordered_map<std::string, std::string> &parameterNameChanges,
      IndirectFitData const *fitData, WorkspaceIndex spectrum);

  void addOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                 const FitDataIterator &fitDataBegin,
                 const FitDataIterator &fitDataEnd);
  void addOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                 IndirectFitData const *fitData, WorkspaceIndex spectrum);

  void removeOutput(IndirectFitData const *fitData);

private:
  void updateFitResults(Mantid::API::WorkspaceGroup_sptr resultGroup,
                        const FitDataIterator &fitDataBegin,
                        const FitDataIterator &fitDataEnd);
  void updateParameters(Mantid::API::ITableWorkspace_sptr parameterTable,
                        const FitDataIterator &fitDataBegin,
                        const FitDataIterator &fitDataEnd);
  void
  updateFitResultsFromUnstructured(Mantid::API::WorkspaceGroup_sptr resultGroup,
                                   const FitDataIterator &fitDataBegin,
                                   const FitDataIterator &fitDataEnd);
  void
  updateFitResultsFromStructured(Mantid::API::WorkspaceGroup_sptr resultGroup,
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
