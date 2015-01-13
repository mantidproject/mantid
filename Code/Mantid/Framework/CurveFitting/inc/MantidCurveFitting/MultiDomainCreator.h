#ifndef MANTID_CURVEFITTING_MULTIDOMAINCREATOR_H_
#define MANTID_CURVEFITTING_MULTIDOMAINCREATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDomainCreator.h"

namespace Mantid {

namespace CurveFitting {
/**
Creates a composite domain.

@author Roman Tolchenov, Tessella plc
@date 06/12/2011

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
class DLLExport MultiDomainCreator : public API::IDomainCreator {
  /// A friend that can create instances of this class
  // friend class Fit;
public:
  /// Constructor
  MultiDomainCreator(Kernel::IPropertyManager *fit,
                     const std::vector<std::string> &workspacePropertyNames)
      : API::IDomainCreator(fit, workspacePropertyNames),
        m_creators(workspacePropertyNames.size()) {}

  /// Create a domain from the input workspace
  virtual void createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                            boost::shared_ptr<API::FunctionValues> &values,
                            size_t i0 = 0);
  /// Create the output workspace
  boost::shared_ptr<API::Workspace>
  createOutputWorkspace(const std::string &baseName,
                        API::IFunction_sptr function,
                        boost::shared_ptr<API::FunctionDomain> domain,
                        boost::shared_ptr<API::FunctionValues> values,
                        const std::string &outputWorkspacePropertyName);

  /// Return the size of the domain to be created.
  virtual size_t getDomainSize() const { return 0; }
  /// Set ith creator
  void setCreator(size_t i, API::IDomainCreator *creator);
  bool hasCreator(size_t i) const;
  /// Get number of creators
  size_t getNCreators() const { return m_creators.size(); }
  /// Initialize the function
  void initFunction(API::IFunction_sptr function);

protected:
  /// Vector of creators.
  std::vector<boost::shared_ptr<API::IDomainCreator>> m_creators;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_MULTIDOMAINCREATOR_H_*/
