// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDomainCreator.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {

namespace CurveFitting {
/**
Creates a composite domain.

@author Roman Tolchenov, Tessella plc
@date 06/12/2011
*/
class MANTID_CURVEFITTING_DLL MultiDomainCreator : public API::IDomainCreator {
  /// A friend that can create instances of this class
  // friend class Fit;
public:
  /// Constructor
  MultiDomainCreator(Kernel::IPropertyManager *fit, const std::vector<std::string> &workspacePropertyNames)
      : API::IDomainCreator(fit, workspacePropertyNames), m_creators(workspacePropertyNames.size()) {}

  /// Create a domain from the input workspace
  void createDomain(std::shared_ptr<API::FunctionDomain> &domain, std::shared_ptr<API::FunctionValues> &ivalues,
                    size_t i0 = 0) override;
  /// Create the output workspace
  std::shared_ptr<API::Workspace> createOutputWorkspace(const std::string &baseName, API::IFunction_sptr function,
                                                        std::shared_ptr<API::FunctionDomain> domain,
                                                        std::shared_ptr<API::FunctionValues> values,
                                                        const std::string &outputWorkspacePropertyName) override;

  /// Return the size of the domain to be created.
  size_t getDomainSize() const override { return 0; }
  /// Set ith creator
  void setCreator(size_t i, API::IDomainCreator *creator);
  bool hasCreator(size_t i) const;
  /// Get number of creators
  size_t getNCreators() const { return m_creators.size(); }
  /// Initialize the function
  void initFunction(API::IFunction_sptr function) override;

protected:
  /// Vector of creators.
  std::vector<std::shared_ptr<API::IDomainCreator>> m_creators;
};

} // namespace CurveFitting
} // namespace Mantid
