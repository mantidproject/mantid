#ifndef MANTID_CURVEFITTING_IFIT_H_
#define MANTID_CURVEFITTING_IFIT_H_

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
    class FunctionValues;
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
    class DLLExport IFit : public API::Algorithm
    {
    public:
      /// Default constructor
      IFit() : API::Algorithm(),m_function() {};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Optimization";}

    protected:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      // Overridden Algorithm methods
      void init();
      void exec();

      /// declare properties that specify the dataset within the workspace to fit to.
      virtual void declareDatasetProperties() {}
      /// Create a domain from the input workspace
      virtual void createDomain(boost::shared_ptr<API::FunctionDomain>&, boost::shared_ptr<API::FunctionValues>&) = 0;
      /// Create an output workspace filled with data simulated with the fitting function
      virtual void createOutputWorkspace(
        const std::string& baseName,
        boost::shared_ptr<API::FunctionDomain> domain,
        boost::shared_ptr<API::FunctionValues> values) {}

      /// Pointer to the fitting function
      API::IFunction_sptr m_function;

    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_IFIT_H_*/
