#ifndef MANTID_KERNEL_NEARESTNEIGHBOURS_H_
#define MANTID_KERNEL_NEARESTNEIGHBOURS_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ANN/ANN.h"
#include "MantidKernel/make_unique.h"

#include <Eigen/Core>
#include <Eigen/StdVector>
#include <type_traits>
#include <vector>

namespace Mantid {
namespace Kernel {

//------------------------------------------------------------------------------
// Helper classes
//------------------------------------------------------------------------------

/**
 * NNDataPoints is a thin RAII wrapper class around the ANNpointArray type. This
 * takes care of the proper allocation and deallocation of memory.
 */
class NNDataPoints {
public:
  /** Construct a new set of data points
   *
   * @param nPts :: the number of data points
   * @param nElems :: the number of elements for each point
   */
  NNDataPoints(const size_t nPts, const size_t nElems) : m_nPts(nPts) {
    m_data = annAllocPts(static_cast<int>(m_nPts), static_cast<int>(nElems));
  }

  /** Return a handle to the raw ANNpointArray wrapped by this class
   *
   * @return handle to the raw ANNpointArray
   */
  ANNpointArray rawData() {
    return m_data;
  }

  /** Access a raw point in the collection of points
   *
   * This will check the index used is within bounds and return nullptr if
   * outside of those bounds
   *
   * @param i :: the index of the point to return a handle to
   * @return handle to a single point in the collection of points
   */
  ANNcoord* mutablePoint(const size_t i) {
    if (i < m_nPts)
      return m_data[i];
    else
      return nullptr;
  }

  ~NNDataPoints() { annDeallocPts(m_data); }
  NNDataPoints(const NNDataPoints&) = delete;

private:
  /// Number of points stored
  const size_t m_nPts;
  /// Array of points for use with NN search
  ANNpointArray m_data;
};

//------------------------------------------------------------------------------
// NearestNeighbours implementation
//------------------------------------------------------------------------------

template <size_t N = 3>
class MANTID_KERNEL_DLL NearestNeighbours {
  // typedefs for code brevity
  typedef Eigen::Array<double, N, 1> ArrayType;
  typedef std::vector<std::tuple<ArrayType, size_t, double>> NearestNeighbourResults;

public:
  /** Create a nearest neighbour search object
   *
   * @param points :: vector of Eigen::Arrays to search through
   */
  NearestNeighbours(const std::vector<ArrayType, Eigen::aligned_allocator<ArrayType>> & points)
  {
    const auto numPoints = static_cast<int>(points.size());
    if (numPoints == 0)
      std::runtime_error("Need at least one point to initilise NearestNeighbours.");

    m_dataPoints = make_unique<NNDataPoints>(numPoints, N);

    for (size_t i = 0; i < points.size(); ++i) {
      Eigen::Map<ArrayType>(m_dataPoints->mutablePoint(i), N, 1) = points[i];
    }
    m_kdTree = make_unique<ANNkd_tree>(m_dataPoints->rawData(), numPoints, N);
  }

  ~NearestNeighbours() {
    annClose();
  }

  NearestNeighbours(const NearestNeighbours&) = delete;

  /** Find the k nearest neighbours to a given point
   *
   * This is a thin wrapper around the ANN library annkSearch method
   *
   * @param pos :: the position to find th k nearest neighbours of
   * @param k :: the number of neighbours to find
   * @param error :: error term for finding approximate nearest neighbours. if
   * 	zero then exact neighbours will be found. (default = 0.0).
   * @return vector neighbours as tuples of (position, index, distance)
   */
  NearestNeighbourResults findNearest(const ArrayType& pos, const size_t k = 1, const double error = 0.0) {
    const auto numNeighbours = static_cast<int>(k);
    // create arrays to store the indices & distances of nearest neighbours
    auto nnIndexList = std::unique_ptr<ANNidx[]>(new ANNidx[numNeighbours]);
    auto nnDistList = std::unique_ptr<ANNdist[]>(new ANNdist[numNeighbours]);

    // create ANNpoint from Eigen array
    auto point = std::unique_ptr<ANNcoord[]>(annAllocPt(N));
    Eigen::Map<ArrayType>(point.get(), N, 1) = pos;

    // find the k nearest neighbours
    m_kdTree->annkSearch(point.get(), numNeighbours, nnIndexList.get(), nnDistList.get(), error);

    return makeResults(k, std::move(nnIndexList), std::move(nnDistList));
  }

private:

  /** Helper function to create a instance of NearestNeighbourResults
   *
   * @param k :: the number of neighbours searched for
   * @param nnIndexList :: the ordered list of indicies matching the closest k neighbours
   * @param nnDistList :: the ordered list of distances matching the closest k neighbours
   * @return a new NearestNeighbourResults object from the found items
   */
  NearestNeighbourResults makeResults(const size_t k, const std::unique_ptr<ANNidx[]> nnIndexList, const std::unique_ptr<ANNdist[]> nnDistList) {
    NearestNeighbourResults results;
    results.reserve(k);

    for(size_t i = 0; i < k; ++i) {
      // create Eigen array from ANNpoint
      auto pos = m_dataPoints->mutablePoint(nnIndexList[i]);
      ArrayType point = Eigen::Map<ArrayType>(pos, N, 1);
      auto item = std::make_tuple(point, nnIndexList[i], nnDistList[i]);
      results.push_back(item);
    }

    return results;
  }

  /// handle to the list of data points to search through
  std::unique_ptr<NNDataPoints> m_dataPoints;
  /// handle to the ANN KD-tree used for searching
  std::unique_ptr<ANNkd_tree> m_kdTree;
};

}
}

#endif
