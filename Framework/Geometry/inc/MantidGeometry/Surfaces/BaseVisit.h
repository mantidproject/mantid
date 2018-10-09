// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GEOMETRY_BASEVISIT_H
#define GEOMETRY_BASEVISIT_H

#include "MantidGeometry/DllConfig.h"

namespace Mantid {
namespace Geometry {

class Surface;
class Quadratic;
class Plane;
class Cylinder;
class Cone;
class Sphere;
class General;
class Line;

/**
\class BaseVisit
\version 1.0
\author S. Ansell
\brief Adds the main

*/

class MANTID_GEOMETRY_DLL BaseVisit {

public:
  /// Destructor
  virtual ~BaseVisit() = default;

  virtual void Accept(const Surface &) = 0;  ///< Accept a surface
  virtual void Accept(const Plane &) = 0;    ///< Accept a plane
  virtual void Accept(const Sphere &) = 0;   ///< Accept a sphere
  virtual void Accept(const Cone &) = 0;     ///< Accept a cone
  virtual void Accept(const Cylinder &) = 0; ///< Accept a cylinder
  virtual void Accept(const General &) = 0;  ///< Accept a general surface
};

} // NAMESPACE Geometry
} // NAMESPACE Mantid

#endif
