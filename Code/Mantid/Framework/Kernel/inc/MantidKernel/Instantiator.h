#ifndef MANTID_KERNEL_INSTANTIATOR_H_
#define MANTID_KERNEL_INSTANTIATOR_H_

#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif

namespace Mantid 
{
namespace Kernel
{
/** @class Instantiator Instantiator.h Kernel/Instantiator.h

    The instantiator is a generic class for creating objects of the template type.
    It is used by DynamicFactory.

    @author Nick Draper, Tessella Support Services plc
    @date 10/10/2007
    
    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <class Base>
class DLLExport AbstractInstantiator
/// The base class for instantiators
{
public:
  /// Creates the AbstractInstantiator.
  AbstractInstantiator()
  {
  }
  
  /// Destroys the AbstractInstantiator.
  virtual ~AbstractInstantiator()
  {
  }
  
  /// Creates an instance of a concrete subclass of Base. 
  virtual boost::shared_ptr<Base> createInstance() const = 0;

  /// Creates an instance of a concrete subclass of Base, which is
  /// not wrapped in a boost shared_ptr
  virtual Base* createUnwrappedInstance() const = 0;

private:
  /// Private copy constructor
  AbstractInstantiator(const AbstractInstantiator&);
  /// Private assignment operator
  AbstractInstantiator& operator = (const AbstractInstantiator&);
};

// A template class for the easy instantiation of 
// instantiators. 
//
// For the Instantiator to work, the class of which
// instances are to be instantiated must have a no-argument
// constructor.
template <class C, class Base>
class DLLExport Instantiator: public AbstractInstantiator<Base>
{
public:
  /// Creates the Instantiator.
  Instantiator()
  {
  }
  
  /// Destroys the Instantiator.
  virtual ~Instantiator()
  {
  }

  /** Creates an instance of a concrete subclass of Base. 
   *  @return A pointer to the base type
   */
  boost::shared_ptr<Base> createInstance() const
  {
    boost::shared_ptr<Base> ptr(new C);
    return ptr;
  }

  /** Creates an instance of a concrete subclass of Base that is not wrapped in a boost shared_ptr.
   *  @return A bare pointer to the base type
   */
  virtual Base* createUnwrappedInstance() const
  {
    return static_cast<Base*>(new C);
  }
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_INSTANTIATOR_H_*/
