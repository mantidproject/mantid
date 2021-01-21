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
#include <utility>

#include "MantidAPI/IDomainCreator.h"

namespace Mantid {

namespace API {
class FunctionDomain;
class FunctionDomainMD;
class IMDWorkspace;
class IMDEventWorkspace;
class IMDHistoWorkspace;
} // namespace API

namespace MDAlgorithms {
/**
Creates FunctionDomainMD from an IMDWorkspace. Does not create any properties.

@author Roman Tolchenov, Tessella plc
@date 06/12/2011
 */
class DLLExport FitMD : public API::IDomainCreator {
public:
  /// Default constructor
  FitMD();
  /// Constructor
  FitMD(Kernel::IPropertyManager *fit, const std::string &workspacePropertyName, DomainType domainType = Simple);
  /// Constructor
  FitMD(DomainType domainType)
      : API::IDomainCreator(nullptr, std::vector<std::string>(), domainType), m_startIndex(0), m_count(0) {}
  /// Initialize
  void initialize(Kernel::IPropertyManager *pm, const std::string &workspacePropertyName,
                  DomainType domainType) override;

  /// declare properties that specify the dataset within the workspace to fit
  /// to.
  void declareDatasetProperties(const std::string &suffix = "", bool addProp = true) override;
  /// Create a domain from the input workspace
  void createDomain(std::shared_ptr<API::FunctionDomain> &, std::shared_ptr<API::FunctionValues> &, size_t i0) override;
  std::shared_ptr<API::Workspace>
  createOutputWorkspace(const std::string &baseName, API::IFunction_sptr function,
                        std::shared_ptr<API::FunctionDomain> domain, std::shared_ptr<API::FunctionValues> values,
                        const std::string &outputWorkspacePropertyName = "OutputWorkspace") override;

  /// Return the size of the domain to be created.
  size_t getDomainSize() const override;
  /// Set the workspace
  void setWorkspace(std::shared_ptr<API::IMDWorkspace> IMDWorkspace) { m_IMDWorkspace = std::move(IMDWorkspace); }
  /// Set the range
  void setRange(size_t startIndex, size_t count);
  /// Set max size for Sequantial and Parallel domains
  /// @param maxSize :: Maximum size of each simple domain
  void setMaxSize(size_t maxSize) { m_maxSize = maxSize; }

protected:
  /// Set all parameters
  void setParameters() const;
  /// Create event output workspace
  std::shared_ptr<API::Workspace> createEventOutputWorkspace(const std::string &baseName,
                                                             const API::IMDEventWorkspace &inputWorkspace,
                                                             const API::FunctionValues &values,
                                                             const std::string &outputWorkspacePropertyName);
  /// Create histo output workspace
  std::shared_ptr<API::Workspace>
  createHistoOutputWorkspace(const std::string &baseName, const API::IFunction_sptr &function,
                             const std::shared_ptr<const API::IMDHistoWorkspace> &inputWorkspace,
                             const std::string &outputWorkspacePropertyName);

  /// Store workspace property name
  std::string m_workspacePropertyName;
  /// Store maxSize property name
  std::string m_maxSizePropertyName;
  /// The input IMDWorkspace
  mutable std::shared_ptr<API::IMDWorkspace> m_IMDWorkspace;
  /// Max size for seq domain
  mutable size_t m_maxSize;
  /// Starting index
  size_t m_startIndex;
  /// Size of the domain if part of the workspace is used
  size_t m_count;
};

} // namespace MDAlgorithms
} // namespace Mantid
