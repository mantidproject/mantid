#ifndef MANTID_API_MATRIXWORKSPACEMDITERATOR_H_
#define MANTID_API_MATRIXWORKSPACEMDITERATOR_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace API {

/** IMDIterator-compatible implementation of an iterator
 * through a MatrixWorkspace

  @date 2012-02-08

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MatrixWorkspaceMDIterator : public IMDIterator {
public:
  MatrixWorkspaceMDIterator(const MatrixWorkspace *workspace,
                            Mantid::Geometry::MDImplicitFunction *function,
                            size_t beginWI = 0, size_t endWI = size_t(-1));
  size_t getDataSize() const override;

  bool valid() const override;

  bool next() override;

  bool next(size_t skip) override;

  void jumpTo(size_t index) override;

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

  uint16_t getInnerRunIndex(size_t index) const override;

  int32_t getInnerDetectorID(size_t index) const override;

  coord_t getInnerPosition(size_t index, size_t dimension) const override;

  signal_t getInnerSignal(size_t index) const override;

  signal_t getInnerError(size_t index) const override;

  bool getIsMasked() const override;

  std::vector<size_t> findNeighbourIndexes() const override;

  std::vector<size_t> findNeighbourIndexesFaceTouching() const override;

  size_t getLinearIndex() const override;

  bool isWithinBounds(size_t index) const override;

private:
  void calcWorkspacePos(size_t newWI);

  /// Workspace being iterated
  const MatrixWorkspace *m_ws;

  /// The linear position/index into the MDHistoWorkspace.
  uint64_t m_pos;

  /// The maximum linear index in the workspace
  uint64_t m_max;

  /// Implicit function to limit volume searched
  Mantid::Geometry::MDImplicitFunction *m_function;

  /// Workspace index of the spectrum we are looking at
  size_t m_workspaceIndex;

  /// x-index, index into the Y[] data array of the spectrum.
  size_t m_xIndex;

  /// Coordinates of the center at the current iterator pos
  mutable Mantid::Kernel::VMD m_center;

  /// Cached copies of X,Y,E at current workspace index
  MantidVec m_X;
  MantidVec m_Y;
  mutable MantidVec m_E;

  /// Error vector has been cached?
  mutable bool m_errorIsCached;

  /// Is the matrix workspace binned (true) e.g. Y vector is 1 shorter than X
  bool m_isBinnedData;

  /// The Y (vertical, e.g. spectra) dimension
  Mantid::Geometry::IMDDimension_const_sptr m_dimY;

  /// vector of starting index of the unraveled data array
  std::vector<size_t> m_startIndices;

  /// Workspace index at which the iterator begins
  size_t m_beginWI;

  /// Workspace index at which the iterator ends
  size_t m_endWI;

  /// For numeric axes, this is the size of the bin in the vertical direction.
  /// It is 1.0 for spectrum axes
  double m_verticalBinSize;

  /// SpectrumInfo object, used for masking information
  const SpectrumInfo &m_spectrumInfo;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_MATRIXWORKSPACEMDITERATOR_H_ */
