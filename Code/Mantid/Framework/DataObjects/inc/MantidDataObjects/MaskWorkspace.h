#ifndef MANTID_DATAOBJECTS_MASKWORKSPACE_H
#define MANTID_DATAOBJECTS_MASKWORKSPACE_H

#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {

class DLLExport MaskWorkspace : public SpecialWorkspace2D,
                                public API::IMaskWorkspace {
public:
  MaskWorkspace();
  MaskWorkspace(std::size_t numvectors);
  MaskWorkspace(Mantid::Geometry::Instrument_const_sptr instrument,
                const bool includeMonitors = false);
  MaskWorkspace(const API::MatrixWorkspace_const_sptr parent);
  ~MaskWorkspace();

  /// Returns a clone of the workspace
  std::unique_ptr<MaskWorkspace> clone() const {
    return std::unique_ptr<MaskWorkspace>(doClone());
  }

  bool isMasked(const detid_t detectorID) const;
  bool isMasked(const std::set<detid_t> &detectorIDs) const;
  bool isMaskedIndex(const std::size_t wkspIndex) const;
  void setMasked(const detid_t detectorID, const bool mask = true);
  void setMasked(const std::set<detid_t> &detectorIDs, const bool mask = true);
  void setMaskedIndex(const std::size_t wkspIndex, const bool mask = true);
  std::size_t getNumberMasked() const;
  std::set<detid_t> getMaskedDetectors() const;
  std::set<std::size_t> getMaskedWkspIndices() const;

  virtual const std::string id() const;

  /// Copy the set up from another workspace
  virtual void copyFrom(boost::shared_ptr<const SpecialWorkspace2D> sourcews);

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  MaskWorkspace(const MaskWorkspace &other);

  /// Protected copy assignment operator. Assignment not implemented.
  MaskWorkspace &operator=(const MaskWorkspace &other);

  /// Return human-readable string
  virtual const std::string toString() const;

private:
  virtual MaskWorkspace *doClone() const { return new MaskWorkspace(*this); }

  /// Clear original incorrect mask
  void clearMask();

  /// Check whether any instrument associated
  bool hasInstrument() const;
};

/// shared pointer to the MaskWorkspace class
typedef boost::shared_ptr<MaskWorkspace> MaskWorkspace_sptr;

/// shared pointer to a const MaskWorkspace
typedef boost::shared_ptr<const MaskWorkspace> MaskWorkspace_const_sptr;

} // namespace DataObjects
} // namespace Mantid

#endif // MANTID_DATAOBJECTS_MASKWORKSPACE_H
