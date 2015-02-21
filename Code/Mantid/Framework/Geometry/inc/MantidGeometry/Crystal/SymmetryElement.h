#ifndef MANTID_GEOMETRY_SYMMETRYELEMENT_H_
#define MANTID_GEOMETRY_SYMMETRYELEMENT_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Crystal/V3R.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"

#include <boost/shared_ptr.hpp>
#include <gsl/gsl_matrix.h>

namespace Mantid {
namespace Geometry {

/** @class SymmetryElement

    SymmetryElement is an interface for representing symmetry elements that
    occur for example in space and point groups.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 05/02/2015

    Copyright © 2015 PSI-MSS

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
class MANTID_GEOMETRY_DLL SymmetryElement {
public:
  virtual ~SymmetryElement() {}

  virtual void init(const SymmetryOperation &operation) = 0;

  std::string hmSymbol() const { return m_hmSymbol; }

protected:
  SymmetryElement();

  void setHMSymbol(const std::string &symbol);

  std::string m_hmSymbol;
};

typedef boost::shared_ptr<SymmetryElement> SymmetryElement_sptr;

class MANTID_GEOMETRY_DLL SymmetryElementIdentity : public SymmetryElement {
public:
  SymmetryElementIdentity();
  ~SymmetryElementIdentity() {}

  void init(const SymmetryOperation &operation);
};

class MANTID_GEOMETRY_DLL SymmetryElementInversion : public SymmetryElement {
public:
  SymmetryElementInversion();
  ~SymmetryElementInversion() {}

  void init(const SymmetryOperation &operation);

  V3R getInversionPoint() const { return m_inversionPoint; }

protected:
  void setInversionPoint(const V3R &inversionPoint);

  V3R m_inversionPoint;
};

class MANTID_GEOMETRY_DLL SymmetryElementWithAxis : public SymmetryElement {
public:
  ~SymmetryElementWithAxis() {}

  V3R getAxis() const { return m_axis; }
  V3R getTranslation() const { return m_translation; }

protected:
  SymmetryElementWithAxis();

  void setAxis(const V3R &axis);
  void setTranslation(const V3R &translation) { m_translation = translation; }

  V3R determineTranslation(const SymmetryOperation &operation) const;
  V3R determineAxis(const Kernel::IntMatrix &matrix) const;

  virtual std::string
  determineSymbol(const SymmetryOperation &operation) const = 0;

  V3R m_axis;
  V3R m_translation;
};

class MANTID_GEOMETRY_DLL SymmetryElementRotation
    : public SymmetryElementWithAxis {
public:
  enum RotationSense {
    Positive,
    Negative
  };

  SymmetryElementRotation();
  ~SymmetryElementRotation() {}

  RotationSense getRotationSense() const { return m_rotationSense; }

  void init(const SymmetryOperation &operation);

protected:
  void setRotationSense(const RotationSense &rotationSense) {
    m_rotationSense = rotationSense;
  }

  RotationSense determineRotationSense(const SymmetryOperation &operation,
                                       const V3R &rotationAxis) const;

  bool isNotRotation(int determinant, int trace) const;
  std::string determineSymbol(const SymmetryOperation &operation) const;

  RotationSense m_rotationSense;
};

class MANTID_GEOMETRY_DLL SymmetryElementMirror
    : public SymmetryElementWithAxis {
public:
  SymmetryElementMirror();
  ~SymmetryElementMirror() {}

  void init(const SymmetryOperation &operation);

protected:
  bool isNotMirror(int determinant, int trace) const;
  std::string determineSymbol(const SymmetryOperation &operation) const;

  static std::map<V3R, std::string> g_glideSymbolMap;
};

MANTID_GEOMETRY_DLL gsl_matrix *getGSLMatrix(const Kernel::IntMatrix &matrix);
MANTID_GEOMETRY_DLL gsl_matrix *getGSLIdentityMatrix(size_t rows, size_t cols);

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SYMMETRYELEMENT_H_ */
