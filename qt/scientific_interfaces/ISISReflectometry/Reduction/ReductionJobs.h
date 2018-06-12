#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#include "../DllConfig.h"
#include <boost/optional.hpp>
#include "Group.h"

namespace MantidQt {
namespace CustomInterfaces {

template <typename Group> class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs {
public:
  ReductionJobs() = default;
  ReductionJobs(std::vector<Group> groups);
  Group& appendGroup(Group group);
  Group& insertGroup(Group group, int beforeIndex);
  bool hasGroupWithName(std::string const& name) const;
  void removeGroup(int index);

  std::vector<Group> &groups();
  std::vector<Group> const &groups() const;
  Group const &operator[](int index) const;

private:
  std::vector<Group> m_groups;
};

template class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs<SlicedGroup>;
using SlicedReductionJobs = ReductionJobs<SlicedGroup>;

template class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs<UnslicedGroup>;
using UnslicedReductionJobs = ReductionJobs<UnslicedGroup>;

using Jobs = boost::variant<UnslicedReductionJobs, SlicedReductionJobs>;

void appendEmptyRow(Jobs &jobs, int groupIndex);
void insertEmptyRow(Jobs &jobs, int groupIndex, int beforeRow);
void removeRow(Jobs &jobs, int groupIndex, int rowIndex);
void updateRow(Jobs &jobs, int groupIndex, int rowIndex, boost::optional<RowVariant> const& newValue);

void appendEmptyGroup(Jobs &jobs);
void insertEmptyGroup(Jobs &jobs, int beforeGroup);
void removeGroup(Jobs &jobs, int groupIndex);

bool setGroupName(Jobs &jobs, int groupIndex, std::string const &newValue);
void prettyPrintModel(Jobs const &jobs);

void mergeRowInto(Jobs &jobs, RowVariant const& row, double thetaTolerance);

UnslicedReductionJobs unsliced(SlicedReductionJobs const &slicedJobs);
SlicedReductionJobs sliced(UnslicedReductionJobs const &unslicedJobs);
}
}

#endif // MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
