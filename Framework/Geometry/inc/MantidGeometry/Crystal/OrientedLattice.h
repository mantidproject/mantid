// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_ORIENTEDLATTICE_H_
#define MANTID_GEOMETRY_ORIENTEDLATTICE_H_

#include "MantidGeometry/Crystal/UnitCell.h"
#include <nexus/NeXusFile.hpp>

namespace Mantid {
namespace Geometry {
/** @class OrientedLattice OrientedLattice.h Geometry/Crystal/OrientedLattice.h
Class to implement UB matrix.
See documentation about UB matrix in the Mantid repository.\n

@author Andrei Savici, SNS, ORNL
@date 2011-04-15
*/
class MANTID_GEOMETRY_DLL OrientedLattice : public UnitCell {
public:
  // Default constructor. a = b = c = 1, alpha = beta = gamma = 90 degrees
  OrientedLattice(const Kernel::DblMatrix &Umatrix = Kernel::DblMatrix(3, 3,
                                                                       true));
  // a,b,c constructor
  OrientedLattice(const double _a, const double _b, const double _c,
                  const Kernel::DblMatrix &Umatrix = Kernel::DblMatrix(3, 3,
                                                                       true));
  // a,b,c,alpha,beta,gamma constructor
  OrientedLattice(const double _a, const double _b, const double _c,
                  const double _alpha, const double _beta, const double _gamma,
                  const Kernel::DblMatrix &Umatrix = Kernel::DblMatrix(3, 3,
                                                                       true),
                  const int angleunit = angDegrees);
  // UnitCell constructor
  OrientedLattice(const UnitCell &uc,
                  const Kernel::DblMatrix &Umatrix = Kernel::DblMatrix(3, 3,
                                                                       true));

  // Access private variables
  const Kernel::DblMatrix &getU() const;
  const Kernel::DblMatrix &getUB() const;
  void setU(const Kernel::DblMatrix &newU, const bool force = true);
  void setUB(const Kernel::DblMatrix &newUB);
  // get u and v vectors for Horace/Mslice
  Kernel::V3D getuVector() const;
  Kernel::V3D getvVector() const;
  /// Return hkl from the Q-sample coordinates
  Kernel::V3D hklFromQ(const Kernel::V3D &Q) const;
  /// Return Q-sample coordinates from hkl
  Kernel::V3D qFromHKL(const Kernel::V3D &hkl) const;
  /// Create the U matrix from two vectors
  const Kernel::DblMatrix &setUFromVectors(const Kernel::V3D &u,
                                           const Kernel::V3D &v);
  /// Save the lattice to an open NeXus file
  void saveNexus(::NeXus::File *file, const std::string &group) const;
  /// Load the lattice to from an open NeXus file
  void loadNexus(::NeXus::File *file, const std::string &group);
  /// Get the UB matix corresponding to the real space edge vectors a, b, c
  static bool GetUB(Kernel::DblMatrix &UB, const Kernel::V3D &a_dir,
                    const Kernel::V3D &b_dir, const Kernel::V3D &c_dir);

  /// Get the real space edge vectors a, b, c corresponding to the UB matrix
  static bool GetABC(const Kernel::DblMatrix &UB, Kernel::V3D &a_dir,
                     Kernel::V3D &b_dir, Kernel::V3D &c_dir);

private:
  Kernel::DblMatrix U;
  Kernel::DblMatrix UB;

  /** Make recalculateFromGstar private. */
  void recalculateFromGstar(const Kernel::DblMatrix &NewGstar) override {
    UnitCell::recalculateFromGstar(NewGstar);
  }
  void recalculate() override;
};
} // namespace Geometry
} // namespace Mantid
#endif /* MANTID_GEOMETRY_UNITCELL_H_ */
