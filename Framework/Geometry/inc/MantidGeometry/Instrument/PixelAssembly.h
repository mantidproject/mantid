// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/IObject.h"
#include <string>
#include <tuple>
#include <vector>

namespace Mantid {
namespace Geometry {

class ComponentVisitor;
/**
 *  GridDetector is a type of CompAssembly, an assembly of components.
 *  It is designed to be an easy way to specify a 3D grid (XYZ) array of
 *  Detector pixels. Ragged grids are not allowed, pixels are uniform in each
 *  dimension.
 *
 * @class PixelAssembly
 * @brief Assembly of Detector objects in a 3D grid shape
 * @author Lamar Moore, ISIS
 * @date 2018-Jul-24
 */

class MANTID_GEOMETRY_DLL PixelAssembly : public CompAssembly, public IObjComponent {

public:
  /// String description of the type of component
  std::string type() const override { return "PixelAssembly"; }

  //! Constructor with a name and parent reference
  PixelAssembly(const std::string &name, IComponent *reference = nullptr);

  //! Parametrized constructor
  PixelAssembly(const PixelAssembly *base, const ParameterMap *map);

  /// Matches name to Structured Detector
  static bool compareName(const std::string &proposedMatch);

  /// Create all the detector pixels of this grid detector.
  void initialize(std::shared_ptr<IObject> shape, int xpixels, double xstart, double xstep, int ypixels, double ystart,
                  double ystep, int zpixels, double zstart, double zstep, int idstart, const std::string &idFillOrder,
                  int idstepbyrow, int idstep = 1);

  //! Make a clone of the present component
  PixelAssembly *clone() const override;

  int idstart() const;
  int idstep() const;

  /// minimum detector id
  detid_t minDetectorID() const;
  /// maximum detector id
  detid_t maxDetectorID() const;
  std::shared_ptr<const IComponent> getComponentByName(const std::string &cname, int nlevels = 0) const override;

  // This should inherit the getBoundingBox implementation from  CompAssembly
  // but the multiple inheritance seems to confuse it so we'll explicityly tell
  // it that here
  using CompAssembly::getBoundingBox;

  void testIntersectionWithChildren(Track &testRay, std::deque<IComponent_const_sptr> &searchQueue) const override;

  // ------------ IObjComponent methods ----------------

  /// Does the point given lie within this object component?
  bool isValid(const Kernel::V3D &point) const override;

  /// Does the point given lie on the surface of this object component?
  bool isOnSide(const Kernel::V3D &point) const override;

  /// Checks whether the track given will pass through this Component.
  int interceptSurface(Track &track) const override;

  /// Finds the approximate solid angle covered by the component when viewed
  /// from the point given
  double solidAngle(const Geometry::SolidAngleParams &params) const override;
  /// Retrieve the cached bounding box
  void getBoundingBox(BoundingBox &assemblyBox) const override;

  /// Try to find a point that lies within (or on) the object
  int getPointInObject(Kernel::V3D &point) const override;

  // Rendering member functions
  /// Draws the objcomponent.
  void draw() const override;

  /// Draws the Object.
  void drawObject() const override;

  /// Initializes the ObjComponent for rendering, this function should be called
  /// before rendering.
  void initDraw() const override;

  /// Returns the shape of the Object
  const std::shared_ptr<const IObject> shape() const override;
  /// Returns the material of the detector
  const Kernel::Material material() const override;

  virtual size_t registerContents(class ComponentVisitor &componentVisitor) const override;

  // ------------ End of IObjComponent methods ----------------
protected:
  /// initialize members to bare defaults
  void init();
  void createLayer(const std::string &name, CompAssembly *parent, int iz, int &minDetID, int &maxDetID);

private:
  void initializeValues(std::shared_ptr<IObject> shape, int xpixels, double xstart, double xstep, int ypixels,
                        double ystart, double ystep, int zpixels, double zstart, double zstep, int idstart,
                        const std::string &idFillOrder, int idstepbyrow, int idstep);

  void validateInput() const;
  /// Pointer to the base GridDetector, for parametrized instruments
  const GridDetector *m_gridBase;
  bool isParametrized() const override { return m_map && m_gridBase; }
  /// Private copy assignment operator
  PixelAssembly &operator=(const ICompAssembly &);

  /// Pointer to the shape of the pixels in this detector array.
  std::shared_ptr<IObject> m_shape;
  /// minimum detector id
  detid_t m_minDetId;
  /// maximum detector id
  detid_t m_maxDetId;

  /// IDs start here
  int m_idstart;
  /// Step size in ID in each col
  int m_idstep;
};

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &, const PixelAssembly &);

using PixelAssembly_sptr = std::shared_ptr<PixelAssembly>;
using PixelAssembly_const_sptr = std::shared_ptr<const PixelAssembly>;

} // Namespace Geometry
} // Namespace Mantid
