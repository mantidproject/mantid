#ifndef MANTID_DATAOBJECTS_GROUPINGWORKSPACE_H_
#define MANTID_DATAOBJECTS_GROUPINGWORKSPACE_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {

/** A GroupingWorkspace is a subclass of Workspace2D
 * where each spectrum has a single number entry, the value
 * of which signifies to which group that workspace index belongs.
 *
 * Group numbers are counted starting at 1!
 *
 * @author Janik Zikovsky
 * @date 2011-05-02
 */
class DLLExport GroupingWorkspace : public SpecialWorkspace2D {
public:
  GroupingWorkspace(Geometry::Instrument_const_sptr inst);
  GroupingWorkspace() = default;
  GroupingWorkspace(size_t numvectors);
  GroupingWorkspace &operator=(const GroupingWorkspace &) = delete;

  /// Returns a clone of the workspace
  std::unique_ptr<GroupingWorkspace> clone() const {
    return std::unique_ptr<GroupingWorkspace>(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<GroupingWorkspace> cloneEmpty() const {
    return std::unique_ptr<GroupingWorkspace>(doCloneEmpty());
  }

  /** Gets the name of the workspace type
  @return Standard string name  */
  const std::string id() const override { return "GroupingWorkspace"; }

  void makeDetectorIDToGroupMap(std::map<detid_t, int> &detIDToGroup,
                                int64_t &ngroups) const;
  void makeDetectorIDToGroupVector(std::vector<int> &detIDToGroup,
                                   int64_t &ngroups) const;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  GroupingWorkspace(const GroupingWorkspace &) = default;

private:
  GroupingWorkspace *doClone() const override {
    return new GroupingWorkspace(*this);
  }
  GroupingWorkspace *doCloneEmpty() const override {
    return new GroupingWorkspace();
  }
};

/// shared pointer to the GroupingWorkspace class
typedef boost::shared_ptr<GroupingWorkspace> GroupingWorkspace_sptr;

/// shared pointer to a const GroupingWorkspace
typedef boost::shared_ptr<const GroupingWorkspace> GroupingWorkspace_const_sptr;

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_DATAOBJECTS_GROUPINGWORKSPACE_H_ */
