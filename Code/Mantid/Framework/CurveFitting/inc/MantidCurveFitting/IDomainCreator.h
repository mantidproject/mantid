#ifndef MANTID_CURVEFITTING_IDOMAINCREATOR_H_
#define MANTID_CURVEFITTING_IDOMAINCREATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IPropertyManager.h"
#include "MantidAPI/IFunction.h"

namespace Mantid
{

  namespace API
  {
    class FunctionDomain;
    class IFunctionValues;
  }

  namespace CurveFitting
  {
    /**

    An base class for domain creators for use in Fit. Implementations create function domains
    from particular workspaces. Domain creators are instantiated by Fit algorithm and are responsible
    for declaring Fit's dynamic properties. Derived creators can implement createOutput method to
    declare the OutputWorkspace property for comparing the fitted and calculated data.

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
      /// Constructor.
      /// @param manager :: A property manager which has information about the data source (eg workspace)
      /// and the function.
      /// @param workspacePropertyNames :: Property names for workspaces to get the data from.
      IDomainCreator( Kernel::IPropertyManager* manager,
        const std::vector<std::string>& workspacePropertyNames ):
      m_manager(manager),
      m_workspacePropertyNames(workspacePropertyNames)
      {}
      /// Virtual destructor
      virtual ~IDomainCreator() {};

      /// declare properties that specify the dataset within the workspace to fit to.
      /// @param suffix :: A suffix to give to all new properties.
      /// @param addProp :: If false don't actually declare new properties but do other stuff if needed
      virtual void declareDatasetProperties(const std::string& suffix = "",bool addProp = true) 
      {UNUSED_ARG(suffix);UNUSED_ARG(addProp);}

      /// Create a domain and values from the input workspace. FunctionValues must be filled with data to fit to.
      /// @param workspacePropetyName :: A name of a workspace property. Domain will be created for this workspace.
      /// @param domain :: Shared pointer to hold the created domain
      /// @param values :: Shared pointer to hold the created values with set fitting data and weights.
      ///  Implementations must check whether it's empty or not. If values pointer is empty create new values instance
      ///  of an appropriate type otherwise extend it if neccessary.
      /// @param i0 :: Starting index in values for the fitting data. Implementations must make sure values has enough room
      ///   for the data from index i0 to the end of the container.
      virtual void createDomain(
        boost::shared_ptr<API::FunctionDomain>& domain, 
        boost::shared_ptr<API::IFunctionValues>& values,
        size_t i0 = 0) = 0;

      /**
       * Create an output workspace filled with data simulated with the fitting function.
       */
      virtual void createOutputWorkspace(
        const std::string& baseName,
        API::IFunction_sptr function,
        boost::shared_ptr<API::FunctionDomain> domain,
        boost::shared_ptr<API::IFunctionValues> values) 
      {UNUSED_ARG(baseName);UNUSED_ARG(function);UNUSED_ARG(domain);UNUSED_ARG(values);}

      /// Initialize the function
      virtual void initFunction(API::IFunction_sptr function);

      /// Return the size of the domain to be created.
      virtual size_t getDomainSize() const = 0;

    protected:
      /// Declare a property to the algorithm
      void declareProperty(Kernel::Property* prop,const std::string& doc);
      /// Pointer to a property manager
      Kernel::IPropertyManager* m_manager;
      /// Property names for workspaces to get the data from
      std::vector<std::string> m_workspacePropertyNames;
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_IDOMAINCREATOR_H_*/
