// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef General_h
#define General_h

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Surfaces/Quadratic.h"

namespace Mantid {
namespace Geometry {

/**
  \class General
  \brief Holds a general quadratic surface
  \author S. Ansell
  \date April 2004
  \version 1.0

  Holds a general surface with equation form
  \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
  which has been defined as a gq surface in MCNPX.
  It is a realisation of the Surface object.
*/

class MANTID_GEOMETRY_DLL General : public Quadratic {
private:
  General *doClone() const override;

protected:
  General(const General &) = default;
  General &operator=(const General &) = delete;

public:
  General();
  std::unique_ptr<General> clone() const;

  int setSurface(const std::string &) override;
  void setBaseEqn() override;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin) override;
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif
