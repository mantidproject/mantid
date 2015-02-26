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

  virtual boost::shared_ptr<SymmetryElement> clone() const = 0;

  std::string hmSymbol() const { return m_hmSymbol; }

protected:
  SymmetryElement(const std::string &symbol);

  std::string m_hmSymbol;
};

typedef boost::shared_ptr<SymmetryElement> SymmetryElement_sptr;

class MANTID_GEOMETRY_DLL SymmetryElementIdentity : public SymmetryElement {
public:
  SymmetryElementIdentity();
  ~SymmetryElementIdentity() {}

  SymmetryElement_sptr clone() const;
};

typedef boost::shared_ptr<SymmetryElementIdentity> SymmetryElementIdentity_sptr;

class MANTID_GEOMETRY_DLL SymmetryElementInversion : public SymmetryElement {
public:
  SymmetryElementInversion(const V3R &inversionPoint = V3R(0,0,0));
  ~SymmetryElementInversion() {}

  SymmetryElement_sptr clone() const;

  V3R getInversionPoint() const { return m_inversionPoint; }

protected:
  V3R m_inversionPoint;
};

typedef boost::shared_ptr<SymmetryElementInversion>
SymmetryElementInversion_sptr;

class MANTID_GEOMETRY_DLL SymmetryElementTranslation : public SymmetryElement {
public:
  SymmetryElementTranslation(const V3R &translation);
  ~SymmetryElementTranslation() {}

  V3R getTranslation() const { return m_translation; }

  SymmetryElement_sptr clone() const;

protected:
  V3R m_translation;
};

typedef boost::shared_ptr<SymmetryElementTranslation>
SymmetryElementTranslation_sptr;

class MANTID_GEOMETRY_DLL SymmetryElementWithAxis : public SymmetryElement {
public:
  ~SymmetryElementWithAxis() {}

  V3R getAxis() const { return m_axis; }
  V3R getTranslation() const { return m_translation; }

protected:
  SymmetryElementWithAxis(const std::string &symbol, const V3R &axis,
                          const V3R &translation);

  void setAxis(const V3R &axis);

  V3R m_axis;
  V3R m_translation;
};

typedef boost::shared_ptr<SymmetryElementWithAxis> SymmetryElementWithAxis_sptr;

class MANTID_GEOMETRY_DLL SymmetryElementRotation
    : public SymmetryElementWithAxis {
public:
  enum RotationSense {
    Positive,
    Negative
  };

  SymmetryElementRotation(const std::string &symbol, const V3R &axis,
                          const V3R &translation = V3R(0,0,0),
                          const RotationSense &rotationSense = Positive);
  ~SymmetryElementRotation() {}

  SymmetryElement_sptr clone() const;

  RotationSense getRotationSense() const { return m_rotationSense; }

protected:
  RotationSense m_rotationSense;
};

typedef boost::shared_ptr<SymmetryElementRotation> SymmetryElementRotation_sptr;

class MANTID_GEOMETRY_DLL SymmetryElementMirror
    : public SymmetryElementWithAxis {
public:
  SymmetryElementMirror(const std::string &symbol, const V3R &axis,
                        const V3R &translation = V3R(0,0,0));
  ~SymmetryElementMirror() {}

  SymmetryElement_sptr clone() const;
};

typedef boost::shared_ptr<SymmetryElementMirror> SymmetryElementMirror_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SYMMETRYELEMENT_H_ */
