// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_TYPEREGISTRY_H_
#define MANTID_PYTHONINTERFACE_TYPEREGISTRY_H_

#include "MantidKernel/System.h"
#include <typeinfo>

namespace Mantid {
namespace PythonInterface {

namespace Registry {
//-----------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------
struct PropertyValueHandler;

/**
 * The values that are held within a given C++ property type all have a
 * fixed type, required by static typing. This means that when passing
 * a value for a property from Python there must be a match between between
 * the types.
 *
 * This class defines a registry of mappings between a C++ type T and a
 * PropertyValueHandler object that is able to extract (or attempt to extract)
 * the correct C++ type for that property from a given Python object.
 */
class DLLExport TypeRegistry {
public:
  /// Register handlers for basic C++ types into the registry
  static void registerBuiltins();
  /// Subscribe a handler object for given template type
  template <typename HandlerType> static void subscribe() {
    subscribe(typeid(typename HandlerType::HeldType), new HandlerType);
  }
  /// Subscribe a handler object for a given typeinfo
  static void subscribe(const std::type_info &typeInfo,
                        PropertyValueHandler *handler);
  /// Lookup a handler base on a given type_info object
  static const PropertyValueHandler &retrieve(const std::type_info &typeInfo);
};
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_TYPEREGISTRY_H_*/
