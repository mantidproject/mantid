#ifndef MANTID_WORKFLOWALGORITHMS_LOADEVENTANDCOMPRESS_H_
#define MANTID_WORKFLOWALGORITHMS_LOADEVENTANDCOMPRESS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** LoadEventAndCompress : TODO: DESCRIPTION

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadEventAndCompress : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadEventNexus", "CompressEvents"};
  }
  const std::string category() const override;
  const std::string summary() const override;

protected:
  API::ITableWorkspace_sptr
  determineChunk(const std::string &filename) override;
  API::MatrixWorkspace_sptr loadChunk(const size_t rowIndex) override;
  API::MatrixWorkspace_sptr processChunk(API::MatrixWorkspace_sptr &wksp,
                                         double filterBadPulses);

  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;

private:
  void init() override;
  void exec() override;

  API::ITableWorkspace_sptr m_chunkingTable;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_LOADEVENTANDCOMPRESS_H_ */
