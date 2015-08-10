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
  LoadEventAndCompress();
  virtual ~LoadEventAndCompress();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

protected:
  API::ITableWorkspace_sptr determineChunk(const std::string &filename);
  API::MatrixWorkspace_sptr loadChunk(const size_t rowIndex);
  void processChunk(API::MatrixWorkspace_sptr wksp);

private:
  void init();
  void exec();

  std::string m_filename;
  double m_filterBadPulses;
  API::ITableWorkspace_sptr m_chunkingTable;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_LOADEVENTANDCOMPRESS_H_ */
