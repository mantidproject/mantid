#ifndef MANTID_KERNEL_UTILS_H_
#define MANTID_KERNEL_UTILS_H_
    
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{

/** Utils: General-purpose utility functions that do not belong anywhere else.
 *
 * NOTE! There are other places to put specific functions:
 *
 *  VectorHelper.h
 *  Strings.h
 *  Statistics.h
 *  Memory.h
 *
 * @author Janik Zikovsky
 * @date 2011-03-29 14:36:20.460710
 */
namespace Utils
{



  //------------------------------------------------------------------------------------------------
  /** Set up a nested for loop.
   *
   * @param numDims :: how many levels of nesting do the for loops have?
   * @param value :: fill the array to this.
   * @return an array of counters, set to 0, of the right size.
   */
  inline size_t * nestedForLoopSetUp(const size_t numDims, const size_t value = 0)
  {
    // Allocate and clear to 0.
    size_t * out = new size_t[numDims];
    for (size_t d=0; d<numDims; d++)
      out[d] = value;
    return out;
  }



  //------------------------------------------------------------------------------------------------
  /** Utility function for performing arbitrarily nested for loops in a serial way.
   *
   * @param numDims :: the number of dimensions (levels of nesting) to loop over
   * @param index :: an array[numDims] of the current counter index in each dimension.
   *        The index at the lowest dimension will be incremented, carrying over to higher dimensions.
   * @param index_max :: an array[numDims] of the maximum value (exclusive) of the index in each dimension
   * @param index_min :: an array[numDims] of the minimum value of the index in each dimension.
   * @return true if the end of the loop was reached; false otherwise.
   */
  inline bool nestedForLoopIncrement(const size_t numDims, size_t * index, size_t * index_max, size_t * index_min)
  {
    size_t d = 0;
    while (d < numDims)
    {
      // Add one to the index in this dimension
      if (++index[d] >= index_max[d])
      {
        // Roll this counter back to 0 (or whatever the min is)
        index[d] = index_min[d];
        // Go up one in a higher dimension
        d++;
        // Reached the maximum of the last dimension. Time to exit the entire loop.
        if (d == numDims)
          return true;
      }
      else
        return false;
    }
    return false;
  }


  //------------------------------------------------------------------------------------------------
  /** Utility function for performing arbitrarily nested for loops in a serial way.
   * This version assumes that the minimum in each dimension is 0.
   *
   * @param numDims :: the number of dimensions (levels of nesting) to loop over
   * @param index :: an array[numDims] of the counter index in each dimension.
   * @param index_max :: an array[numDims] of the maximum value (exclusive) of the index in each dimension
   * @return true if the end of the loop was reached; false otherwise.
   */
  inline bool nestedForLoopIncrement(const size_t numDims, size_t * index, size_t * index_max)
  {
    size_t d = 0;
    while (d < numDims)
    {
      // Add one to the index in this dimension
      if (++index[d] >= index_max[d])
      {
        // Roll this counter back to 0
        index[d] = 0;
        // Go up one in a higher dimension
        d++;
        // Reached the maximum of the last dimension. Time to exit the entire loop.
        if (d == numDims)
          return true;
      }
      else
        return false;
    }
    return false;
  }


} // namespace Utils


} // namespace Mantid
} // namespace Kernel

#endif  /* MANTID_KERNEL_UTILS_H_ */
