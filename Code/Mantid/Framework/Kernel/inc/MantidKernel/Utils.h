#ifndef MANTID_KERNEL_UTILS_H_
#define MANTID_KERNEL_UTILS_H_
    
#include "MantidKernel/DllConfig.h"
#include <cmath>
#include <vector>

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
  /** Custom rounding method for a double->long because none is
   * portable in C++ (!)
   *
   * @param x :: floating point value to round
   * @return closest integer as a long (positive or negative)
   */
  inline long round(double x)
  {
    return long(x + (x < 0 ? -0.5 : +0.5));
  }

  //------------------------------------------------------------------------------------------------
  /** Custom rounding method for a double->double because none is
   * portable in C++ (!)
   *
   * @param r :: floating point value to round
   * @return closest integer as a double (positive or negative)
   */
  inline double rounddbl(double r)
  {
    return (r > 0.0) ? std::floor(r + 0.5) : std::ceil(r - 0.5);
  }

  //------------------------------------------------------------------------------------------------
  /** Custom rounding method for a double->double because none is
   * portable in C++ (!)
   *
   * @param r :: floating point value to round
   * @param f :: number of significant figures to preserve
   * @return r rounded to f significant figures
   */
  inline double roundToSF(double r, int f)
  {
    double factor = pow(10.0, f - ceil(log10(fabs(r))));
    return rounddbl(r * factor) / factor;
  }

  //------------------------------------------------------------------------------------------------
  /** Custom rounding method for a double->double because none is
   * portable in C++ (!)
   *
   * @param r :: floating point value to round
   * @param d :: number of digits after decimal point to preserve
   * @return r rounded to d decimal places
   */
  inline double roundToDP(double r, int d)
  {
    double m = pow(10.0, d);
    return (int)r + (1.0/m) * rounddbl((r - (int)r) * m);
  }


  namespace NestedForLoop
  {
  //------------------------------------------------------------------------------------------------
  /** Set up a nested for loop by setting an array of counters.
   *
   * @param numDims :: how many levels of nesting do the for loops have?
   * @param out :: a size-numDims array that will be modified
   * @param value :: fill the array to this.
   */
  inline void SetUp(const size_t numDims, size_t * out, const size_t value = 0)
  {
    // Allocate and clear to 0.
    for (size_t d=0; d<numDims; d++)
      out[d] = value;
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
   * @param out :: a size-numDims array that will be modified
   * @param index_max :: an array[numDims] of the maximum value (exclusive) of the index in each dimension.
   *        The minimum must be 0 in each dimension for the algorithm to work
   */
  inline void SetUpIndexMaker(const size_t numDims, size_t * out, const size_t * index_max)
  {
    // Allocate and start at 1
    for (size_t d=0; d<numDims; d++)
      out[d] = 1;

    for (size_t d=1; d<numDims; d++)
      out[d] =  out[d-1] * index_max[d-1];
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
   * @param index_maker :: result of SetUpIndexMaker()
   * @return the linear index into the array
   */
  inline size_t GetLinearIndex(const size_t numDims, size_t * index, size_t * index_maker)
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
   * @param index_maker :: an array[numDims], result of SetUpIndexMaker()
   * @param index_max :: an array[numDims] of the maximum value (exclusive) of the index in each dimension.
   *        The minimum must be 0 in each dimension for the algorithm to work
   * @param[out] out_indices :: an array, sized numDims, which will be
   *             filled with the index for each dimension, given the linear index
   */
  inline void GetIndicesFromLinearIndex(const size_t numDims, const size_t linear_index,
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
  inline bool Increment(const size_t numDims, size_t * index, size_t * index_max, size_t * index_min)
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
  inline bool Increment(const size_t numDims, size_t * index, size_t * index_max)
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


} // namespace NestedForLoop
  //------------------------------------------------------------------------------------------------
  /** Convert an linear index in nDim workspace into vector of loop indexes of nDim depth loop
   *  Unsafe (pointers) version, used by safe vectors version
   *
   * @param linear_index :: linear index into the nested for loop.
   * @param numBins    :: an array[numDims], with number of bins in each dimension
   * @param numDims :: how many levels of nesting do the for loops have and the size of the numBins array above
   *
   * @param[out] out_indices :: an array, sized numDims, which will be
   *             filled with the index for each dimension, given the linear index
   */
  inline void getIndicesFromLinearIndex(const size_t linear_index,
      size_t const * const numBins, const size_t numDims, size_t * const out_indices)
  {
    // number of bins in first dimension
    size_t nBins  = *(numBins+0);
    // first index
    size_t ind    = linear_index%nBins;
    *(out_indices+0)= ind;
    // what left in linear index after first was removed;
    size_t rest = linear_index/nBins;
    for (size_t d=1; d<numDims; d++)
    {
      nBins = *(numBins+d);
      ind  = rest%nBins;
      *(out_indices+d)= ind;
      rest = rest/nBins;
    }
  }
  //------------------------------------------------------------------------------------------------
  /** Convert an linear index in nDim workspace into vector of loop indexes of nDim depth loop
   * 
   * @param linear_index :: linear index into the nested for loop.
   * @param num_bins :: a vector of [numDims] size, where numDims is the loop depth and each element equal to number of bins in the correspondent dimension
   *
   * @param[out] out_indices :: the vector, sized numDims, which will be
   *             filled with the index for each dimension, given the linear index
   */
  inline void getIndicesFromLinearIndex(const size_t linear_index,
      const std::vector<size_t> & num_bins, std::vector<size_t> & out_indices)
  {
    if(num_bins.empty())
    {
      out_indices.clear();
      return;
    }
    else
    {
      size_t nBins = num_bins.size();
      out_indices.resize(nBins);
      getIndicesFromLinearIndex(linear_index,&num_bins[0],nBins,&out_indices[0]);
    }

  }

  /**
   * Determine, using an any-vertex touching type approach, whether the neighbour linear index corresponds to a true neighbour of the subject, which is already
   * decomposed into it's constituent dimension indices. subject is already expressed in it's constituent indices for speed.
   *
   * The approach used here is to determine if a dimension index differ by any more than 1 in any dimension. If it does, then the neighbour does not represent
   * a valid neighbour of the subject.
   * @param ndims
   * @param neighbour_linear_index
   * @param subject_indices
   * @param num_bins
   * @param index_max
   * @return True if the are neighbours, otherwise false.
   */
  inline bool isNeighbourOfSubject(const size_t ndims, const size_t neighbour_linear_index, const size_t* subject_indices, const size_t * num_bins, const size_t * index_max)
  {
    std::vector<size_t> neighbour_indices(ndims);
    Utils::NestedForLoop::GetIndicesFromLinearIndex(ndims, neighbour_linear_index, num_bins, index_max, &neighbour_indices.front());

    for(size_t ind = 0; ind < ndims; ++ind)
    {
      long double diff = std::abs(static_cast<long double>(subject_indices[ind]) - static_cast<long double>(neighbour_indices[ind]));
      if (diff > 1)
      {
        return false;
      }
    }
    return true;
  }


} // namespace Utils


} // namespace Mantid
} // namespace Kernel

#endif  /* MANTID_KERNEL_UTILS_H_ */
