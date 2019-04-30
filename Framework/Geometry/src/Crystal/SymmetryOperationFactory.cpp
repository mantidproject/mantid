// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/LibraryManager.h"

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {

/// Creates a SymmetryOperation object from its identifier.
SymmetryOperation
SymmetryOperationFactoryImpl::createSymOp(const std::string &identifier) {
  if (!isSubscribed(identifier)) {
    subscribeSymOp(identifier);
  }

  return SymmetryOperation(m_prototypes[identifier]);
}

/// Creates all symmetry operations in string (separated by semicolon).
std::vector<SymmetryOperation>
SymmetryOperationFactoryImpl::createSymOps(const std::string &identifiers) {
  std::vector<std::string> symOpStrings;
  boost::split(symOpStrings, identifiers, boost::is_any_of(";"));

  return createSymOps(symOpStrings);
}

/// Creates all symmetry operations with the given strings (whitespaces at
/// beginning and end are removed).
std::vector<SymmetryOperation> SymmetryOperationFactoryImpl::createSymOps(
    const std::vector<std::string> &identifiers) {
  std::vector<SymmetryOperation> symOps;
  symOps.reserve(identifiers.size());
  for (const auto &identifier : identifiers) {
    symOps.emplace_back(createSymOp(boost::trim_copy(identifier)));
  }

  return symOps;
}

/// Subscribes a symmetry operation into the factory
void SymmetryOperationFactoryImpl::subscribeSymOp(
    const std::string &identifier) {
  SymmetryOperation prototype(identifier);

  subscribe(identifier, prototype);
}

/// Unsubscribes a symmetry operation from the factory
void SymmetryOperationFactoryImpl::unsubscribeSymOp(
    const std::string &identifier) {
  if (isSubscribed(identifier)) {
    m_prototypes.erase(identifier);
  }
}

/// Returns true if identifier already has a prototype in the factory.
bool SymmetryOperationFactoryImpl::isSubscribed(
    const std::string &identifier) const {
  return m_prototypes.find(identifier) != m_prototypes.end();
}

/// Returns all symbols in the factory.
std::vector<std::string>
SymmetryOperationFactoryImpl::subscribedSymbols() const {
  std::vector<std::string> symbols;
  symbols.reserve(m_prototypes.size());
  for (const auto &prototype : m_prototypes) {
    symbols.emplace_back(prototype.first);
  }
  return symbols;
}

/// Subscribes symmetry operation into factory, using the supplied alias as key.
void SymmetryOperationFactoryImpl::subscribe(
    const std::string &alias, const SymmetryOperation &prototype) {
  if (!isSubscribed(alias)) {
    m_prototypes.emplace(alias, prototype);
  }
}

/// Private default constructor.
SymmetryOperationFactoryImpl::SymmetryOperationFactoryImpl() : m_prototypes() {
  Kernel::LibraryManager::Instance();
}

} // namespace Geometry
} // namespace Mantid
