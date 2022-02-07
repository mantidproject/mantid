// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>

namespace Mantid {
namespace Kernel {
/** @class Instantiator Instantiator.h Kernel/Instantiator.h

    The instantiator is a generic class for creating objects of the template
   type.
    It is used by DynamicFactory.

    @author Nick Draper, Tessella Support Services plc
    @date 10/10/2007
*/
template <class Base>
class DLLExport AbstractInstantiator
/// The base class for instantiators
{
public:
  /// Creates the AbstractInstantiator.
  AbstractInstantiator() = default;

  /// Destroys the AbstractInstantiator.
  virtual ~AbstractInstantiator() = default;

  /// Creates an instance of a concrete subclass of Base.
  virtual std::shared_ptr<Base> createInstance() const = 0;

  /// Creates an instance of a concrete subclass of Base, which is
  /// not wrapped in a shared_ptr
  virtual Base *createUnwrappedInstance() const = 0;

private:
  /// Private copy constructor
  AbstractInstantiator(const AbstractInstantiator &);
  /// Private assignment operator
  AbstractInstantiator &operator=(const AbstractInstantiator &);
};

// A template class for the easy instantiation of
// instantiators.
//
// For the Instantiator to work, the class of which
// instances are to be instantiated must have a no-argument
// constructor.
template <class C, class Base> class DLLExport Instantiator : public AbstractInstantiator<Base> {
public:
  /// Creates the Instantiator.
  Instantiator() = default;

  /** Creates an instance of a concrete subclass of Base.
   *  @return A pointer to the base type
   */
  std::shared_ptr<Base> createInstance() const override { return std::make_shared<C>(); }

  /** Creates an instance of a concrete subclass of Base that is not wrapped in
   * a shared_ptr.
   *  @return A bare pointer to the base type
   */
  Base *createUnwrappedInstance() const override { return static_cast<Base *>(new C()); }
};

} // namespace Kernel
} // namespace Mantid
