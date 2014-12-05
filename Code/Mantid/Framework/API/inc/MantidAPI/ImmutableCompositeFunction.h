#ifndef MANTID_API_IMMUTABLECOMPOSITEFUNCTION_H_
#define MANTID_API_IMMUTABLECOMPOSITEFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"

#include <map>

namespace Mantid
{
namespace API
{
/** 
    Immutable composite function is a composite function which members cannot be added or removed
    after creation. Only a derived class can add functions in its contructor (or methods). 
    The function factory treat an ImmutableCompositeFunction as a simple function.

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MANTID_API_DLL ImmutableCompositeFunction : public CompositeFunction
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
  /// Set i-th parameter
  void setParameter(size_t i, const double& value, bool explicitlySet = true)
  {CompositeFunction::setParameter(i, value, explicitlySet);}
  /// Set i-th parameter description
  void setParameterDescription(size_t i, const std::string& description)
  {CompositeFunction::setParameterDescription(i, description);}
  /// Set parameter by name.
  void setParameter(const std::string& name, const double& value, bool explicitlySet = true);
  /// Set description of parameter by name.
  void setParameterDescription(const std::string& name, const std::string& description);
  /// Get i-th parameter
  double getParameter(size_t i)const { return CompositeFunction::getParameter( i ); }
  /// Get parameter by name.
  double getParameter(const std::string& name)const;
  /// Returns the index of parameter name
  size_t parameterIndex(const std::string& name)const;
  /// Returns the name of parameter i
  std::string parameterName(size_t i)const;

protected:

  /// Make it protected
  using CompositeFunction::addFunction;
  /// Overload addFunction to take a bare pointer
  void addFunction(IFunction* fun);
  /// Define an alias for a parameter
  void setAlias(const std::string& parName, const std::string& alias);
  /// Add default ties
  void addDefaultTies(const std::string& ties);
  /// Add default constraints
  void addDefaultConstraints(const std::string& constraints);

private:

  /// Keep paramater aliases
  std::map< std::string, size_t > m_alias;

};


} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IMMUTABLECOMPOSITEFUNCTION_H_*/
