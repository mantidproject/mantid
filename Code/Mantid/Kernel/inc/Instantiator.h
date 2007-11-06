#ifndef MANTID_KERNEL_INSTANTIATOR_H_
#define MANTID_KERNEL_INSTANTIATOR_H_

namespace Mantid 
{
namespace Kernel
{
/** @class Instantiator Instantiator.h Kernel/Instantiator.h

    The instantiator is a generic class for creating objects of the template type.
    It is used by DynamicFactory.

    @author Nick Draper, Tessella Support Services plc
    @date 10/10/2007
    
    Copyright &copy; 2007 ???RAL???

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
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
  virtual Base* createInstance() const = 0;

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
  Base* createInstance() const
  {
    return new C;
  }
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_INSTANTIATOR_H_*/
