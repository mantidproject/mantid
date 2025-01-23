// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/VMD.h"
#include <memory>

#include <memory>

namespace Mantid {
namespace Geometry {
class IMDDimension;
}
namespace API {
class CoordTransform;
class IMDWorkspace;
class MDGeometryNotificationHelper;
class Workspace;

/** Describes the geometry (i.e. dimensions) of an IMDWorkspace.
 * This defines the dimensions contained in the workspace.
 * On option, it can also relate the coordinates of this workspace
 * to another workspace, e.g. if a workspace is a slice or a view
 * onto an original workspace.

  @author Janik Zikovsky
  @date 2011-09-06
*/
class MANTID_API_DLL MDGeometry {
public:
  MDGeometry();
  MDGeometry(const MDGeometry &other);
  MDGeometry &operator=(const MDGeometry &other);
  virtual ~MDGeometry();
  void initGeometry(const std::vector<std::shared_ptr<Geometry::IMDDimension>> &dimensions);

  // --------------------------------------------------------------------------------------------
  // These are the main methods for dimensions, that CAN be overridden (e.g. by
  // MatrixWorkspace)
  virtual size_t getNumDims() const;
  virtual size_t getNumNonIntegratedDims() const;
  virtual std::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(size_t index) const;
  virtual std::shared_ptr<const Mantid::Geometry::IMDDimension> getDimensionWithId(std::string id) const;
  size_t getDimensionIndexByName(const std::string &name) const;
  size_t getDimensionIndexById(const std::string &id) const;
  std::vector<std::shared_ptr<const Geometry::IMDDimension>> getNonIntegratedDimensions() const;
  virtual std::vector<coord_t> estimateResolution() const;

  // --------------------------------------------------------------------------------------------
  std::shared_ptr<const Mantid::Geometry::IMDDimension> getXDimension() const;
  std::shared_ptr<const Mantid::Geometry::IMDDimension> getYDimension() const;
  std::shared_ptr<const Mantid::Geometry::IMDDimension> getZDimension() const;
  std::shared_ptr<const Mantid::Geometry::IMDDimension> getTDimension() const;

  std::string getGeometryXML() const;

  void addDimension(const std::shared_ptr<Mantid::Geometry::IMDDimension> &dim);
  void addDimension(Mantid::Geometry::IMDDimension *dim);

  // --------------------------------------------------------------------------------------------
  Mantid::Kernel::VMD &getBasisVector(size_t index);
  const Mantid::Kernel::VMD &getBasisVector(size_t index) const;
  void setBasisVector(size_t index, const Mantid::Kernel::VMD &vec);
  bool allBasisNormalized() const;

  // --------------------------------------------------------------------------------------------
  bool hasOriginalWorkspace(size_t index = 0) const;
  size_t numOriginalWorkspaces() const;
  std::shared_ptr<Workspace> getOriginalWorkspace(size_t index = 0) const;
  void setOriginalWorkspace(std::shared_ptr<Workspace> ws, size_t index = 0);
  Mantid::API::CoordTransform const *getTransformFromOriginal(size_t index = 0) const;
  void setTransformFromOriginal(Mantid::API::CoordTransform *transform, size_t index = 0);
  Mantid::API::CoordTransform const *getTransformToOriginal(size_t index = 0) const;
  void setTransformToOriginal(Mantid::API::CoordTransform *transform, size_t index = 0);

  void transformDimensions(std::vector<double> &scaling, std::vector<double> &offset);

  size_t getNumberTransformsToOriginal() const;
  size_t getNumberTransformsFromOriginal() const;

  // --------------------------------------------------------------------------------------------
  ///@return the vector of the origin (in the original workspace) that
  /// corresponds to 0,0,0... in this workspace
  Mantid::Kernel::VMD &getOrigin() { return m_origin; }

  ///@return the vector of the origin (in the original workspace) that
  /// corresponds to 0,0,0... in this workspace
  const Mantid::Kernel::VMD &getOrigin() const { return m_origin; }

  /// Sets the origin of this geometry.
  ///@param orig :: the vector of the origin (in the original workspace) that
  /// corresponds to 0,0,0... in this workspace
  void setOrigin(const Mantid::Kernel::VMD &orig) { m_origin = orig; }
  /// set the transformation from Q in crystal cartezian coordinate system to Q
  /// in orthogonal or real HKL coordiate system alined with arbitrary slicing
  /// plane
  void setWTransf(const Kernel::DblMatrix &WTransf) { m_Wtransf = WTransf; }
  /// get the transformation from Qin crystal cartezian coordinate system to Q
  /// in orthogonal or real HKL coordiate system alined with arbitrary slicing
  /// plane
  const Kernel::DblMatrix &getWTransf() const { return m_Wtransf; }

  /// Clear transforms
  void clearTransforms();
  /// Clear original workspaces
  void clearOriginalWorkspaces();

  friend class MDGeometryNotificationHelper;

protected:
  /// Function called when observer objects recieves a notification
  void deleteNotificationReceived(const std::shared_ptr<const Workspace> &replaced);

  /// Function called when observer detects a workspace is replaced
  void replaceNotificationReceived(const std::shared_ptr<const Workspace> &deleted);

  /// Vector of the dimensions used, in the order X Y Z t, etc.
  std::vector<std::shared_ptr<Geometry::IMDDimension>> m_dimensions;

  /// Pointer to the original workspace(s), if this workspace is a coordinate
  /// transformation from an original workspace.
  std::vector<std::shared_ptr<Workspace>> m_originalWorkspaces;

  /// Vector of the origin (in the original workspace) that corresponds to
  /// 0,0,0... in this workspace
  Mantid::Kernel::VMD m_origin;

  /// Coordinate Transformation that goes from the original workspace to this
  /// workspace's coordinates.
  std::vector<std::shared_ptr<const Mantid::API::CoordTransform>> m_transforms_FromOriginal;

  /// Coordinate Transformation that goes from this workspace's coordinates to
  /// the original workspace coordinates.
  std::vector<std::shared_ptr<const Mantid::API::CoordTransform>> m_transforms_ToOriginal;

  /// Helper that deals with notifications and observing the ADS
  std::unique_ptr<MDGeometryNotificationHelper> m_notificationHelper;

  /** the matrix which transforms momentums from orthogonal Q-system to
     Orthogonal HKL or non-orthogonal HKL system alighned WRT to arbitrary
     coordinate system requested
     See the UB matrix formalizm, pg 7 for further details on implementation.
     Small variation here is that in also includes either B-matrix or orthogonal
     B-matrix */
  Kernel::DblMatrix m_Wtransf; // TODO: should be reconciled with the vectors
                               // below and be either always synchroneous or
                               // only one set remains
  /// Vector of the basis vector (in the original workspace) for each dimension
  /// of this workspace.
  std::vector<Mantid::Kernel::VMD> m_basisVectors;
};

} // namespace API
} // namespace Mantid
