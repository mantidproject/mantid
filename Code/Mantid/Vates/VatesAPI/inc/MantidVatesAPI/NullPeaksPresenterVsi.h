#ifndef MANTID_VATES_NULL_PEAKS_PRESENTER
#define MANTID_VATES_NULL_PEAKS_PRESENTER

#include "MantidKernel/System.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidKernel/SpecialCoordinateSystem.h"

#include <vector>

namespace Mantid {
namespace VATES {
class DLLExport NullPeaksPresenterVsi : public PeaksPresenterVsi {
public:
  NullPeaksPresenterVsi() {}
  virtual ~NullPeaksPresenterVsi() {}
  virtual Mantid::API::IPeaksWorkspace_sptr getPeaksWorkspace() const {
    throw std::runtime_error(
        "NullPeaksPresenterVsi does not implement this method. Misused");
  }
  virtual std::vector<bool> getViewablePeaks() const {
    throw std::runtime_error(
        "NullPeaksPresenterVsi does not implement this method. Misused");
  }
  virtual void updateViewFrustum(ViewFrustum) {}
  virtual std::string getFrame() const {
    throw std::runtime_error(
        "NullPeaksPresenterVsi does not implement this method. Misused");
  }
  virtual std::string getPeaksWorkspaceName() const {
    throw std::runtime_error(
        "NullPeaksPresenterVsi does not implement this method. Misused");
  }
  virtual void getPeaksInfo(Mantid::API::IPeaksWorkspace_sptr, int,
                            Mantid::Kernel::V3D &, double &, Mantid::Kernel::SpecialCoordinateSystem) const {
    throw std::runtime_error(
        "NullPeaksPresenterVsi does not implement this method. Misused");
  }
  virtual void sortPeaksWorkspace(const std::string &,
                                  const bool ) {}
};
}
}

#endif