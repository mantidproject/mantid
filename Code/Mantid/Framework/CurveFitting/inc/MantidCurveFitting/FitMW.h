#ifndef MANTID_CURVEFITTING_FITMW_H_
#define MANTID_CURVEFITTING_FITMW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IDomainCreator.h"

namespace Mantid
{

  namespace API
  {
    class FunctionDomain;
    class FunctionDomain1D;
    class IFunctionValues;
    class MatrixWorkspace;
  }

  namespace CurveFitting
  {
    /**
    Creates FunctionDomain1D form a spectrum in a MatrixWorkspace.
    Declares WorkspaceIndex, StartX, and EndX input properties.
    Declares OutputWorkspace output property.

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
    class DLLExport FitMW : public IDomainCreator
    {
    public:
      /// declare properties that specify the dataset within the workspace to fit to.
      virtual void declareDatasetProperties(const std::string& suffix = "",bool addProp = true);
      /// Create a domain from the input workspace
      virtual void createDomain(
        const std::vector<std::string>& workspacePropetyNames,
        boost::shared_ptr<API::FunctionDomain>& domain, 
        boost::shared_ptr<API::IFunctionValues>& values, size_t i0 = 0);
      void createOutputWorkspace(
        const std::string& baseName,
        boost::shared_ptr<API::FunctionDomain> domain,
        boost::shared_ptr<API::IFunctionValues> values
        );
    protected:
      /// Constructor
      FitMW(API::Algorithm* fit):IDomainCreator(fit){}
      /// A friend that can create instances of this class
      friend class Fit;

      /// Store workspace property name
      std::string m_workspacePropertyName;
      /// Store workspace index property name
      std::string m_workspaceIndexPropertyName;
      /// Store startX property name
      std::string m_startXPropertyName;
      /// Store endX property name
      std::string m_endXPropertyName;

      /// Pointer to the fitting function
      API::IFunction_sptr m_function;
      /// The input MareixWorkspace
      boost::shared_ptr<API::MatrixWorkspace> m_matrixWorkspace;
      /// The workspace index
      size_t m_workspaceIndex;
      size_t m_startIndex;
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FITMW_H_*/
