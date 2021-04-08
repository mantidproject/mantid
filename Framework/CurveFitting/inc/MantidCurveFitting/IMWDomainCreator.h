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
#include "MantidKernel/cow_ptr.h"

#include <list>
#include <memory>
#include <utility>

namespace Mantid {
namespace API {
class FunctionDomain;
class FunctionDomain1D;
class FunctionValues;
class MatrixWorkspace;
} // namespace API

namespace CurveFitting {

/**
A base class for domain creators taking 1D data from a spectrum of a matrix
workspace.
*/
class MANTID_CURVEFITTING_DLL IMWDomainCreator : public API::IDomainCreator {
public:
  /// Constructor
  IMWDomainCreator(Kernel::IPropertyManager *fit, const std::string &workspacePropertyName,
                   DomainType domainType = Simple);
  /// Declare properties that specify the dataset within the workspace to fit
  /// to.
  void declareDatasetProperties(const std::string &suffix = "", bool addProp = true) override;
  /// Create an output workspace.
  std::shared_ptr<API::Workspace> createOutputWorkspace(const std::string &baseName, API::IFunction_sptr function,
                                                        std::shared_ptr<API::FunctionDomain> domain,
                                                        std::shared_ptr<API::FunctionValues> values,
                                                        const std::string &outputWorkspacePropertyName) override;
  /// Return the size of the domain to be created.
  size_t getDomainSize() const override;
  /// Initialize the function
  void initFunction(API::IFunction_sptr function) override;
  /// Set the workspace
  /// @param ws :: workspace to set.
  void setWorkspace(std::shared_ptr<API::MatrixWorkspace> ws) { m_matrixWorkspace = std::move(ws); }
  /// Set the workspace index
  /// @param wi :: workspace index to set.
  void setWorkspaceIndex(size_t wi) { m_workspaceIndex = wi; }
  /// Set the startX and endX
  /// @param startX :: Start of the domain
  /// @param endX :: End of the domain
  void setRange(double startX, double endX) {
    m_startX = startX;
    m_endX = endX;
  }

protected:
  /// Calculate size and starting iterator in the X array
  std::pair<size_t, size_t> getXInterval() const;
  /// Set all parameters
  virtual void setParameters() const;

  /// Creates the blank output workspace of the correct size
  std::shared_ptr<API::MatrixWorkspace> createEmptyResultWS(const size_t nhistograms, const size_t nyvalues);
  /// Set initial values for parameters with default values.
  void setInitialValues(API::IFunction &function);
  // Unrolls function into its constituent parts if it is a composite and adds
  // it to the list. Note this is recursive
  void appendCompositeFunctionMembers(std::list<API::IFunction_sptr> &functionList,
                                      const API::IFunction_sptr &function) const;
  // Create separate Convolutions for each component of the model of a
  // convolution
  void appendConvolvedCompositeFunctionMembers(std::list<API::IFunction_sptr> &functionList,
                                               const API::IFunction_sptr &function) const;
  /// Add the calculated function values to the workspace
  void addFunctionValuesToWS(const API::IFunction_sptr &function, std::shared_ptr<API::MatrixWorkspace> &ws,
                             const size_t wsIndex, const std::shared_ptr<API::FunctionDomain> &domain,
                             const std::shared_ptr<API::FunctionValues> &resultValues) const;

  /// Store workspace property name
  std::string m_workspacePropertyName;
  /// Store workspace index property name
  std::string m_workspaceIndexPropertyName;
  /// Store startX property name
  std::string m_startXPropertyName;
  /// Store endX property name
  std::string m_endXPropertyName;

  /// The input MareixWorkspace
  mutable std::shared_ptr<API::MatrixWorkspace> m_matrixWorkspace;
  /// The workspace index
  mutable size_t m_workspaceIndex;
  /// startX
  mutable double m_startX;
  /// endX
  mutable double m_endX;
  /// Store the created domain and values
  mutable std::weak_ptr<API::FunctionDomain1D> m_domain;
  mutable std::weak_ptr<API::FunctionValues> m_values;
  size_t m_startIndex;
};

} // namespace CurveFitting
} // namespace Mantid
