// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ANN/ANN.h"
#include "MantidKernel/DllConfig.h"

#include <Eigen/Core>
#include <memory>
#include <vector>

/**
  NearestNeighbours is a thin wrapper class around the ANN library for finding
  the k nearest neighbours.

  Given a vector of Eigen::Vectors this class will generate a KDTree. The tree
  can then be interrogated to find the closest k neighbours to a given position.

  This classes is templated with a parameter N which defines the dimensionality
  of the vector type used. i.e. if N = 3 then Eigen::Vector3d is used.

  @author Samuel Jackson
  @date 2017
*/

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
  NNDataPoints(const int nPts, const int nElems) : m_nPts(nPts) { m_data = annAllocPts(m_nPts, nElems); }

  ~NNDataPoints() { annDeallocPts(m_data); }

  /** Return a handle to the raw ANNpointArray wrapped by this class
   *
   * @return handle to the raw ANNpointArray
   */
  ANNpointArray rawData() { return m_data; }

  /** Access a raw point in the collection of points
   *
   * This will check the index used is within bounds and return nullptr if
   * outside of those bounds
   *
   * @param i :: the index of the point to return a handle to
   * @return handle to a single point in the collection of points
   */
  ANNcoord *mutablePoint(const int i) {
    if (i < m_nPts)
      return m_data[i];
    else
      return nullptr;
  }

private:
  /// Number of points stored
  const int m_nPts;
  /// Array of points for use with NN search
  ANNpointArray m_data;
};

//------------------------------------------------------------------------------
// NearestNeighbours implementation
//------------------------------------------------------------------------------

template <int N = 3> class MANTID_KERNEL_DLL NearestNeighbours {

public:
  // typedefs for code brevity
  using VectorType = Eigen::Matrix<double, N, 1>;
  using NearestNeighbourResults = std::vector<std::tuple<VectorType, size_t, double>>;

  /** Create a nearest neighbour search object
   *
   * @param points :: vector of Eigen::Vectors to search through
   */
  NearestNeighbours(const std::vector<VectorType> &points) {
    const auto numPoints = static_cast<int>(points.size());
    if (numPoints == 0)
      throw std::runtime_error("Need at least one point to initialise NearestNeighbours.");

    m_dataPoints = std::make_unique<NNDataPoints>(numPoints, static_cast<int>(N));

    for (size_t i = 0; i < points.size(); ++i) {
      Eigen::Map<VectorType>(m_dataPoints->mutablePoint(static_cast<int>(i)), N, 1) = points[i];
    }
    m_kdTree = std::make_unique<ANNkd_tree>(m_dataPoints->rawData(), numPoints, static_cast<int>(N));
  }

  ~NearestNeighbours() { annClose(); }

  NearestNeighbours(const NearestNeighbours &) = delete;

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
  NearestNeighbourResults findNearest(const VectorType &pos, const size_t k = 1, const double error = 0.0) {
    const auto numNeighbours = static_cast<int>(k);
    // create arrays to store the indices & distances of nearest neighbours
    auto nnIndexList = std::unique_ptr<ANNidx[]>(new ANNidx[numNeighbours]);
    auto nnDistList = std::unique_ptr<ANNdist[]>(new ANNdist[numNeighbours]);

    // create ANNpoint from Eigen array
    auto point = std::unique_ptr<ANNcoord[]>(annAllocPt(N));
    Eigen::Map<VectorType>(point.get(), N, 1) = pos;

    // find the k nearest neighbours
    m_kdTree->annkSearch(point.get(), numNeighbours, nnIndexList.get(), nnDistList.get(), error);

    return makeResults(k, std::move(nnIndexList), std::move(nnDistList));
  }

private:
  /** Helper function to create a instance of NearestNeighbourResults
   *
   * @param k :: the number of neighbours searched for
   * @param nnIndexList :: the ordered list of indicies matching the closest k
   *neighbours
   * @param nnDistList :: the ordered list of distances matching the closest k
   *neighbours
   * @return a new NearestNeighbourResults object from the found items
   */
  NearestNeighbourResults makeResults(const size_t k, const std::unique_ptr<ANNidx[]> nnIndexList,
                                      const std::unique_ptr<ANNdist[]> nnDistList) {
    NearestNeighbourResults results;
    results.reserve(k);

    for (size_t i = 0; i < k; ++i) {
      // create Eigen array from ANNpoint
      auto pos = m_dataPoints->mutablePoint(nnIndexList[i]);
      VectorType point = Eigen::Map<VectorType>(pos, N, 1);
      results.emplace_back(point, nnIndexList[i], nnDistList[i]);
    }

    return results;
  }

  /// handle to the list of data points to search through
  std::unique_ptr<NNDataPoints> m_dataPoints;
  /// handle to the ANN KD-tree used for searching
  std::unique_ptr<ANNkd_tree> m_kdTree;
};
} // namespace Kernel
} // namespace Mantid
