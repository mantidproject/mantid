#ifndef MANTID_GEOMETRY_SYMMETRYELEMENT_H_
#define MANTID_GEOMETRY_SYMMETRYELEMENT_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Crystal/V3R.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {

/** @class SymmetryElement

    SymmetryElement is an interface for representing symmetry elements that
    occur for example in space and point groups.

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
  V3R getFixPoint() const { return m_fixPoint; }

protected:
  SymmetryElementWithAxis();

  V3R determineTranslation(const SymmetryOperation &operation) const;
  V3R determineAxis(const Kernel::IntMatrix &matrix) const;
  V3R determineFixPoint(const Kernel::IntMatrix &matrix,
                        const V3R &vector) const;

  V3R m_axis;
  V3R m_translation;
  V3R m_fixPoint;
};

class MANTID_GEOMETRY_DLL SymmetryElementRotation
    : public SymmetryElementWithAxis {
public:
  SymmetryElementRotation();
  ~SymmetryElementRotation() {}

  void init(const SymmetryOperation &operation);
};

class MANTID_GEOMETRY_DLL SymmetryElementMirror
    : public SymmetryElementWithAxis {
  SymmetryElementMirror();
  ~SymmetryElementMirror() {}

  void init(const SymmetryOperation &operation);
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SYMMETRYELEMENT_H_ */
