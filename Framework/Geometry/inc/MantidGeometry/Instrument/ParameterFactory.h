// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_PARAMETERFACTORY_H_
#define MANTID_GEOMETRY_PARAMETERFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Instantiator.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SingletonHolder.h"

#include <map>
#include <vector>

namespace Mantid {

namespace Kernel {
class Logger;
}

namespace Geometry {

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Parameter;

/** The ParameterFactory class creates parameters for the instrument
   ParameterMap.

    @author Roman Tolchenov, Tessella plc
    @date 19/05/2009
*/
class MANTID_GEOMETRY_DLL ParameterFactory {
public:
  template <class C> static void subscribe(const std::string &className);

  static boost::shared_ptr<Parameter> create(const std::string &className,
                                             const std::string &name);

private:
  /// Private default constructor
  ParameterFactory();
  /// Private copy constructor
  ParameterFactory(const ParameterFactory &);
  /// Private assignment operator
  ParameterFactory &operator=(const ParameterFactory &);

  /// A typedef for the instantiator
  using AbstractFactory = Kernel::AbstractInstantiator<Parameter>;
  using FactoryMap = std::map<std::string, std::unique_ptr<AbstractFactory>>;
  /// The map holding the registered class names and their instantiators
  static FactoryMap s_map;
};

/**  Templated method for parameter subscription
 *   @param className :: The parameter type name
 *   @tparam C The parameter type
 */
template <class C>
void ParameterFactory::subscribe(const std::string &className) {
  auto it = s_map.find(className);
  if (!className.empty() && it == s_map.cend()) {
    s_map[className] =
        std::make_unique<Kernel::Instantiator<C, Parameter>>();
  } else {
    throw std::runtime_error("Parameter type" + className +
                             " is already registered.\n");
  }
}

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_PARAMETERFACTORY_H_*/
