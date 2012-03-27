#ifndef MANTID_CURVEFITTING_FIT_H_
#define MANTID_CURVEFITTING_FIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/Workspace.h"
#include "MantidCurveFitting/IDomainCreator.h"

#include <boost/scoped_ptr.hpp>

namespace Mantid
{

  namespace API
  {
    class FunctionDomain;
    class IFunctionValues;
    class Workspace;
  }

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
    class DLLExport Fit : public API::Algorithm
    {
    public:
      /// Default constructor
      Fit() : API::Algorithm(),m_workspaceCount(0) {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Fit";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Optimization";}

    protected:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      // Overridden Algorithm methods
      void init();
      void exec();
      /// Override this method to perform a custom action right after a property was set.
      /// The argument is the property name. Default - do nothing.
      virtual void afterPropertySet(const std::string& propName);
      void setFunction();
      void addWorkspace(const std::string& workspaceNameProperty, bool addProperties = true);

      /// Pointer to the fitting function
      API::IFunction_sptr m_function;
      ///// Pointer 
      //API::Workspace_const_sptr m_workspace;
      /// Pointer to a domain creator
      boost::scoped_ptr<IDomainCreator> m_domainCreator;
      friend class IDomainCreator;
      size_t m_workspaceCount;

    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FIT_H_*/
