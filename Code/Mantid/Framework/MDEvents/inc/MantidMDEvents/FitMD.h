#ifndef MANTID_MDEVENTS_FITMD_H_
#define MANTID_MDEVENTS_FITMD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDomainCreator.h"

namespace Mantid {

namespace API {
class FunctionDomain;
class FunctionDomainMD;
class IMDWorkspace;
class IMDEventWorkspace;
class IMDHistoWorkspace;
}

namespace MDEvents {
/**
Creates FunctionDomainMD from an IMDWorkspace. Does not create any properties.

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
class DLLExport FitMD : public API::IDomainCreator {
public:
  /// Default constructor
  FitMD();
  /// Constructor
  FitMD(Kernel::IPropertyManager *fit, const std::string &workspacePropertyName,
        DomainType domainType = Simple);
  /// Constructor
  FitMD(DomainType domainType)
      : API::IDomainCreator(NULL, std::vector<std::string>(), domainType),
        m_startIndex(0), m_count(0) {}
  /// Initialize
  void initialize(Kernel::IPropertyManager *pm,
                  const std::string &workspacePropertyName,
                  DomainType domainType);

  /// declare properties that specify the dataset within the workspace to fit
  /// to.
  virtual void declareDatasetProperties(const std::string &suffix = "",
                                        bool addProp = true);
  /// Create a domain from the input workspace
  virtual void createDomain(boost::shared_ptr<API::FunctionDomain> &,
                            boost::shared_ptr<API::FunctionValues> &,
                            size_t i0);
  virtual boost::shared_ptr<API::Workspace> createOutputWorkspace(
      const std::string &baseName, API::IFunction_sptr function,
      boost::shared_ptr<API::FunctionDomain> domain,
      boost::shared_ptr<API::FunctionValues> values,
      const std::string &outputWorkspacePropertyName = "OutputWorkspace");

  /// Return the size of the domain to be created.
  virtual size_t getDomainSize() const;
  /// Set the workspace
  void setWorkspace(boost::shared_ptr<API::IMDWorkspace> IMDWorkspace) {
    m_IMDWorkspace = IMDWorkspace;
  }
  /// Set the range
  void setRange(size_t startIndex, size_t count);
  /// Set max size for Sequantial and Parallel domains
  /// @param maxSize :: Maximum size of each simple domain
  void setMaxSize(size_t maxSize) { m_maxSize = maxSize; }

protected:
  /// Set all parameters
  void setParameters() const;
  /// Create event output workspace
  boost::shared_ptr<API::Workspace>
  createEventOutputWorkspace(const std::string &baseName,
                             const API::IMDEventWorkspace &inputWorkspace,
                             const API::FunctionValues &values,
                             const std::string &outputWorkspacePropertyName);
  /// Create histo output workspace
  boost::shared_ptr<API::Workspace> createHistoOutputWorkspace(
      const std::string &baseName, API::IFunction_sptr function,
      boost::shared_ptr<const API::IMDHistoWorkspace> inputWorkspace,
      const std::string &outputWorkspacePropertyName);

  /// Store workspace property name
  std::string m_workspacePropertyName;
  /// Store maxSize property name
  std::string m_maxSizePropertyName;
  /// The input IMDWorkspace
  mutable boost::shared_ptr<API::IMDWorkspace> m_IMDWorkspace;
  /// Max size for seq domain
  mutable size_t m_maxSize;
  /// Starting index
  size_t m_startIndex;
  /// Size of the domain if part of the workspace is used
  size_t m_count;
};

} // namespace MDEvents
} // namespace Mantid

#endif /*MANTID_MDEVENTS_FITMD_H_*/
