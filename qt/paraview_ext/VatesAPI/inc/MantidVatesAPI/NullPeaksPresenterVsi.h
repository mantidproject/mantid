#ifndef MANTID_VATES_NULL_PEAKS_PRESENTER
#define MANTID_VATES_NULL_PEAKS_PRESENTER

#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/System.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidVatesAPI/ViewFrustum.h"

#include <vector>

namespace Mantid {
namespace VATES {
class DLLExport NullPeaksPresenterVsi : public PeaksPresenterVsi {
public:
  NullPeaksPresenterVsi() {}
  ~NullPeaksPresenterVsi() override {}
  Mantid::API::IPeaksWorkspace_sptr getPeaksWorkspace() const override {
    throw std::runtime_error(
        "NullPeaksPresenterVsi does not implement this method. Misused");
  }
  std::vector<bool> getViewablePeaks() const override {
    throw std::runtime_error(
        "NullPeaksPresenterVsi does not implement this method. Misused");
  }
  void updateViewFrustum(ViewFrustum_const_sptr) override {}
  std::string getFrame() const override {
    throw std::runtime_error(
        "NullPeaksPresenterVsi does not implement this method. Misused");
  }
  std::string getPeaksWorkspaceName() const override {
    throw std::runtime_error(
        "NullPeaksPresenterVsi does not implement this method. Misused");
  }
  void getPeaksInfo(Mantid::API::IPeaksWorkspace_sptr, int,
                    Mantid::Kernel::V3D &, double &,
                    Mantid::Kernel::SpecialCoordinateSystem) const override {
    throw std::runtime_error(
        "NullPeaksPresenterVsi does not implement this method. Misused");
  }
  void sortPeaksWorkspace(const std::string &, const bool) override {}
};
} // namespace VATES
} // namespace Mantid

#endif