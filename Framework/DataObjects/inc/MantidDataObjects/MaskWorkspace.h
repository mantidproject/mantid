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
  MaskWorkspace() = default;
  MaskWorkspace(std::size_t numvectors);
  MaskWorkspace(Mantid::Geometry::Instrument_const_sptr instrument,
                const bool includeMonitors = false);
  MaskWorkspace(const API::MatrixWorkspace_const_sptr parent);

  /// Returns a clone of the workspace
  std::unique_ptr<MaskWorkspace> clone() const {
    return std::unique_ptr<MaskWorkspace>(doClone());
  }
  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<MaskWorkspace> cloneEmpty() const {
    return std::unique_ptr<MaskWorkspace>(doCloneEmpty());
  }
  MaskWorkspace &operator=(const MaskWorkspace &other) = delete;
  bool isMasked(const detid_t detectorID) const override;
  bool isMasked(const std::set<detid_t> &detectorIDs) const override;
  bool isMaskedIndex(const std::size_t wkspIndex) const;
  void setMasked(const detid_t detectorID, const bool mask = true) override;
  void setMasked(const std::set<detid_t> &detectorIDs,
                 const bool mask = true) override;
  void setMaskedIndex(const std::size_t wkspIndex, const bool mask = true);
  std::size_t getNumberMasked() const override;
  std::set<detid_t> getMaskedDetectors() const;
  std::set<std::size_t> getMaskedWkspIndices() const;

  const std::string id() const override;

  /// Copy the set up from another workspace
  void copyFrom(boost::shared_ptr<const SpecialWorkspace2D> sourcews) override;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  MaskWorkspace(const MaskWorkspace &) = default;

  /// Return human-readable string
  const std::string toString() const override;

private:
  MaskWorkspace *doClone() const override { return new MaskWorkspace(*this); }
  MaskWorkspace *doCloneEmpty() const override { return new MaskWorkspace(); }

  IMaskWorkspace *doInterfaceClone() const override { return doClone(); };
  /// Clear original incorrect mask
  void clearMask();

  /// Check whether any instrument associated
  bool hasInstrument() const;
};

/// shared pointer to the MaskWorkspace class
using MaskWorkspace_sptr = boost::shared_ptr<MaskWorkspace>;

/// shared pointer to a const MaskWorkspace
using MaskWorkspace_const_sptr = boost::shared_ptr<const MaskWorkspace>;

} // namespace DataObjects
} // namespace Mantid

#endif // MANTID_DATAOBJECTS_MASKWORKSPACE_H
