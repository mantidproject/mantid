#ifndef MANTID_DATAOBJECTS_OFFSETSWORKSPACE_H_
#define MANTID_DATAOBJECTS_OFFSETSWORKSPACE_H_

#include "MantidAPI/MatrixWorkspace.h"
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
  OffsetsWorkspace();
  OffsetsWorkspace(Geometry::Instrument_const_sptr inst);
  ~OffsetsWorkspace();

  /** Gets the name of the workspace type
  @return Standard string name  */
  virtual const std::string id() const { return "OffsetsWorkspace"; }

private:
  /// Private copy constructor. NO COPY ALLOWED
  OffsetsWorkspace(const OffsetsWorkspace &);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  OffsetsWorkspace &operator=(const OffsetsWorkspace &);
};

/// shared pointer to the OffsetsWorkspace class
typedef boost::shared_ptr<OffsetsWorkspace> OffsetsWorkspace_sptr;

/// shared pointer to a const OffsetsWorkspace
typedef boost::shared_ptr<const OffsetsWorkspace> OffsetsWorkspace_const_sptr;

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_DATAOBJECTS_OFFSETSWORKSPACE_H_ */
