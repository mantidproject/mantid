#ifndef MANTID_KERNEL_XMLINSTANTIATOR_H_
#define MANTID_KERNEL_XMLINSTANTIATOR_H_

#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
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
    
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

// Forward declarations
namespace Poco
{
  namespace XML
  {
    class Element;
  }
}


namespace Mantid
{
namespace Kernel
{
template <class Base>
class DLLExport XMLAbstractInstantiator
/// The base class for XML instantiators
{
public:
  /// Creates the XMLAbstractInstantiator.
  XMLAbstractInstantiator()
  {
  }
  
  /// Destroys the XMLAbstractInstantiator.
  virtual ~XMLAbstractInstantiator()
  {
  }
  
  /// Creates an instance of a concrete subclass of Base. 
  virtual boost::shared_ptr<Base> createInstance( const Poco::XML::Element* elem) const = 0;

  /// Creates an instance of a concrete subclass of Base, which is
  /// not wrapped in a boost shared_ptr
  virtual Base* createUnwrappedInstance( const Poco::XML::Element* elem) const = 0;

private:
  /// Private copy constructor
  XMLAbstractInstantiator(const XMLAbstractInstantiator&);
  /// Private assignment operator
  XMLAbstractInstantiator& operator = (const XMLAbstractInstantiator&);
};

// A template class for the easy instantiation of 
// instantiators. 
//
// For the Instantiator to work, the class of which
// instances are to be instantiated must have a no-argument
// constructor.
template <class C, class Base>
class DLLExport XMLInstantiator: public XMLAbstractInstantiator<Base>
{
public:
  /// Creates the XML Instantiator.
  XMLInstantiator()
  {
  }
  
  /// Destroys the XML Instantiator.
  virtual ~XMLInstantiator()
  {
  }

  /** Creates an instance of a concrete subclass of Base. 
   *  @return A pointer to the base type
   */
  boost::shared_ptr<Base> createInstance(const Poco::XML::Element* elem) const
  {
    boost::shared_ptr<Base> ptr(new C( elem));
    return ptr;
  }

  /** Creates an instance of a concrete subclass of Base that is not wrapped in a boost shared_ptr.
   *  @return A bare pointer to the base type
   */
  virtual Base* createUnwrappedInstance( const Poco::XML::Element* elem) const
  {
    return static_cast<Base*>(new C(elem));
  }
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_XMLINSTANTIATOR_H_*/
