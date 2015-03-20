#ifndef MANTID_MDEVENTS_MDHISTOWORKSPACEITERATOR_H_
#define MANTID_MDEVENTS_MDHISTOWORKSPACEITERATOR_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidMDEvents/SkippingPolicy.h"
#include <set>

namespace Mantid {
namespace MDEvents {

/** An implementation of IMDIterator that iterates through
  a MDHistoWorkspace. It treats the bin in the workspace as
  a box containing a single "event", at the center of each bin
  and with the proper signal/error.

  @author Janik Zikovsky
  @date 2011-10-06

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport MDHistoWorkspaceIterator : public Mantid::API::IMDIterator {
public:
  MDHistoWorkspaceIterator(
      MDHistoWorkspace_const_sptr workspace, SkippingPolicy *skippingPolicy,
      Mantid::Geometry::MDImplicitFunction *function = NULL,
      size_t beginPos = 0, size_t endPos = size_t(-1));
  MDHistoWorkspaceIterator(
      const MDHistoWorkspace *workspace, SkippingPolicy *skippingPolicy,
      Mantid::Geometry::MDImplicitFunction *function = NULL,
      size_t beginPos = 0, size_t endPos = size_t(-1));
  MDHistoWorkspaceIterator(
      MDHistoWorkspace_const_sptr workspace,
      Mantid::Geometry::MDImplicitFunction *function = NULL,
      size_t beginPos = 0, size_t endPos = size_t(-1));
  MDHistoWorkspaceIterator(
      const MDHistoWorkspace *workspace,
      Mantid::Geometry::MDImplicitFunction *function = NULL,
      size_t beginPos = 0, size_t endPos = size_t(-1));
  virtual ~MDHistoWorkspaceIterator();

  void init(const MDHistoWorkspace *workspace,
            Mantid::Geometry::MDImplicitFunction *function, size_t beginPos = 0,
            size_t endPos = size_t(-1));

  virtual size_t getDataSize() const;

  virtual bool valid() const;

  virtual bool next();

  virtual bool next(size_t skip);

  virtual void jumpTo(size_t index);

  virtual signal_t getNormalizedSignal() const;

  virtual signal_t getNormalizedError() const;

  virtual signal_t getSignal() const;

  virtual signal_t getError() const;

  virtual coord_t *getVertexesArray(size_t &numVertices) const;

  virtual coord_t *getVertexesArray(size_t &numVertices,
                                    const size_t outDimensions,
                                    const bool *maskDim) const;

  virtual Mantid::Kernel::VMD getCenter() const;

  virtual size_t getNumEvents() const;

  virtual uint16_t getInnerRunIndex(size_t index) const;

  virtual int32_t getInnerDetectorID(size_t index) const;

  virtual coord_t getInnerPosition(size_t index, size_t dimension) const;

  virtual signal_t getInnerSignal(size_t index) const;

  virtual signal_t getInnerError(size_t index) const;

  virtual bool getIsMasked() const;

  virtual size_t getLinearIndex() const;

  std::vector<size_t> findNeighbourIndexes() const;

  std::vector<size_t> findNeighbourIndexesFaceTouching() const;

  std::vector<size_t> findNeighbourIndexesByWidth(const int width) const;

  virtual bool isWithinBounds(size_t index) const;

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
  Mantid::Geometry::MDImplicitFunction *m_function;

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

  /// Neighbour finding permutations.
  mutable std::vector<int64_t> m_permutationsVertexTouching;
  mutable std::vector<int64_t> m_permutationsFaceTouching;

  /// Skipping policy.
  SkippingPolicy_scptr m_skippingPolicy;
};

} // namespace MDEvents
} // namespace Mantid

#endif /* MANTID_MDEVENTS_MDHISTOWORKSPACEITERATOR_H_ */
