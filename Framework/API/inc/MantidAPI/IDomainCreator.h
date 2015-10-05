#ifndef MANTID_API_IDOMAINCREATOR_H_
#define MANTID_API_IDOMAINCREATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IPropertyManager.h"
#include "MantidAPI/DomainCreatorFactory.h"
#include "MantidAPI/IFunction.h"

namespace Mantid {

namespace API {
class FunctionDomain;
class FunctionValues;
class Workspace;
}

namespace API {
/**

An base class for domain creators for use in Fit. Implementations create
function domains
from particular workspaces. Domain creators are instantiated by Fit algorithm
and are responsible
for declaring Fit's dynamic properties. Derived creators can implement
createOutput method to
declare the OutputWorkspace property for comparing the fitted and calculated
data.

@author Roman Tolchenov, Tessella plc
@date 22/03/2012

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IDomainCreator {
public:
  /// Type of domain to create.
  enum DomainType { Simple = 0, Sequential, Parallel };
  /// Constrcutor
  IDomainCreator(Kernel::IPropertyManager *manager,
                 const std::vector<std::string> &workspacePropertyNames,
                 DomainType domainType = Simple);
  /// Virtual destructor
  virtual ~IDomainCreator() {}
  /// Initialize
  virtual void initialize(Kernel::IPropertyManager *, const std::string &,
                          DomainType) {}

  /// Toggle output of either just composite or composite + members
  void separateCompositeMembersInOutput(const bool value,
                                        const bool conv = false);

  /// Declare properties that specify the dataset within the workspace to fit
  /// to.
  /// @param suffix :: A suffix to give to all new properties.
  /// @param addProp :: If false don't actually declare new properties but do
  /// other stuff if needed
  virtual void declareDatasetProperties(const std::string &suffix = "",
                                        bool addProp = true) {
    UNUSED_ARG(suffix);
    UNUSED_ARG(addProp);
  }

  /// Create a domain and values from the input workspace. FunctionValues must
  /// be filled with data to fit to.
  /// @param domain :: Shared pointer to hold the created domain
  /// @param values :: Shared pointer to hold the created values with set
  /// fitting data and weights.
  ///  Implementations must check whether it's empty or not. If values pointer
  ///  is empty create new values instance
  ///  of an appropriate type otherwise extend it if neccessary.
  /// @param i0 :: Starting index in values for the fitting data.
  /// Implementations must make sure values has enough room
  ///   for the data from index i0 to the end of the container.
  virtual void createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                            boost::shared_ptr<API::FunctionValues> &values,
                            size_t i0 = 0) = 0;

  /// Create an output workspace filled with data simulated with the fitting
  /// function.
  /// @param baseName :: Specifies the name of the output workspace
  /// @param function :: A pointer to the fitting function
  /// @param domain :: The domain containing x-values for the function
  /// @param values :: A FunctionValues instance containing the fitting data
  /// @param outputWorkspacePropertyName :: Name of the property to declare and
  /// set to the created output workspace.
  ///                                       If empty do not create the property,
  ///                                       just return a pointer
  /// @return A shared pointer to the created workspace.
  virtual boost::shared_ptr<API::Workspace> createOutputWorkspace(
      const std::string &baseName, API::IFunction_sptr function,
      boost::shared_ptr<API::FunctionDomain> domain,
      boost::shared_ptr<API::FunctionValues> values,
      const std::string &outputWorkspacePropertyName = "OutputWorkspace") {
    UNUSED_ARG(baseName);
    UNUSED_ARG(function);
    UNUSED_ARG(domain);
    UNUSED_ARG(values);
    UNUSED_ARG(outputWorkspacePropertyName);
    throw std::logic_error("Method createOutputWorkspace() isn't implemented");
  }
  /// Initialize the function
  /// @param function :: A function to initialize.
  virtual void initFunction(API::IFunction_sptr function);

  /// Return the size of the domain to be created.
  virtual size_t getDomainSize() const = 0;

  /// Set to ignore invalid data
  void ignoreInvalidData(bool yes) { m_ignoreInvalidData = yes; }

protected:
  /// Declare a property to the algorithm
  void declareProperty(Kernel::Property *prop, const std::string &doc);
  /// Pointer to a property manager
  Kernel::IPropertyManager *m_manager;
  /// Property names for workspaces to get the data from
  std::vector<std::string> m_workspacePropertyNames;
  /// Domain type
  DomainType m_domainType;
  /// Output separate composite function values
  bool m_outputCompositeMembers;
  /// Perform convolution of output composite components
  bool m_convolutionCompositeMembers;
  /// Flag to ignore nans, infinities and zero errors.
  bool m_ignoreInvalidData;
};

/// Typedef for a shared pointer to IDomainCreator.
typedef boost::shared_ptr<IDomainCreator> IDomainCreator_sptr;

} // namespace API
} // namespace Mantid

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 * The id is the key that should be used to create the object
 */
#define DECLARE_DOMAINCREATOR(classname)                                       \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper                                           \
      register_alg_##classname(((Mantid::API::DomainCreatorFactory::Instance() \
                                     .subscribe<classname>(#classname)),       \
                                0));                                           \
  }

#endif /*MANTID_API_IDOMAINCREATOR_H_*/
