#include "ReductionJobs.h"
#include <iostream>
namespace MantidQt {
namespace CustomInterfaces {

bool ReductionJobs::appendGroup(UnslicedGroup group) {
  if (canAddGroup(group)) {
    m_groups.emplace_back(std::move(group));
    return true;
  } else {
    return false;
  }
}

bool ReductionJobs::insertGroup(UnslicedGroup group, int beforeIndex) {
  if (canAddGroup(group)) {
    std::cout << "insertGroup before item at index " << beforeIndex
              << std::endl;
    m_groups.insert(m_groups.begin() + beforeIndex, std::move(group));
    return true;
  } else {
    return false;
  }
}

bool ReductionJobs::canAddGroup(UnslicedGroup const &group) const {
  auto const &name = group.name();
  return std::find_if(m_groups.cbegin(), m_groups.cend(),
                      [name](UnslicedGroup const &group) -> bool {
                        return group.name() == name;
                      }) == m_groups.cend();
}

void ReductionJobs::removeGroup(int index) {
  m_groups.erase(m_groups.cbegin() + index);
}

std::vector<UnslicedGroup> &ReductionJobs::groups() { return m_groups; }

std::vector<UnslicedGroup> const &ReductionJobs::groups() const {
  return m_groups;
}

UnslicedGroup const &ReductionJobs::operator[](int index) const {
  return m_groups[index];
}
}
}
