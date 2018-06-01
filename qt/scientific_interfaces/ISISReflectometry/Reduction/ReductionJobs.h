#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#include "../DllConfig.h"
#include <boost/optional.hpp>
#include "Group.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs {
public:
  bool appendGroup(UnslicedGroup group);
  bool insertGroup(UnslicedGroup group, int beforeIndex);
  bool canAddGroup(UnslicedGroup const &group) const;
  void removeGroup(int index);

  std::vector<UnslicedGroup>& groups();
  std::vector<UnslicedGroup> const &groups() const;
  UnslicedGroup const &operator[](int index) const;

private:
  std::vector<UnslicedGroup> m_groups;
};
}
}

#endif // MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
