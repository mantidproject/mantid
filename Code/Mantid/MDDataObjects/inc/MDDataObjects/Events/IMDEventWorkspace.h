#ifndef IMDEVENTWORKSPACE_H_
#define IMDEVENTWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MDDataObjects/Events/MDEvent.h"

namespace Mantid
{
namespace MDDataObjects
{

  /** Abstract base class for multi-dimension event workspaces (MDEventWorkspace).
   * This class will handle as much of the common operations as possible;
   * but since MDEventWorkspace is a templated class, that makes some aspects
   * impossible.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  DLLExport class IMDEventWorkspace
  {
  public:
    virtual int getNumDimensions() const = 0;
    virtual size_t getNumPoints() const = 0;
    virtual size_t getMemoryUsed() const = 0;
    //virtual MDEvent<3> getEvent3(size_t index) = 0;
  };





}//namespace MDDataObjects

}//namespace Mantid

#endif /* IMDEVENTWORKSPACE_H_ */
