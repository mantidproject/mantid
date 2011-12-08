#ifndef MANTID_CURVEFITTING_NEWFIT_H_
#define MANTID_CURVEFITTING_NEWFIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"

namespace Mantid
{

  namespace API
  {
    class FunctionDomain;
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
    class DLLExport NewFit : public API::Algorithm
    {
    public:
      /// Default constructor
      NewFit() : API::Algorithm(),m_function() {};
      /// Destructor
      virtual ~NewFit();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "NewFit";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Optimization";}

      virtual void setPropertyValue(const std::string &name, const std::string &value);


    protected:
      enum DomainType {Vector, MatrixWorkspace};
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      // Overridden Algorithm methods
      void init();
      void exec();

      /// calculates the derivative of a declared parameter over active parameter i
      double transformationDerivative(int i);
      void declareWorkspaceDomainProperties();
      void declareVectorDomainProperties();
      API::FunctionDomain* createDomain() const;
      void setWorkspace();

      /// Pointer to the fitting function
      boost::shared_ptr<API::IFunction> m_function;
      DomainType m_domainType;

    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_NEWFIT_H_*/
