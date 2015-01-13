#ifndef MANTID_DATAOBJECTS_GROUPINGWORKSPACE_H_
#define MANTID_DATAOBJECTS_GROUPINGWORKSPACE_H_

#include "MantidAPI/MatrixWorkspace.h"
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
  GroupingWorkspace();
  GroupingWorkspace(size_t numvectors);
  ~GroupingWorkspace();

  /** Gets the name of the workspace type
  @return Standard string name  */
  virtual const std::string id() const { return "GroupingWorkspace"; }

  void makeDetectorIDToGroupMap(std::map<detid_t, int> &detIDToGroup,
                                int64_t &ngroups) const;
  void makeDetectorIDToGroupVector(std::vector<int> &detIDToGroup,
                                   int64_t &ngroups) const;

private:
  /// Private copy constructor. NO COPY ALLOWED
  GroupingWorkspace(const GroupingWorkspace &);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  GroupingWorkspace &operator=(const GroupingWorkspace &);
};

/// shared pointer to the GroupingWorkspace class
typedef boost::shared_ptr<GroupingWorkspace> GroupingWorkspace_sptr;

/// shared pointer to a const GroupingWorkspace
typedef boost::shared_ptr<const GroupingWorkspace> GroupingWorkspace_const_sptr;

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_DATAOBJECTS_GROUPINGWORKSPACE_H_ */
