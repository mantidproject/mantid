#ifndef MDEVENTWORKSPACE_H_
#define MDEVENTWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDPoint.h"
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
   * @tparam nv :: number of corner vertices per dimension. If only the
   *               center of the point is required, this == 0.
   *               If all corners are needed, this == nd.
   *               Default value == 0; meaning only the center coordinates are used.
   *
   * @tparam TE :: Type for a bit of extra data that can be carried around in each point.
   *               For example, this could be a single uint32 representing a detector ID.
   *               Or, if more complex things are required, this could be a struct with
   *               a few fields (should not be dynamically allocated like a pointer, in general).
   *               Default value == char[0] (occupying no memory).
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  template <size_t nd, size_t nv=0, typename TE=char>
  class DLLExport MDEventWorkspace  : public IMDEventWorkspace
  {
  public:
    /** Typedef of the basic MDPoint data type used in this MDEventWorkspace.
     * This is for convenience; an algorithm can declare
     *  MyWorkspace::Point somePoint;
     * without having to look up template parameters...
     */
    typedef MDPoint<nd,nv,TE> Point;

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
    Point getPoint(int n);

  protected:

  };





}//namespace MDEvents

}//namespace Mantid


#endif /* MDEVENTWORKSPACE_H_ */
