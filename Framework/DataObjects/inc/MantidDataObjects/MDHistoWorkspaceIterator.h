// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_MDHISTOWORKSPACEITERATOR_H_
#define MANTID_DATAOBJECTS_MDHISTOWORKSPACEITERATOR_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/SkippingPolicy.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include <boost/tuple/tuple.hpp>
#include <map>
#include <vector>

namespace Mantid {
namespace DataObjects {

// Typedef for a map for mapping width of neighbours (key) to permutations
// needed in the calcualtion.
using PermutationsMap = std::map<std::vector<int>, std::vector<int64_t>>;
// Typedef for extents
using MDExtentPair =
    boost::tuple<Mantid::coord_t, Mantid::coord_t>; // Min/Max pair
// Typedef for vector of extents
using VecMDExtents = std::vector<MDExtentPair>;

/** An implementation of IMDIterator that iterates through
  a MDHistoWorkspace. It treats the bin in the workspace as
  a box containing a single "event", at the center of each bin
  and with the proper signal/error.

  @author Janik Zikovsky
  @date 2011-10-06
*/
class DLLExport MDHistoWorkspaceIterator : public Mantid::API::IMDIterator {
public:
  MDHistoWorkspaceIterator(
      MDHistoWorkspace_const_sptr workspace, SkippingPolicy *skippingPolicy,
      Mantid::Geometry::MDImplicitFunction *function = nullptr,
      size_t beginPos = 0, size_t endPos = size_t(-1));
  MDHistoWorkspaceIterator(
      const MDHistoWorkspace *workspace, SkippingPolicy *skippingPolicy,
      Mantid::Geometry::MDImplicitFunction *function = nullptr,
      size_t beginPos = 0, size_t endPos = size_t(-1));
  MDHistoWorkspaceIterator(
      MDHistoWorkspace_const_sptr workspace,
      Mantid::Geometry::MDImplicitFunction *function = nullptr,
      size_t beginPos = 0, size_t endPos = size_t(-1));
  MDHistoWorkspaceIterator(
      const MDHistoWorkspace *workspace,
      Mantid::Geometry::MDImplicitFunction *function = nullptr,
      size_t beginPos = 0, size_t endPos = size_t(-1));
  ~MDHistoWorkspaceIterator() override;

  void init(const MDHistoWorkspace *workspace,
            Mantid::Geometry::MDImplicitFunction *function, size_t beginPos = 0,
            size_t endPos = size_t(-1));

  size_t getDataSize() const override;

  bool valid() const override;

  bool next() override;

  bool next(size_t skip) override;

  void jumpTo(size_t index) override;

  virtual coord_t jumpToNearest(const Mantid::Kernel::VMD &fromLocation);

  signal_t getNormalizedSignal() const override;

  signal_t getNormalizedError() const override;

  signal_t getSignal() const override;

  signal_t getError() const override;

  std::unique_ptr<coord_t[]>
  getVertexesArray(size_t &numVertices) const override;

  std::unique_ptr<coord_t[]>
  getVertexesArray(size_t &numVertices, const size_t outDimensions,
                   const bool *maskDim) const override;

  Mantid::Kernel::VMD getCenter() const override;

  size_t getNumEvents() const override;

  virtual signal_t getNumEventsFraction() const;

  uint16_t getInnerRunIndex(size_t index) const override;

  int32_t getInnerDetectorID(size_t index) const override;

  coord_t getInnerPosition(size_t index, size_t dimension) const override;

  signal_t getInnerSignal(size_t index) const override;

  signal_t getInnerError(size_t index) const override;

  bool getIsMasked() const override;

  size_t getLinearIndex() const override;

  std::vector<size_t> findNeighbourIndexes() const override;

  std::vector<size_t> findNeighbourIndexesFaceTouching() const override;

  std::vector<size_t> findNeighbourIndexesByWidth(const int &width) const;

  std::pair<std::vector<size_t>, std::vector<bool>>
  findNeighbourIndexesByWidth1D(const int &width,
                                const int &width_dimension) const;

  std::vector<size_t>
  findNeighbourIndexesByWidth(const std::vector<int> &widths) const;

  bool isWithinBounds(size_t index) const override;

  size_t permutationCacheSize() const;

  VecMDExtents getBoxExtents() const;

protected:
  /// The MDHistoWorkspace being iterated.
  const MDHistoWorkspace *m_ws;

  /// The linear position/index into the MDHistoWorkspace.
  uint64_t m_pos;

  /// The beginning linear index in the workspace
  uint64_t m_begin;

  /// The maximum linear index in the workspace
  uint64_t m_max;

  /// Implicit function to limit volume searched
  std::unique_ptr<Mantid::Geometry::MDImplicitFunction> m_function;

  /// Number of dimensions
  size_t m_nd;

  /// Center of the current box. Not set until getCenter() is called.
  coord_t *m_center;

  /// Origin (index 0,0,0) in the space = the minimum of each dimension
  coord_t *m_origin;

  /// Width of each bin in each dimension
  coord_t *m_binWidth;

  /// Index into each dimension
  size_t *m_index;

  /// Index into each dimension
  size_t *m_indexMax;

  /// Array to find indices from linear indices
  size_t *m_indexMaker;

  /// Neigbour finding permutations for face touching neighbours (3 by 3 width).
  mutable std::vector<int64_t> m_permutationsFaceTouching;

  /// Neighbour finding permutations map for vertex touching. Keyed via the
  /// width (n-pixels) of neighbours required.
  mutable PermutationsMap m_permutationsVertexTouchingMap;

  /// Skipping policy.
  SkippingPolicy_scptr m_skippingPolicy;

  /// Create or fetch permutations relating to a given neighbour width.
  std::vector<int64_t> createPermutations(const std::vector<int> &widths) const;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_MDHISTOWORKSPACEITERATOR_H_ */
