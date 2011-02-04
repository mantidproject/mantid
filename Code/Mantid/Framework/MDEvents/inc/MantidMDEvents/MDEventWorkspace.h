#ifndef MDEVENTWORKSPACE_H_
#define MDEVENTWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** Templated class for the multi-dimensional event workspace.
   *
   * Template parameters are the same as the MDPoint<> template params, and
   * determine the basic MDPoint type used.
   *
   * @tparam nd :: the number of dimensions that each MDPoint will be tracking.
   *               an usigned int > 0.
   *
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport MDEventWorkspace  : public IMDEventWorkspace
  {
  public:
    /** Typedef of the basic MDEvent data type used in this MDEventWorkspace.
     * This is for convenience; an algorithm can declare
     *  MyWorkspace::Event someEvent;
     * without having to look up template parameters...
     */
    typedef MDEvent<nd> Event;

    /** Returns the number of dimensions in this workspace */
    virtual int getNumDims() const;

    /** Returns the total number of points (events) in this workspace */
    virtual size_t getNPoints() const;

    /** Returns the number of bytes of memory
     * used by the workspace. */
    virtual size_t getMemoryUsed() const;

    /** Sample function returning the n-th point in the workspace.
     * This may not be needed.
     *  */
    Event getPoint(size_t n);

  protected:

  };





}//namespace MDEvents

}//namespace Mantid


#endif /* MDEVENTWORKSPACE_H_ */
