// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/Crystal/V3R.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"

#include <gsl/gsl_matrix.h>
#include <memory>

namespace Mantid {
namespace Geometry {

/** @class SymmetryElement

    SymmetryElement is an interface for representing symmetry elements that
    occur for example in space and point groups. The base class itself can not
    be used, it only defines a (very small) interface that currently only
    contains the Hermann-Mauguin symbol of the element, as that is the one
    common factor for all symmetry elements. There are however classes for
    identity, translation, inversion, rotation and mirror.

    However, these classes should not be instantiated directly, they are merely
    storing the information on a symmetry element. Instead, they should be
    generated through SymmetryElementFactory, which generates the elements
    from SymmetryOperation-objects. Please see the documentation of
    SymmetryElementFactory for details and examples.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 05/02/2015
  */
class MANTID_GEOMETRY_DLL SymmetryElement {
public:
  virtual ~SymmetryElement() = default;

  virtual std::shared_ptr<SymmetryElement> clone() const = 0;

  /// Returns the internally stored Hermann-Mauguin symbol.
  const std::string &hmSymbol() const { return m_hmSymbol; }

protected:
  SymmetryElement(std::string symbol);

  std::string m_hmSymbol;
};

using SymmetryElement_sptr = std::shared_ptr<SymmetryElement>;

/** @class SymmetryElementIdentity

    SymmetryElementIdentity represents the identity. It has no parameters and
    always returns the symbol "1".
 */
class MANTID_GEOMETRY_DLL SymmetryElementIdentity : public SymmetryElement {
public:
  SymmetryElementIdentity();

  SymmetryElement_sptr clone() const override;
};

using SymmetryElementIdentity_sptr = std::shared_ptr<SymmetryElementIdentity>;

/** @class SymmetryElementInversion

    SymmetryElementInversion represents the inversion. The default inversion
    point is (0,0,0), but the constructor takes the inversion point as
    a parameter. The symbol stored internally is "-1".
 */
class MANTID_GEOMETRY_DLL SymmetryElementInversion : public SymmetryElement {
public:
  SymmetryElementInversion(const V3R &inversionPoint = V3R(0, 0, 0));

  SymmetryElement_sptr clone() const override;

  /// Returns the internally stored inversion point.
  V3R getInversionPoint() const { return m_inversionPoint; }

protected:
  V3R m_inversionPoint;
};

using SymmetryElementInversion_sptr = std::shared_ptr<SymmetryElementInversion>;

/** @class SymmetryElementTranslation

    SymmetryElementTranslation represents translations. The constructor takes
    a translation vector as argument, which is then stored. As symbol, "t" is
    used, in accordance with the International Tables for Crystallography.
 */
class MANTID_GEOMETRY_DLL SymmetryElementTranslation : public SymmetryElement {
public:
  SymmetryElementTranslation(const V3R &translation);

  /// Returns the internally stored translation vector.
  V3R getTranslation() const { return m_translation; }

  SymmetryElement_sptr clone() const override;

protected:
  V3R m_translation;
};

using SymmetryElementTranslation_sptr = std::shared_ptr<SymmetryElementTranslation>;

/** @class SymmetryElementWithAxis

    SymmetryElementWithAxis does not represent any symmetry element directly. It
    serves as a base class for rotation-axes and mirror-planes, as both are
    described by an axis and may have a translation component (screws and
    glides). The axis-vector can not be (0,0,0).
 */
class MANTID_GEOMETRY_DLL SymmetryElementWithAxis : public SymmetryElement {
public:
  /// Returns the internally stored axis.
  V3R getAxis() const { return m_axis; }

  /// Returns the internally stored translation vector.
  V3R getTranslation() const { return m_translation; }

protected:
  SymmetryElementWithAxis(const std::string &symbol, const V3R &axis, const V3R &translation);

  void setAxis(const V3R &axis);

  V3R m_axis;
  V3R m_translation;
};

using SymmetryElementWithAxis_sptr = std::shared_ptr<SymmetryElementWithAxis>;

/** @class SymmetryElementRotation

    SymmetryElementRotation represents rotation-, rotoinversion- and screw-axes.
    Besides the axis and translation vectors, which are inherited from
    SymmetryElementWithAxis, it also provides a rotation sense.

    When constructed directly, it's possible to leave out translation and
    rotation sense, these will be set to default values ((0,0,0) and positive
    rotation sense).

    Symbol determination is perfomed by SymmetryElementRotationGenerator, which
    uses the SymmetryOperation to derive the Herrman-Mauguin symbol.
 */
class MANTID_GEOMETRY_DLL SymmetryElementRotation : public SymmetryElementWithAxis {
public:
  enum RotationSense { Positive, Negative, NoRotation };

  SymmetryElementRotation(const std::string &symbol, const V3R &axis, const V3R &translation = V3R(0, 0, 0),
                          const RotationSense &rotationSense = Positive);

  SymmetryElement_sptr clone() const override;

  /// Returns the internally stored rotation sense.
  RotationSense getRotationSense() const { return m_rotationSense; }

protected:
  RotationSense m_rotationSense;
};

using SymmetryElementRotation_sptr = std::shared_ptr<SymmetryElementRotation>;

/** @class SymmetryElementMirror

    SymmetryElementMirror represents mirror and glide-planes. The axis which is
    inherited from SymmetryElementWithAxis is perpendicular to the actual
    mirror-plane. The translation is (0,0,0) by default.

    Symbol determination is perfomed by SymmetryElementMirrorGenerator, which
    uses the SymmetryOperation to derive the Herrman-Mauguin symbol.
 */
class MANTID_GEOMETRY_DLL SymmetryElementMirror : public SymmetryElementWithAxis {
public:
  SymmetryElementMirror(const std::string &symbol, const V3R &axis, const V3R &translation = V3R(0, 0, 0));

  SymmetryElement_sptr clone() const override;
};

using SymmetryElementMirror_sptr = std::shared_ptr<SymmetryElementMirror>;

} // namespace Geometry
} // namespace Mantid
