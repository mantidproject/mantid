#ifndef MANTID_VATES_PEAKS_PRESENTER_VSI_H
#define MANTID_VATES_PEAKS_PRESENTER_VSI_H

#include "MantidKernel/System.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include <vector>
#include <string>


namespace Mantid
{
namespace VATES
{
class ViewFrustum;

class DLLExport PeaksPresenterVsi
{
  public:
    virtual ~PeaksPresenterVsi(){};
    virtual std::vector<bool> getViewablePeaks() = 0;
    virtual Mantid::API::IPeaksWorkspace_sptr getPeaksWorkspace() = 0;
    virtual void updateViewFrustum(ViewFrustum frustum) = 0;
    virtual std::string getFrame() = 0;
};
}
}
#endif