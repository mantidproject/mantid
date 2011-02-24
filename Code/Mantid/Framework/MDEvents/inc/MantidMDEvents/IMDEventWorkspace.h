#ifndef IMDEVENTWORKSPACE_H_
#define IMDEVENTWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace MDEvents
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
  DLLExport class IMDEventWorkspace  : public API::Workspace
  {
  public:
    /** Returns the number of dimensions in this workspace */
    virtual int getNumDims() const = 0;

    /** Returns the total number of points (events) in this workspace */
    virtual size_t getNPoints() const = 0;

  };





}//namespace MDEvents

}//namespace Mantid

#endif /* IMDEVENTWORKSPACE_H_ */
