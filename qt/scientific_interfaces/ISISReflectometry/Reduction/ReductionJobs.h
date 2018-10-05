// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#include "../DllConfig.h"
#include <boost/optional.hpp>

#include "Group.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL Jobs {
public:
  Jobs();
  Jobs(std::vector<Group> groups);
  Group &appendGroup(Group group);
  Group &insertGroup(Group group, int beforeIndex);
  bool hasGroupWithName(std::string const &groupName) const;
  boost::optional<int> indexOfGroupWithName(std::string const &groupName);
  void removeGroup(int index);

  std::vector<Group> &groups();
  std::vector<Group> const &groups() const;
  Group const &operator[](int index) const;
  std::string nextEmptyGroupName();

private:
  std::vector<Group> m_groups;
  size_t m_groupNameSuffix;
};

void appendEmptyRow(Jobs &jobs, int groupIndex);
void insertEmptyRow(Jobs &jobs, int groupIndex, int beforeRow);
void removeRow(Jobs &jobs, int groupIndex, int rowIndex);
void updateRow(Jobs &jobs, int groupIndex, int rowIndex,
               boost::optional<Row> const &newValue);

void appendEmptyGroup(Jobs &jobs);
void insertEmptyGroup(Jobs &jobs, int beforeGroup);
void removeGroup(Jobs &jobs, int groupIndex);

bool setGroupName(Jobs &jobs, int groupIndex, std::string const &newValue);
std::string groupName(Jobs const &jobs, int groupIndex);
void prettyPrintModel(Jobs const &jobs);

void mergeRowIntoGroup(Jobs &jobs, Row const &row, double thetaTolerance,
                       std::string const &groupName);

template <typename ModificationListener>
void mergeJobsInto(Jobs &intoHere, Jobs const &fromHere, double thetaTolerance,
                   ModificationListener &listener) {
  for (auto const &group : fromHere.groups()) {
    auto maybeGroupIndex = intoHere.indexOfGroupWithName(group.name());
    if (maybeGroupIndex.is_initialized()) {
      auto indexToUpdateAt = maybeGroupIndex.get();
      auto &intoGroup = intoHere.groups()[indexToUpdateAt];
      mergeRowsInto(intoGroup, group, indexToUpdateAt, thetaTolerance,
                    listener);
    } else {
      intoHere.appendGroup(group);
      listener.groupAppended(static_cast<int>(intoHere.groups().size()) - 1,
                             group);
    }
  }
}
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
