#ifndef MANTID_MDALGORITHMS_SLICINGALGORITHM_H_
#define MANTID_MDALGORITHMS_SLICINGALGORITHM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/CoordTransformAffine.h"

namespace Mantid {
namespace MDAlgorithms {

/** Abstract Algorithm class that will be used by:
 *    BinMD and SliceMD
 * and shares code for getting a slice from one workspace to another

  @author Janik Zikovsky
  @date 2011-09-27

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
class DLLExport SlicingAlgorithm : public API::Algorithm {
public:
  SlicingAlgorithm();
  ~SlicingAlgorithm();

  ///@return a string with the character that identifies each dimension in order
  ///(01234)
  static std::string getDimensionChars() { return "012345"; }

protected:
  /// Initialise the properties
  void initSlicingProps();

  void createTransform();

  void createGeneralTransform();
  void processGeneralTransformProperties();
  void createAlignedTransform();

  void makeAlignedDimensionFromString(const std::string &str);
  void makeBasisVectorFromString(const std::string &str);

  Mantid::Geometry::MDImplicitFunction *
  getImplicitFunctionForChunk(const size_t *const chunkMin,
                              const size_t *const chunkMax);
  Mantid::Geometry::MDImplicitFunction *
  getGeneralImplicitFunction(const size_t *const chunkMin,
                             const size_t *const chunkMax);

  /// Input workspace
  Mantid::API::IMDWorkspace_sptr m_inWS;

  /// Original (MDEventWorkspace) that inWS was based on. Used during basis
  /// vector constructor
  Mantid::API::IMDWorkspace_sptr m_originalWS;

  /** Bin dimensions to actually use. These are NEW dimensions created,
   * or copied (not pointing to) the original workspace. */
  std::vector<Mantid::Geometry::MDHistoDimension_sptr> m_binDimensions;

  /// Index of the dimension in the MDEW for the dimension in the output. Only
  /// for axis-aligned slices
  std::vector<size_t> m_dimensionToBinFrom;

  /// Coordinate transformation to apply. This transformation
  /// contains the scaling that makes the output coordinate = bin indexes in the
  /// output MDHistoWorkspace.
  Mantid::API::CoordTransform *m_transform;

  /// Coordinate transformation to save in the output workspace
  /// (original->binned)
  Mantid::API::CoordTransform *m_transformFromOriginal;
  /// Coordinate transformation to save in the output workspace
  /// (binned->original)
  Mantid::API::CoordTransform *m_transformToOriginal;

  /// Intermediate original workspace. Output -> intermediate (MDHisto) ->
  /// original (MDEvent)
  Mantid::API::IMDWorkspace_sptr m_intermediateWS;
  /// Coordinate transformation to save in the output WS, from the intermediate
  /// WS
  Mantid::DataObjects::CoordTransformAffine *m_transformFromIntermediate;
  /// Coordinate transformation to save in the intermediate WS
  Mantid::DataObjects::CoordTransformAffine *m_transformToIntermediate;

  /// Set to true if the cut is aligned with the axes
  bool m_axisAligned;

  /// Number of dimensions in the output (binned) workspace.
  size_t m_outD;

  /// Basis vectors of the output dimensions, normalized to unity length
  std::vector<Mantid::Kernel::VMD> m_bases;

  /// Scaling factor to apply for each basis vector (to map to the bins).
  /// i.e.. MULTIPLY the distance in the INPUT workspace by this = an index into
  /// the bin
  std::vector<double> m_binningScaling;

  /// Scaling factor to apply for each basis vector to transfor to the output
  /// dimensions.
  /// i.e.. MULTIPLY the distance in the INPUT workspace by this = a distance in
  /// the OUTPUT dimension
  std::vector<double> m_transformScaling;

  /// Translation from the OUTPUT to the INPUT workspace
  /// i.e. this position in the input workspace = 0,0,0 in the output.
  Mantid::Kernel::VMD m_translation;

  /// Coordinates in the INPUT workspace corresponding to the
  /// minimum edge in all dimensions.
  Mantid::Kernel::VMD m_inputMinPoint;

  /// For non-aligned, the minimum coordinate extents in each OUTPUT dimension
  std::vector<double> m_minExtents;

  /// For non-aligned, the maximum coordinate extents in each OUTPUT dimension
  std::vector<double> m_maxExtents;

  /// For non-aligned, the number of bins in each OUTPUT dimension.
  std::vector<int> m_numBins;

  /// The NormalizeBasisVectors option
  bool m_NormalizeBasisVectors;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SLICINGALGORITHM_H_ */
