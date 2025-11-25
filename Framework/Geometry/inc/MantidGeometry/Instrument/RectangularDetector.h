// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/GridDetector.h"
#include "MantidGeometry/Objects/IObject.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Geometry {

class ComponentVisitor;
/**
 *  RectangularDetector is a type of CompAssembly, an assembly of components.
 *  It is designed to be an easy way to specify a rectangular (XY) array of
 *  Detector pixels.
 *
 * @class RectangularDetector
 * @brief Assembly of Detector objects in a rectangular shape
 * @author Janik Zikovsky, SNS
 * @date 2010-Oct-06
 */

class MANTID_GEOMETRY_DLL RectangularDetector : public GridDetector {
public:
  /// String description of the type of component
  std::string type() const override { return "RectangularDetector"; }
  //! Empty constructor
  RectangularDetector();

  //! Constructor with a name and parent reference
  RectangularDetector(const std::string &, IComponent *reference = nullptr);

  //! Parametrized constructor
  RectangularDetector(const RectangularDetector *base, const ParameterMap *map);

  /// Matches name to Structured Detector
  static bool compareName(const std::string &proposedMatch);

  /// Create all the detector pixels of this rectangular detector.
  void initialize(std::shared_ptr<IObject> shape, int xpixels, double xstart, double xstep, int ypixels, double ystart,
                  double ystep, int idstart, bool idfillbyfirst_y, int idstepbyrow, int idstep = 1);

  //! Make a clone of the present component
  RectangularDetector *clone() const override;

  std::shared_ptr<Detector> getAtXY(const int X, const int Y) const;

  detid_t getDetectorIDAtXY(const int X, const int Y) const;
  std::pair<int, int> getXYForDetectorID(const int detectorID) const;

  Kernel::V3D getRelativePosAtXY(int x, int y) const;
  void getTextureSize(int &xsize, int &ysize) const;

  unsigned int getTextureID() const;
  void setTextureID(unsigned int textureID);

  // This should inherit the getBoundingBox implementation from  CompAssembly
  // but
  // the multiple inheritance seems to confuse it so we'll explicityly tell it
  // that here
  using CompAssembly::getBoundingBox;

  void testIntersectionWithChildren(Track &testRay, std::deque<IComponent_const_sptr> &searchQueue) const override;

  // ------------ IObjComponent methods ----------------

  /// Returns the material of the detector
  const Kernel::Material material() const override;

  virtual size_t registerContents(class ComponentVisitor &componentVisitor) const override;

  // ------------ End of IObjComponent methods ----------------

private:
  /// Private copy assignment operator
  RectangularDetector &operator=(const ICompAssembly &);

  /// Texture ID to use in rendering
  unsigned int m_textureID;
};

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &, const RectangularDetector &);

using RectangularDetector_sptr = std::shared_ptr<RectangularDetector>;
using RectangularDetector_const_sptr = std::shared_ptr<const RectangularDetector>;

} // Namespace Geometry
} // Namespace Mantid
