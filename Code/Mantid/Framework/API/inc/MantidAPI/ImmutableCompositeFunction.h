#ifndef MANTID_API_IMMUTABLECOMPOSITEFUNCTION_H_
#define MANTID_API_IMMUTABLECOMPOSITEFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"

namespace Mantid
{
namespace API
{
/** 
    Immutable composite function is a composite function which members cannot be added or removed
    after creation. Only a derived class can add functions in its contructor (or methods). 
    The function factory treat an ImmutableCompositeFunction as a simple function.

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class MANTID_API_DLL ImmutableCompositeFunction : public virtual CompositeFunction
{
public:
  /// Default constructor
  ImmutableCompositeFunction(): CompositeFunction(){}
  // Destructor
  ~ImmutableCompositeFunction(){}

              /* Overriden methods */

  /// Returns the function's name
  virtual std::string name()const {return "ImmutableCompositeFunction";}
  /// Writes itself into a string
  std::string asString()const;

protected:

  // make it protected
  using CompositeFunction::addFunction;

};


} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IMMUTABLECOMPOSITEFUNCTION_H_*/
