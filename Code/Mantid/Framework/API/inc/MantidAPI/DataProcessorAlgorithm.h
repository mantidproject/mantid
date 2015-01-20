#ifndef MANTID_API_DATAPROCESSORALGORITHM_H_
#define MANTID_API_DATAPROCESSORALGORITHM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidKernel/PropertyManager.h"
#include <vector>

namespace Mantid {
namespace API {
/**

   Data processor algorithm to be used as a parent to workflow algorithms.
   This algorithm provides utility methods to load and process data.

   @date 2012-04-04

   Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport DataProcessorAlgorithm : public Algorithm {
public:
  DataProcessorAlgorithm();
  virtual ~DataProcessorAlgorithm();

protected:
  virtual boost::shared_ptr<Algorithm> createChildAlgorithm(
      const std::string &name, const double startProgress = -1.,
      const double endProgress = -1., const bool enableLogging = true,
      const int &version = -1);
  void setLoadAlg(const std::string &alg);
  void setLoadAlgFileProp(const std::string &filePropName);
  void setAccumAlg(const std::string &alg);
  ITableWorkspace_sptr determineChunk();
  void loadChunk();
  Workspace_sptr load(const std::string &inputData,
                      const bool loadQuiet = false);
  std::vector<std::string> splitInput(const std::string &input);
  void forwardProperties();
  boost::shared_ptr<Kernel::PropertyManager>
  getProcessProperties(const std::string &propertyManager);
  /// MPI option. If false, we will use one job event if MPI is available
  bool m_useMPI;
  Workspace_sptr assemble(const std::string &partialWSName,
                          const std::string &outputWSName);
  void saveNexus(const std::string &outputWSName,
                 const std::string &outputFile);
  bool isMainThread();
  int getNThreads();

  /// Divide a matrix workspace by another matrix workspace
  MatrixWorkspace_sptr divide(const MatrixWorkspace_sptr lhs,
                              const MatrixWorkspace_sptr rhs);
  /// Divide a matrix workspace by a single value
  MatrixWorkspace_sptr divide(const MatrixWorkspace_sptr lhs,
                              const double &rhsValue);

  /// Multiply a matrix workspace by another matrix workspace
  MatrixWorkspace_sptr multiply(const MatrixWorkspace_sptr lhs,
                                const MatrixWorkspace_sptr rhs);
  /// Multiply a matrix workspace by a single value
  MatrixWorkspace_sptr multiply(const MatrixWorkspace_sptr lhs,
                                const double &rhsValue);

  /// Add a matrix workspace to another matrix workspace
  MatrixWorkspace_sptr plus(const MatrixWorkspace_sptr lhs,
                            const MatrixWorkspace_sptr rhs);
  /// Add a single value to a matrix workspace
  MatrixWorkspace_sptr plus(const MatrixWorkspace_sptr lhs,
                            const double &rhsValue);

  /// Subract a matrix workspace by another matrix workspace
  MatrixWorkspace_sptr minus(const MatrixWorkspace_sptr lhs,
                             const MatrixWorkspace_sptr rhs);
  /// Subract a single value from a matrix workspace
  MatrixWorkspace_sptr minus(const MatrixWorkspace_sptr lhs,
                             const double &rhsValue);

private:
  template <typename LHSType, typename RHSType, typename ResultType>
  ResultType executeBinaryAlgorithm(const std::string &algorithmName,
                                    const LHSType lhs, const RHSType rhs) {
    auto alg = createChildAlgorithm(algorithmName);
    alg->initialize();

    alg->setProperty<LHSType>("LHSWorkspace", lhs);
    alg->setProperty<RHSType>("RHSWorkspace", rhs);
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
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_DATAPROCESSORALGORITHM_H_ */
