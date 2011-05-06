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



  //TODO: Separate these into a namespace like NestedForLoop::

  //------------------------------------------------------------------------------------------------
  /** Set up a nested for loop by creating an array of counters.
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
  /** Set up an "index maker" for a  nested for loop.
   *
   * Makes an array of size [numDims] that is used to make a linearized index
   * out of dimensional indices. If the output is "out" and the array of indices is "index":
   *  linear_index = out[0] * index[0] + out[1] * index[1] + ...
   *
   * The lowest dimension index (0) will vary the slowest.
   *
   * @param numDims :: how many levels of nesting do the for loops have?
   * @param index_max :: an array[numDims] of the maximum value (exclusive) of the index in each dimension.
   *        The minimum must be 0 in each dimension for the algorithm to work
   * @return an array of linear_index, set to 0, of the right size.
   */
  inline size_t * nestedForLoopSetUpIndexMaker(const size_t numDims, const size_t * index_max)
  {
    // Allocate and start at 1
    size_t * out = new size_t[numDims];
    for (size_t d=0; d<numDims; d++)
      out[d] = 1;

    for (size_t d=1; d<numDims; d++)
      out[d] =  out[d-1] * index_max[d-1];

    return out;
  }


  //------------------------------------------------------------------------------------------------
  /** Return a linear index from dimensional indices of a nested for loop.
   *
   * linear_index = index_maker[0] * index[0] + index_maker[1] * index[1] + ...
   *
   * The lowest dimension index (0) will vary the slowest.
   *
   * @param numDims :: how many levels of nesting do the for loops have?
   * @param index :: an array[numDims] of the counter index in each dimension.
   * @param index_maker :: result of nestedForLoopSetUpIndexMaker()
   * @return the linear index into the array
   */
  inline size_t nestedForLoopGetLinearIndex(const size_t numDims, size_t * index, size_t * index_maker)
  {
    size_t out = 0;
    for (size_t d=0; d<numDims; d++)
      out += index[d] * index_maker[d];
    return out;
  }


  //------------------------------------------------------------------------------------------------
  /** Set up a nested for loop by creating an array of counters.
   *
   * @param numDims :: how many levels of nesting do the for loops have?
   * @param linear_index :: linear index into the nested for loop.
   * @param index_maker :: an array[numDims], result of nestedForLoopSetUpIndexMaker()
   * @param index_max :: an array[numDims] of the maximum value (exclusive) of the index in each dimension.
   *        The minimum must be 0 in each dimension for the algorithm to work
   * @param[out] out_indices :: an array, sized numDims, which will be
   *             filled with the index for each dimension, given the linear index
   */
  inline void nestedForLoopGetIndicesFromLinearIndex(const size_t numDims, const size_t linear_index,
      const size_t * index_maker, const size_t * index_max,
      size_t * out_indices)
  {
    for (size_t d=0; d<numDims; d++)
    {
      out_indices[d] = (linear_index / index_maker[d]) % index_max[d];
    }
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
