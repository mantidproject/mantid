#ifndef MANTID_CURVEFITTING_IDOMAINCREATOR_H_
#define MANTID_CURVEFITTING_IDOMAINCREATOR_H_

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

    An base class for domain creators for use in Fit. Implementations create function domains
    from particular workspaces.

    @author Roman Tolchenov, Tessella plc
    @date 22/03/2012

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
    class DLLExport IDomainCreator
    {
    public:
      /// Virtual destructor
      virtual ~IDomainCreator() {};

      /// declare properties that specify the dataset within the workspace to fit to.
      virtual void declareDatasetProperties() {}
      /// Create a domain and values from the input workspace. FunctionValues must be filled with data to fit to.
      virtual void createDomain(boost::shared_ptr<API::FunctionDomain>&, boost::shared_ptr<API::FunctionValues>&) = 0;
      /// Create an output workspace filled with data simulated with the fitting function
      virtual void createOutputWorkspace(
        const std::string& baseName,
        boost::shared_ptr<API::FunctionDomain> domain,
        boost::shared_ptr<API::FunctionValues> values) {}
      virtual void initFunction();

    protected:
      /// Constructor.
      /// @param fit :: Fit algorithm this creator will create domains for
      IDomainCreator(API::Algorithm* fit):m_fit(fit){}
      /// A friend that can create instances of this class
      friend class Fit;
      /// Log as the algorithm
      Kernel::Logger& log() const;
      /// Declare a property to the algorithm
      void declareProperty(Kernel::Property* prop,const std::string& doc);
      /// Pointer to the Fit algorithm
      API::Algorithm* m_fit;
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_IDOMAINCREATOR_H_*/
