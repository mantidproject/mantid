#ifndef MANTID_API_DATAPROCESSORALGORITHM_H_
#define MANTID_API_DATAPROCESSORALGORITHM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidKernel/PropertyManager.h"
#include <vector>

namespace Mantid
{
namespace API
{
/**

   Data processor algorithm to be used as a parent to workflow algorithms.
   This algorithm provides utility methods to load and process data.

   @date 2012-04-04

   Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport DataProcessorAlgorithm : public Algorithm
{
public:
  DataProcessorAlgorithm();
  virtual ~DataProcessorAlgorithm();

protected:
  void setLoadAlg(const std::string & alg);
  void setLoadAlgFileProp(const std::string & filePropName);
  void setAccumAlg(const std::string & alg);
  ITableWorkspace_sptr determineChunk();
  void loadChunk();
  Workspace_sptr load(const std::string &inputData);
  std::vector<std::string> splitInput(const std::string & input);
  void forwardProperties();
  boost::shared_ptr<Kernel::PropertyManager> getProcessProperties(const std::string &propertyManager);
  /// MPI option. If false, we will use one job event if MPI is available
  bool m_useMPI;
  Workspace_sptr assemble(const std::string &partialWSName, const std::string &outputWSName);
  void saveNexus(const std::string &outputWSName, const std::string &outputFile);
  bool isMainThread();
  int getNThreads();

private:
  /// The name of the algorithm to invoke when loading data
  std::string m_loadAlg;
  /// The name of the algorithm to invoke when accumulating data chunks
  std::string m_accumulateAlg;
  /// An alternate filename property for the load algorithm
  std::string m_loadAlgFileProp;
};

} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_DATAPROCESSORALGORITHM_H_ */
