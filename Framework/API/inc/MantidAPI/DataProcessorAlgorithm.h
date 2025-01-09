// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IEventWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/PropertyManager.h"
#include <vector>

namespace Mantid {
namespace API {
/**

   Data processor algorithm to be used as a parent to workflow algorithms.
   This algorithm provides utility methods to load and process data.

   @date 2012-04-04
 */
template <class Base> class MANTID_API_DLL GenericDataProcessorAlgorithm : public Base {
public:
  GenericDataProcessorAlgorithm();
  std::string getPropertyValue(const std::string &name) const override;
  Kernel::IPropertyManager::TypedValue getProperty(const std::string &name) const override;

protected:
  std::shared_ptr<Algorithm> createChildAlgorithm(const std::string &name, const double startProgress = -1.,
                                                  const double endProgress = -1., const bool enableLogging = true,
                                                  const int &version = -1) override;
  void setLoadAlg(const std::string &alg);
  void setLoadAlgFileProp(const std::string &filePropName);
  void setAccumAlg(const std::string &alg);
  void setPropManagerPropName(const std::string &propName);
  void mapPropertyName(const std::string &nameInProp, const std::string &nameInPropManager);
  void copyProperty(const API::Algorithm_sptr &alg, const std::string &name);
  virtual ITableWorkspace_sptr determineChunk(const std::string &filename);
  virtual MatrixWorkspace_sptr loadChunk(const size_t rowIndex);
  Workspace_sptr load(const std::string &inputData, const bool loadQuiet = false);
  std::vector<std::string> splitInput(const std::string &input);
  void forwardProperties();
  std::shared_ptr<Kernel::PropertyManager>
  getProcessProperties(const std::string &propertyManager = std::string()) const;
  void saveNexus(const std::string &outputWSName, const std::string &outputFile);

  /// Divide a matrix workspace by another matrix workspace
  MatrixWorkspace_sptr divide(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
  /// Divide a matrix workspace by a single value
  MatrixWorkspace_sptr divide(const MatrixWorkspace_sptr lhs, const double &rhsValue);

  /// Multiply a matrix workspace by another matrix workspace
  MatrixWorkspace_sptr multiply(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
  /// Multiply a matrix workspace by a single value
  MatrixWorkspace_sptr multiply(const MatrixWorkspace_sptr lhs, const double &rhsValue);

  /// Add a matrix workspace to another matrix workspace
  MatrixWorkspace_sptr plus(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
  /// Add a single value to a matrix workspace
  MatrixWorkspace_sptr plus(const MatrixWorkspace_sptr lhs, const double &rhsValue);

  /// Subract a matrix workspace by another matrix workspace
  MatrixWorkspace_sptr minus(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
  /// Subract a single value from a matrix workspace
  MatrixWorkspace_sptr minus(const MatrixWorkspace_sptr lhs, const double &rhsValue);

private:
  template <typename LHSType, typename RHSType, typename ResultType>
  ResultType executeBinaryAlgorithm(const std::string &algorithmName, const LHSType lhs, const RHSType rhs) {
    auto alg = createChildAlgorithm(algorithmName);
    alg->initialize();

    alg->template setProperty<LHSType>("LHSWorkspace", lhs);
    alg->template setProperty<RHSType>("RHSWorkspace", rhs);
    alg->execute();

    if (alg->isExecuted()) {
      // Get the output workspace property
      return alg->getProperty("OutputWorkspace");
    } else {
      std::string message = "Error while executing operation: " + algorithmName;
      throw std::runtime_error(message);
    }
  }

  /// Create a matrix workspace from a single number
  MatrixWorkspace_sptr createWorkspaceSingleValue(const double &rhsValue);

  /// The name of the algorithm to invoke when loading data
  std::string m_loadAlg;
  /// The name of the algorithm to invoke when accumulating data chunks
  std::string m_accumulateAlg;
  /// An alternate filename property for the load algorithm
  std::string m_loadAlgFileProp;
  /// The name of the parameter that names the property manager. The default
  /// value is "ReductionProperties".
  std::string m_propertyManagerPropertyName;
  /// Map property names to names in supplied properties manager
  std::map<std::string, std::string> m_nameToPMName;

  // This method is a workaround for the C4661 compiler warning in visual
  // studio. This allows the template declaration and definition to be separated
  // in different files. See stack overflow article for a more detailed
  // explanation:
  // https://stackoverflow.com/questions/44160467/warning-c4661no-suitable-definition-provided-for-explicit-template-instantiatio
  // https://stackoverflow.com/questions/33517902/how-to-export-a-class-derived-from-an-explicitly-instantiated-template-in-visual
  void visualStudioC4661Workaround();
};

template <> MANTID_API_DLL void GenericDataProcessorAlgorithm<Algorithm>::visualStudioC4661Workaround();

using DataProcessorAlgorithm = GenericDataProcessorAlgorithm<Algorithm>;
} // namespace API
} // namespace Mantid
