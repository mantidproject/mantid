#ifndef GRID_DETECTOR_H
#define GRID_DETECTOR_H
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
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
*  Detector pixels.
*
* @class GridDetector
* @brief Assembly of Detector objects in a 3D grid shape
* @author Lamar Moore, ISIS
* @date 2018-Jul-24

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MANTID_GEOMETRY_DLL GridDetector : public CompAssembly,
                                         public IObjComponent {
  friend class GridDetectorPixel;

public:
  /// String description of the type of component
  std::string type() const override { return "GridDetector"; }
  //! Empty constructor
  GridDetector();

  //! Constructor with a name and parent reference
  GridDetector(const std::string &name, IComponent *reference = nullptr);

  //! Parametrized constructor
  GridDetector(const GridDetector *base, const ParameterMap *map);

  /// Matches name to Structured Detector
  static bool compareName(const std::string &proposedMatch);

  /// Create all the detector pixels of this grid detector.
  void initialize(boost::shared_ptr<IObject> shape, int xpixels, double xstart,
                  double xstep, int ypixels, double ystart, double ystep,
                  int zpixels, double zstart, double zstep, int idstart,
                  const std::string &idFillOrder, int idstepbyrow,
                  int idstep = 1);

  //! Make a clone of the present component
  IComponent *clone() const override;

  boost::shared_ptr<Detector> getAtXYZ(const int x, const int y,
                                       const int z) const;

  detid_t getDetectorIDAtXYZ(const int x, const int y, const int z) const;
  std::tuple<int, int, int> getXYZForDetectorID(const int detectorID) const;

  int xpixels() const;
  int ypixels() const;
  int zpixels() const;

  double xstep() const;
  double ystep() const;
  double zstep() const;

  double xstart() const;
  double ystart() const;
  double zstart() const;

  /// Size in X of the detector
  double xsize() const;
  /// Size in Y of the detector
  double ysize() const;
  /// Size in Z of the detector
  double zsize() const;

  int idstart() const;
  bool idfillbyfirst_y() const;
  const std::string &idFillOrder() const;
  int idstepbyrow() const;
  int idstep() const;

  Kernel::V3D getRelativePosAtXYZ(int x, int y, int z) const;
  /// minimum detector id
  int minDetectorID();
  /// maximum detector id
  int maxDetectorID();
  boost::shared_ptr<const IComponent>
  getComponentByName(const std::string &cname, int nlevels = 0) const override;

  // This should inherit the getBoundingBox implementation from  CompAssembly
  // but
  // the multiple inheritance seems to confuse it so we'll explicityly tell it
  // that here
  using CompAssembly::getBoundingBox;

  void testIntersectionWithChildren(
      Track &testRay,
      std::deque<IComponent_const_sptr> &searchQueue) const override;

  // ------------ IObjComponent methods ----------------

  /// Does the point given lie within this object component?
  bool isValid(const Kernel::V3D &point) const override;

  /// Does the point given lie on the surface of this object component?
  bool isOnSide(const Kernel::V3D &point) const override;

  /// Checks whether the track given will pass through this Component.
  int interceptSurface(Track &track) const override;

  /// Finds the approximate solid angle covered by the component when viewed
  /// from the point given
  double solidAngle(const Kernel::V3D &observer) const override;
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
  const boost::shared_ptr<const IObject> shape() const override;
  /// Returns the material of the detector
  const Kernel::Material material() const override;

  virtual size_t
  registerContents(class ComponentVisitor &componentVisitor) const override;

  // ------------ End of IObjComponent methods ----------------
protected:
  /// initialize members to bare defaults
  void init();
  void createLayer(const std::string &name, CompAssembly *parent, int iz,
                   int &minDetID, int &maxDetID);

private:
  void initializeValues(boost::shared_ptr<IObject> shape, int xpixels,
                        double xstart, double xstep, int ypixels, double ystart,
                        double ystep, int zpixels, double zstart, double zstep,
                        int idstart, const std::string &idFillOrder,
                        int idstepbyrow, int idstep);

  void validateInput() const;
  /// Pointer to the base GridDetector, for parametrized instruments
  const GridDetector *m_gridBase;
  /// Private copy assignment operator
  GridDetector &operator=(const ICompAssembly &);


  /// The number of pixels in the X (horizontal) direction
  int m_xpixels;
  /// The number of pixels in the Y (vertical) direction
  int m_ypixels;
  /// The number of pixels in the Z (usually beam) direction
  int m_zpixels;

  /// Size in X of the detector
  double m_xsize;
  /// Size in Y of the detector
  double m_ysize;
  /// Size in Z of the detector
  double m_zsize;

  /// X position of the 0-th pixel
  double m_xstart;
  /// Y position of the 0-th pixel
  double m_ystart;
  /// Z position of the 0-th pixel
  double m_zstart;

  /// Step size in the X direction of detector
  double m_xstep;
  /// Step size in the Y direction of detector
  double m_ystep;
  /// Step size in the Z direction of the detector
  double m_zstep;

  /// Pointer to the shape of the pixels in this detector array.
  boost::shared_ptr<IObject> m_shape;
  /// minimum detector id
  int m_minDetId;
  /// maximum detector id
  int m_maxDetId;

  /// IDs start here
  int m_idstart;
  /// IDs are filled in Y fastest
  bool m_idfillbyfirst_y;
  /// The order in which to fill IDs
  std::string m_idFillOrder;
  /// Step size in ID in each row
  int m_idstepbyrow;
  /// Step size in ID in each col
  int m_idstep;
};

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &,
                                             const GridDetector &);

using GridDetector_sptr = boost::shared_ptr<GridDetector>;
using GridDetector_const_sptr = boost::shared_ptr<const GridDetector>;

} // Namespace Geometry
} // Namespace Mantid

#endif // GRID_DETECTOR_H
