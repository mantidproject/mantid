#ifndef MANTID_DATAOBJECTS_OFFSETSWORKSPACE_H_
#define MANTID_DATAOBJECTS_OFFSETSWORKSPACE_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {

/** An OffsetsWorkspace is a specialized Workspace2D where
 * the Y value at each pixel is the offset to be used for correcting
 * calculations (specifically AlignDetectors).
 *
 * @author Janik Zikovsky
 * @date 2011-05-09
 */
class DLLExport OffsetsWorkspace : public SpecialWorkspace2D {
public:
  OffsetsWorkspace() = default;
  OffsetsWorkspace(Geometry::Instrument_const_sptr inst);

  /// Returns a clone of the workspace
  std::unique_ptr<OffsetsWorkspace> clone() const {
    return std::unique_ptr<OffsetsWorkspace>(doClone());
  }
  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<OffsetsWorkspace> cloneEmpty() const {
    return std::unique_ptr<OffsetsWorkspace>(doCloneEmpty());
  }
  OffsetsWorkspace &operator=(const OffsetsWorkspace &) = delete;
  /** Gets the name of the workspace type
  @return Standard string name  */
  const std::string id() const override { return "OffsetsWorkspace"; }

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  OffsetsWorkspace(const OffsetsWorkspace &) = default;

private:
  OffsetsWorkspace *doClone() const override {
    return new OffsetsWorkspace(*this);
  }
  OffsetsWorkspace *doCloneEmpty() const override {
    return new OffsetsWorkspace();
  }
};

/// shared pointer to the OffsetsWorkspace class
typedef boost::shared_ptr<OffsetsWorkspace> OffsetsWorkspace_sptr;

/// shared pointer to a const OffsetsWorkspace
typedef boost::shared_ptr<const OffsetsWorkspace> OffsetsWorkspace_const_sptr;

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_DATAOBJECTS_OFFSETSWORKSPACE_H_ */
