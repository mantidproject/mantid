#ifndef MDEVENTWORKSPACE_H_
#define MDEVENTWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MDDataObjects/Events/MDEvent.h"
#include "MDDataObjects/Events/MDPoint.h"
#include "MDDataObjects/Events/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid
{
namespace MDDataObjects
{

  /** Templated class for the multi-dimensional event workspace.
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
  template <size_t nd, size_t nv=0, typename TE=char[0]>
  DLLExport class MDEventWorkspace  : public IMDEventWorkspace
  {
  public:
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
    MDPoint<nd,nv,TE> getPoint(int n);

  protected:


  };





}//namespace MDDataObjects

}//namespace Mantid


#endif /* MDEVENTWORKSPACE_H_ */
