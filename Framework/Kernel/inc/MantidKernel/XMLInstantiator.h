// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_XMLINSTANTIATOR_H_
#define MANTID_KERNEL_XMLINSTANTIATOR_H_

#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

/** @class XMLInstantiator XMLInstantiator.h Kernel/XMLInstantiator.h

    The XML instantiator is a generic class for creating objects of the template
    type.  It very similar to the regular instantiator, but will pass an
    XML element into the concrete object's constructor.  In cases where the
    concrete object can use a default constructor, the standard instantiator
    is probably a better choice.
    This class is used by RemoteJobManagerFactoryImpl.

    @author Ross Miller, Oak Ridge National Laboratory
    @date 29/11/2012
*/

// Forward declarations
namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace Kernel {
template <class Base>
class DLLExport XMLAbstractInstantiator
/// The base class for XML instantiators
{
public:
  /// Creates the XMLAbstractInstantiator.
  XMLAbstractInstantiator() {}

  /// Destroys the XMLAbstractInstantiator.
  virtual ~XMLAbstractInstantiator() {}

  /// Creates an instance of a concrete subclass of Base.
  virtual boost::shared_ptr<Base>
  createInstance(const Poco::XML::Element *elem) const = 0;

  /// Creates an instance of a concrete subclass of Base, which is
  /// not wrapped in a boost shared_ptr
  virtual Base *
  createUnwrappedInstance(const Poco::XML::Element *elem) const = 0;

private:
  /// Private copy constructor
  XMLAbstractInstantiator(const XMLAbstractInstantiator &);
  /// Private assignment operator
  XMLAbstractInstantiator &operator=(const XMLAbstractInstantiator &);
};

// A template class for the easy instantiation of
// instantiators.
//
// For the Instantiator to work, the class of which
// instances are to be instantiated must have a no-argument
// constructor.
template <class C, class Base>
class DLLExport XMLInstantiator : public XMLAbstractInstantiator<Base> {
public:
  /// Creates the XML Instantiator.
  XMLInstantiator() {}

  /// Destroys the XML Instantiator.
  virtual ~XMLInstantiator() {}

  /** Creates an instance of a concrete subclass of Base.
   *  @return A pointer to the base type
   */
  boost::shared_ptr<Base> createInstance(const Poco::XML::Element *elem) const {
    boost::shared_ptr<Base> ptr(new C(elem));
    return ptr;
  }

  /** Creates an instance of a concrete subclass of Base that is not wrapped in
   * a boost shared_ptr.
   *  @return A bare pointer to the base type
   */
  virtual Base *createUnwrappedInstance(const Poco::XML::Element *elem) const {
    return static_cast<Base *>(new C(elem));
  }
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_XMLINSTANTIATOR_H_*/
