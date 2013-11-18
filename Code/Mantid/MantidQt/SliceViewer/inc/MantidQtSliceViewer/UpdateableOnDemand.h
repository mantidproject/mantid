#ifndef UPDATEABLEONDEMAND_H_
#define UPDATEABLEONDEMAND_H_

#include "MantidKernel/System.h"

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
    // Destructor
    virtual ~UpdateableOnDemand(){}
  };
}
}




#endif /* UPDATEABLEONDEMAND_H_ */
