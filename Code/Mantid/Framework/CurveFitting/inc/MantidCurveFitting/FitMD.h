#ifndef MANTID_CURVEFITTING_FITMD_H_
#define MANTID_CURVEFITTING_FITMD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IDomainCreator.h"

namespace Mantid
{

  namespace API
  {
    class FunctionDomain;
    class FunctionDomainMD;
    class IFunctionValues;
    class IMDWorkspace;
  }

  namespace CurveFitting
  {
    /**
    Creates FunctionDomainMD from an IMDWorkspace. Does not create any properties.

    @author Roman Tolchenov, Tessella plc
    @date 06/12/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport FitMD : public IDomainCreator
    {
    public:
      /// Constructor
      FitMD(Kernel::IPropertyManager* fit, const std::string& workspacePropertyName)
        :IDomainCreator(fit,std::vector<std::string>(1,workspacePropertyName)){}
      /// Create a domain from the input workspace
      virtual void createDomain(
        boost::shared_ptr<API::FunctionDomain>&, 
        boost::shared_ptr<API::IFunctionValues>&, size_t i0);
      /// Return the size of the domain to be created.
      virtual size_t getDomainSize() const;
    protected:

      /// The input IMDWorkspace
      boost::shared_ptr<API::IMDWorkspace> m_IMDWorkspace;
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FITMD_H_*/
