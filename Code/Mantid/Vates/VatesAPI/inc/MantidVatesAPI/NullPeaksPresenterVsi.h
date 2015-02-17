#ifndef MANTID_VATES_NULL_PEAKS_PRESENTER
#define MANTID_VATES_NULL_PEAKS_PRESENTER

#include "MantidKernel/System.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include <vector>

namespace Mantid
{
namespace VATES
{
  class DLLExport NullPeaksPresenterVsi : public PeaksPresenterVsi
  {
    public:
    NullPeaksPresenterVsi(){}
    virtual ~NullPeaksPresenterVsi(){}
    virtual Mantid::API::IPeaksWorkspace_sptr getPeaksWorkspace(){return Mantid::API::IPeaksWorkspace_sptr();};
    virtual std::vector<bool> getViewablePeaks() {return std::vector<bool>();}
    virtual void updateViewFrustum(ViewFrustum frustum) {};
    virtual std::string getFrame(){return std::string();}
  };
}
}

#endif