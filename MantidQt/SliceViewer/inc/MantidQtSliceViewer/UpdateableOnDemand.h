#ifndef UPDATEABLEONDEMAND_H_
#define UPDATEABLEONDEMAND_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace API
{
class IPeaksWorkspace;
}
}

namespace MantidQt
{
namespace SliceViewer
{
  /**
   * Abstract class for types that can be forced to update themselves upon request.
   */
  class DLLExport UpdateableOnDemand
  {
  public:
    // Force the implementation to update itself
    virtual void performUpdate() = 0;
    // Deliver a new peaks workspace for replacement of an existing one.
    virtual void updatePeaksWorkspace(const std::string& toName, boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toWorkspace) = 0;
    // Destructor
    virtual ~UpdateableOnDemand(){}
  };
}
}




#endif /* UPDATEABLEONDEMAND_H_ */
