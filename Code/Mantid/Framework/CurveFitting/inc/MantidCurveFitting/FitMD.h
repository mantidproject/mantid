#ifndef MANTID_CURVEFITTING_FITMD_H_
#define MANTID_CURVEFITTING_FITMD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IFit.h"

namespace Mantid
{

  namespace API
  {
    class FunctionDomain;
    class FunctionDomainMD;
    class FunctionValues;
    class IMDWorkspace;
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
    class DLLExport FitMD : public IFit
    {
    public:
      /// Default constructor
      FitMD() : IFit() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "FitMD";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Optimization";}

    protected:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();

      /// declare properties that specify the dataset within the workspace to fit to.
      virtual void declareDatasetProperties(){}
      /// Create a domain from the input workspace
      virtual void createDomain(boost::shared_ptr<API::FunctionDomain>&, boost::shared_ptr<API::FunctionValues>&);

      /// The input MareixWorkspace
      boost::shared_ptr<API::IMDWorkspace> m_IMDWorkspace;
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FITMD_H_*/
