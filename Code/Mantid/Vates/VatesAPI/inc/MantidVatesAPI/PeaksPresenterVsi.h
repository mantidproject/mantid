#ifndef MANTID_VATES_PEAKS_PRESENTER_VSI_H
#define MANTID_VATES_PEAKS_PRESENTER_VSI_H

#include "MantidKernel/System.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include <vector>
#include <string>

namespace Mantid {
namespace VATES {

class DLLExport PeaksPresenterVsi {
public:
  virtual ~PeaksPresenterVsi(){};
  virtual std::vector<bool> getViewablePeaks() const = 0;
  virtual Mantid::API::IPeaksWorkspace_sptr getPeaksWorkspace() const = 0;
  virtual void updateViewFrustum(ViewFrustum_const_sptr frustum) = 0;
  virtual std::string getFrame() const = 0;
  virtual std::string getPeaksWorkspaceName() const = 0;
  virtual void getPeaksInfo(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace,
                            int row, Mantid::Kernel::V3D &position,
                            double &radius, Mantid::Kernel::SpecialCoordinateSystem specialCoordinateSystem) const = 0;
  virtual void sortPeaksWorkspace(const std::string &byColumnName,
                                  const bool ascending) = 0;
};

typedef boost::shared_ptr<PeaksPresenterVsi> PeaksPresenterVsi_sptr;
typedef boost::shared_ptr<const PeaksPresenterVsi> PeaksPresenterVsi_const_sptr;
}
}
#endif