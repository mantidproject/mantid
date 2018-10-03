// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_SLICINGALGORITHM_H_
#define MANTID_MDALGORITHMS_SLICINGALGORITHM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** Abstract Algorithm class that will be used by:
 *    BinMD and SliceMD
 * and shares code for getting a slice from one workspace to another

  @author Janik Zikovsky
  @date 2011-09-27
*/
class DLLExport SlicingAlgorithm : public API::Algorithm {
public:
  SlicingAlgorithm();

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

private:
  Mantid::Geometry::MDFrame_uptr
  createMDFrameForNonAxisAligned(std::string units,
                                 Mantid::Kernel::VMD basisVector) const;
  std::vector<Mantid::Kernel::VMD> getOldBasis(size_t dimension) const;
  bool isProjectingOnFrame(const Mantid::Kernel::VMD &oldVector,
                           const Mantid::Kernel::VMD &basisVector) const;
  std::vector<size_t> getIndicesWithProjection(
      const Mantid::Kernel::VMD &basisVector,
      const std::vector<Mantid::Kernel::VMD> &oldBasis) const;
  Mantid::Geometry::MDFrame_uptr
  extractMDFrameForNonAxisAligned(std::vector<size_t> indicesWithProjection,
                                  std::string units) const;
  void setTargetUnits(Mantid::Geometry::MDFrame_uptr &frame,
                      const std::string &units) const;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SLICINGALGORITHM_H_ */
