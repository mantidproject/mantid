#ifndef MANTID_GEOMETRY_SYMMETRYELEMENTFACTORY_H_
#define MANTID_GEOMETRY_SYMMETRYELEMENTFACTORY_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidGeometry/Crystal/SymmetryElement.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidKernel/RegistrationHelper.h"

#include <boost/make_shared.hpp>
#include <set>

namespace Mantid {
namespace Geometry {

/** @class AbstractSymmetryElementGenerator

    SymmetryElementFactoryImpl does not generate SymmetryElement objects
    directly. Instead, in order to stay as flexible as possible, it stores
    instances of AbstractSymmetryElementGenerator. Each subclass of
    AbstractSymmetryElementGenerator can be registered once into the factory,
    which then uses the canProcess-method to determine whether a certain
    generator can be used to derive the SymmetryElement that corresponds to
    the SymmetryOperation.

    More about how the symmetry elements are derived from matrix/vector pairs
    can be found in the International Tables for Crystallography A,
    section 11.2.
 */
class MANTID_GEOMETRY_DLL AbstractSymmetryElementGenerator {
public:
  virtual ~AbstractSymmetryElementGenerator() {}

  /// Must generate a valid SymmetryElement from the given operation.
  virtual SymmetryElement_sptr
  generateElement(const SymmetryOperation &operation) const = 0;

  /// Should return true if the generator can produce a valid SymmetryElement
  /// from the provided SymmetryOperation.
  virtual bool canProcess(const SymmetryOperation &operation) const = 0;
};

typedef boost::shared_ptr<AbstractSymmetryElementGenerator>
AbstractSymmetryElementGenerator_sptr;

/** @class SymmetryElementIdentityGenerator

    This implementation of AbstractSymmetryElementGenerator produces only
    identity elements.
 */
class MANTID_GEOMETRY_DLL SymmetryElementIdentityGenerator
    : public AbstractSymmetryElementGenerator {
public:
  SymmetryElementIdentityGenerator() {}
  ~SymmetryElementIdentityGenerator() {}

  SymmetryElement_sptr
  generateElement(const SymmetryOperation &operation) const;
  bool canProcess(const SymmetryOperation &operation) const;
};

/** @class SymmetryElementTranslationGenerator

    This implementation of AbstractSymmetryElementGenerator produces only
    translation elements.
 */
class MANTID_GEOMETRY_DLL SymmetryElementTranslationGenerator
    : public AbstractSymmetryElementGenerator {
public:
  SymmetryElementTranslationGenerator() {}
  ~SymmetryElementTranslationGenerator() {}

  SymmetryElement_sptr
  generateElement(const SymmetryOperation &operation) const;
  bool canProcess(const SymmetryOperation &operation) const;
};

/** @class SymmetryElementInversionGenerator

    This implementation of AbstractSymmetryElementGenerator produces only
    inversion elements.
 */
class MANTID_GEOMETRY_DLL SymmetryElementInversionGenerator
    : public AbstractSymmetryElementGenerator {
public:
  SymmetryElementInversionGenerator() {}
  ~SymmetryElementInversionGenerator() {}

  SymmetryElement_sptr
  generateElement(const SymmetryOperation &operation) const;
  bool canProcess(const SymmetryOperation &operation) const;
};

MANTID_GEOMETRY_DLL gsl_matrix *getGSLMatrix(const Kernel::IntMatrix &matrix);
MANTID_GEOMETRY_DLL gsl_matrix *getGSLIdentityMatrix(size_t rows, size_t cols);

/** @class SymmetryElementWithAxisGenerator

    SymmetryElementWithAxisGenerator does not create any elements directly, it
    serves as a base for SymmetryElementRotationGenerator and
    SymmetryAxisMirrorGenerator, which have in common that the axis of the
    symmetry element as well as any potential translations must be determined.

    These are implemented according to the algorithms found in the International
    Tables for Crystallography A, section 11.2.

    Subclasses must implement the method to determine the Hermann-Mauguin
    symbol, as that algorithm is different for example for rotation-axes and
    mirror-planes.
 */
class MANTID_GEOMETRY_DLL SymmetryElementWithAxisGenerator
    : public AbstractSymmetryElementGenerator {
public:
  ~SymmetryElementWithAxisGenerator() {}

protected:
  V3R determineTranslation(const SymmetryOperation &operation) const;
  V3R determineAxis(const Kernel::IntMatrix &matrix) const;

  virtual std::string
  determineSymbol(const SymmetryOperation &operation) const = 0;
};

/** @class SymmetryElementRotationGenerator

    SymmetryElementRotationGenerator inherits from
    SymmetryElementWithAxisGenerator, using its methods for determination of
    rotation axis and translations in case of screw axes. Furthermore it
    determines the rotation sense and of course the Hermann-Mauguin symbol.
 */
class MANTID_GEOMETRY_DLL SymmetryElementRotationGenerator
    : public SymmetryElementWithAxisGenerator {
public:
  SymmetryElementRotationGenerator() {}
  ~SymmetryElementRotationGenerator() {}

  SymmetryElement_sptr
  generateElement(const SymmetryOperation &operation) const;
  bool canProcess(const SymmetryOperation &operation) const;

protected:
  SymmetryElementRotation::RotationSense
  determineRotationSense(const SymmetryOperation &operation,
                         const V3R &rotationAxis) const;

  std::string determineSymbol(const SymmetryOperation &operation) const;
};

/** @class SymmetryElementMirrorGenerator

    SymmetryElementMirrorGenerator also inherits from
    SymmetryElementWithAxisGenerator. In addition to that, it determines the
    Herrman-Mauguin symbol of the symmetry element. According to the
    International Tables for Crystallography there are some unconventional
    glide planes which do not have a dedicated symbol. Instead, these are
    labeled with the letter "g".
 */
class MANTID_GEOMETRY_DLL SymmetryElementMirrorGenerator
    : public SymmetryElementWithAxisGenerator {
public:
  SymmetryElementMirrorGenerator() {}
  ~SymmetryElementMirrorGenerator() {}

  SymmetryElement_sptr
  generateElement(const SymmetryOperation &operation) const;
  bool canProcess(const SymmetryOperation &operation) const;

protected:
  std::string determineSymbol(const SymmetryOperation &operation) const;

  static std::map<V3R, std::string> g_glideSymbolMap;
};

/**
  @class SymmetryElementFactoryImpl

  This factory takes a SymmetryOperation and generates the corresponding
  SymmetryElement. It determines what type of element it is (rotation, mirror or
  glide plane, ...) and creates the correct object. An example would be this:

  \code
    // Mirror plane perpendicular to z-axis
    SymmetryOperation mirrorZ("x,y,-z");

    SymmetryElement_sptr element =
            SymmetryElementFactor::Instance().createSymElement(mirrorZ);

    // Prints "m"
    std::cout << element->hmSymbol() << std::endl;

    SymmetryElementMirror_sptr mirrorElement =
            boost::dynamic_pointer_cast<SymmetryElementMirror>(element);

    // Prints [0,0,1]
    std::cout << mirrorElement->getAxis() << std::endl;
  \endcode

  Please see also the additional documentation for SymmetryElement.

  The factory itself stores generators that can generate a SymmetryElement from
  a provided SymmetryOperation. Each time createSymElement is called, the
  factory checks if an operation with that identifier-string has been processed
  before. If that's not the case, it tries to find an
  AbstractSymmetryElementGenerator that is able to process that operation. The
  SymmetryElement that is generated by the generator is not returned directly,
  instead it is stored as a prototype object, so that subsequent calls with the
  same SymmetryOperation do not have to go through the derivation algorithm
  again. Finally a clone of the now available prototype is returned. This way,
  symmetry elements are only derived once.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 25/02/2015

    Copyright © 2015 PSI-NXMM

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
class MANTID_GEOMETRY_DLL SymmetryElementFactoryImpl {
public:
  virtual ~SymmetryElementFactoryImpl() {}

  SymmetryElement_sptr createSymElement(const SymmetryOperation &operation);

  /// Subscribes the generator of type T with its class name into the factory,
  /// throws std::runtime_error if a class with the same name has already been
  /// registered.
  template <typename T>
  void
  subscribeSymmetryElementGenerator(const std::string &generatorClassName) {
    AbstractSymmetryElementGenerator_sptr generator = boost::make_shared<T>();

    if (isSubscribed(generatorClassName)) {
      throw std::runtime_error("A generator with name '" + generatorClassName +
                               "' is already registered.");
    }

    subscribe(generator, generatorClassName);
  }

protected:
  SymmetryElementFactoryImpl()
      : m_generators(), m_generatorNames(), m_prototypes() {}

  bool isSubscribed(const std::string &generatorClassName) const;
  void subscribe(const AbstractSymmetryElementGenerator_sptr &generator,
                 const std::string &generatorClassName);

  SymmetryElement_sptr createFromPrototype(const std::string &identifier) const;
  AbstractSymmetryElementGenerator_sptr
  getGenerator(const SymmetryOperation &operation) const;
  void insertPrototype(const std::string &identifier,
                       const SymmetryElement_sptr &prototype);

  std::vector<AbstractSymmetryElementGenerator_sptr> m_generators;
  std::set<std::string> m_generatorNames;
  std::map<std::string, SymmetryElement_sptr> m_prototypes;

private:
  friend struct Mantid::Kernel::CreateUsingNew<SymmetryElementFactoryImpl>;
};

#ifdef _WIN32
template class MANTID_GEOMETRY_DLL
Mantid::Kernel::SingletonHolder<SymmetryElementFactoryImpl>;
#endif

typedef Mantid::Kernel::SingletonHolder<SymmetryElementFactoryImpl>
SymmetryElementFactory;

} // namespace Geometry
} // namespace Mantid

#define DECLARE_SYMMETRY_ELEMENT_GENERATOR(classname)                          \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper                                           \
  register_symmetry_element_generator_##classname(                             \
      ((Mantid::Geometry::SymmetryElementFactory::Instance()                   \
            .subscribeSymmetryElementGenerator<classname>(#classname)),        \
       0));                                                                    \
  }

#endif /* MANTID_GEOMETRY_SYMMETRYELEMENTFACTORY_H_ */
