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

class MANTID_GEOMETRY_DLL AbstractSymmetryElementGenerator {
public:
  virtual ~AbstractSymmetryElementGenerator() {}

  virtual SymmetryElement_sptr
  generateElement(const SymmetryOperation &operation) const = 0;

  virtual bool canProcess(const SymmetryOperation &operation) const = 0;
};

typedef boost::shared_ptr<AbstractSymmetryElementGenerator>
AbstractSymmetryElementGenerator_sptr;

class MANTID_GEOMETRY_DLL SymmetryElementIdentityGenerator
    : public AbstractSymmetryElementGenerator {
public:
  SymmetryElementIdentityGenerator() {}
  ~SymmetryElementIdentityGenerator() {}

  SymmetryElement_sptr
  generateElement(const SymmetryOperation &operation) const;
  bool canProcess(const SymmetryOperation &operation) const;
};

class MANTID_GEOMETRY_DLL SymmetryElementTranslationGenerator
    : public AbstractSymmetryElementGenerator {
public:
  SymmetryElementTranslationGenerator() {}
  ~SymmetryElementTranslationGenerator() {}

  SymmetryElement_sptr
  generateElement(const SymmetryOperation &operation) const;
  bool canProcess(const SymmetryOperation &operation) const;
};

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
  @class SymmetryElementFactory

  This factory takes a SymmetryOperation and generates the corresponding
  SymmetryElement. It determines what type of element it is (rotation, mirror or
  glide plane, ...) and creates the correct object. An example would be this:

    // Mirror plane perpendicular to z-axis
    SymmetryOperation mirrorZ("x,y,-z");

    SymmetryElement_sptr element =
            SymmetryElementFactor::Instance().createSymElem(mirrorZ);

    // Prints "m"
    std::cout << element->hmSymbol() << std::endl;

    SymmetryElementMirror_sptr mirrorElement =
            boost::dynamic_pointer_cast<SymmetryElementMirror>(element);

    // Prints [0,0,1]
    std::cout << mirrorElement->getAxis() << std::endl;

  Please see also the additional documentation for SymmetryElement.

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

  SymmetryElement_sptr createSymElem(const SymmetryOperation &operation);

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
