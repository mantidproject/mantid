#ifndef MANTID_CURVEFITTING_FITMW_H_
#define MANTID_CURVEFITTING_FITMW_H_

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
    class FunctionDomain1D;
    class FunctionValues;
    class MatrixWorkspace;
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
    class DLLExport FitMW : public API::Algorithm
    {
    public:
      /// Default constructor
      FitMW() : API::Algorithm(),m_function() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "FitMW";}
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

      boost::shared_ptr<API::MatrixWorkspace> createOutputWorkspace(
        boost::shared_ptr<const API::MatrixWorkspace> inWS,
        size_t wi,
        size_t startIndex,
        boost::shared_ptr<API::FunctionDomain1D> domain,
        boost::shared_ptr<API::FunctionValues> values
        );
      /// calculates the derivative of a declared parameter over active parameter i
      double transformationDerivative(int i);

      /// Pointer to the fitting function
      API::IFunction_sptr m_function;

    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FITMW_H_*/
