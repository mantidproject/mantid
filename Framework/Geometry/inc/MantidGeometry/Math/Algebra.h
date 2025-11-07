// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Math/Acomp.h"

namespace Mantid {

namespace Geometry {

/**
  \class  Algebra
  \brief Computes Boolean algebra for simplification
  \author S. Ansell
  \date August 2005
  \version 1.0

  A leveled algebra class for each
  level of factorisation.

*/

class MANTID_GEOMETRY_DLL Algebra {
private:
  std::map<int, std::string> SurfMap; ///< Internal surface map
  Acomp F;                            ///< Factor

public:
  Algebra();

  /// Accessor
  const Acomp &getComp() const { return F; }

  bool operator==(const Algebra &) const;
  bool operator!=(const Algebra &) const;
  Algebra &operator+=(const Algebra &);
  Algebra &operator*=(const Algebra &);
  Algebra operator+(const Algebra &) const;
  Algebra operator*(const Algebra &) const;
  int logicalEqual(const Algebra &) const;

  void Complement();
  void makeDNF() { F.makeDNFobject(); } ///< assessor to makeDNFobj
  void makeCNF() { F.makeCNFobject(); } ///< assessor to makeCNFobj
  std::pair<Algebra, Algebra> algDiv(const Algebra &) const;
  int setFunctionObjStr(const std::string &);
  int setFunction(const std::string &);
  int setFunction(const Acomp &);

  std::ostream &write(std::ostream &) const;
  std::string writeMCNPX() const;

  // Debug Functions::
  int countLiterals() const;
  /// Displays the algebra string
  std::string display() const;
};

std::ostream &operator<<(std::ostream &, const Algebra &);

} // NAMESPACE Geometry

} // NAMESPACE Mantid
