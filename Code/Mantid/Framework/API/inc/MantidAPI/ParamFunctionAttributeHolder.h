#ifndef MANTID_API_PARAMFUNCTIONATTRIBUTEHOLDER_H_
#define MANTID_API_PARAMFUNCTIONATTRIBUTEHOLDER_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid
{
  namespace API
  {
    /**
     *
     * Defines a structure to simplify handling of attributes/parameters for fit functions
     *
     * It holds the attributes themselves and defines a declareAttributes function that
     * can be overridden to define the attributes of the model. Similary, fit parameters
     * can be declared with the declareParameters function.
     *
     */
    class MANTID_API_DLL ParamFunctionAttributeHolder : public API::ParamFunction
    {
    public:
      /// Default constructor
      ParamFunctionAttributeHolder();

      /// Returns the number of attributes associated with the function
      size_t nAttributes() const;
      /// Check if attribute named exists
      bool hasAttribute(const std::string& name)const;
      /// Returns a list of attribute names
      std::vector<std::string> getAttributeNames() const;
      /// Return a value of attribute attName
      API::IFunction::Attribute getAttribute(const std::string& name) const;
      /// Set a value to a named attribute.
      void setAttribute(const std::string& name, const API::IFunction::Attribute & value);

    protected:
      /// Initialize the function holder
      void init();

      /// Override to declare function attributes
      virtual void declareAttributes() {}
      /// Override to declare function parameters
      virtual void declareParameters() {};

      /// Declare a single attribute
      void declareAttribute(const std::string & name, const API::IFunction::Attribute & defaultValue);
      /// Store an attribute's value
      void storeAttributeValue(const std::string& name, const API::IFunction::Attribute & value);

    private:
      /// The declared attributes
      std::map<std::string, API::IFunction::Attribute> m_attrs;
    };

  }
}
#endif /* MANTID_API_PARAMFUNCTIONATTRIBUTEHOLDER_H_ */
