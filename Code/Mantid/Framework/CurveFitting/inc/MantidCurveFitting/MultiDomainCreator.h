#ifndef MANTID_CURVEFITTING_MULTIDOMAINCREATOR_H_
#define MANTID_CURVEFITTING_MULTIDOMAINCREATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IDomainCreator.h"

namespace Mantid
{

  namespace CurveFitting
  {
    /**
    New algorithm for fitting functions. The name is temporary.

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
    class DLLExport MultiDomainCreator : public IDomainCreator
    {
      /// A friend that can create instances of this class
      friend class Fit;
      /// Constructor
      MultiDomainCreator(API::Algorithm* fit, size_t n):
      IDomainCreator(fit),
      m_creators(n),
      m_workspacePropertyNames(n)
      {
      }

      /// Create a domain from the input workspace
      virtual void createDomain(
        const std::vector<std::string>& workspacePropetyNames,
        boost::shared_ptr<API::FunctionDomain>& domain, 
        boost::shared_ptr<API::IFunctionValues>& values, size_t i0);

      void setCreator(size_t i, const std::string& workspacePropetyName,IDomainCreator* creator);
      bool hasCreator(size_t i) const;

      /// Vector of creators.
      std::vector< boost::shared_ptr<IDomainCreator> > m_creators;
      /// Workspace property names
      std::vector<std::string> m_workspacePropertyNames;
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_MULTIDOMAINCREATOR_H_*/
