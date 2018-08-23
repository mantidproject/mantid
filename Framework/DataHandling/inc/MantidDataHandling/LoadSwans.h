#ifndef MANTID_DATAHANDLING_LOADSWANS_H_
#define MANTID_DATAHANDLING_LOADSWANS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <map>

namespace Mantid {
namespace DataHandling {

/** LoadSwans : Test Loader to read data from the LDRD new SWANS detector

 Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadSwans final
    : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  LoadSwans();
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;
  std::map<uint32_t, std::vector<uint32_t>> loadData();
  std::vector<double> loadMetaData();
  void setMetaDataAsWorkspaceProperties(const std::vector<double> &metadata);
  void loadDataIntoTheWorkspace(
      const std::map<uint32_t, std::vector<uint32_t>> &eventMap);
  void setTimeAxis();
  void loadInstrument();
  void placeDetectorInSpace();
  unsigned int getDetectorSize();

  // Member variables
  DataObjects::EventWorkspace_sptr m_ws =
      boost::make_shared<Mantid::DataObjects::EventWorkspace>();
  unsigned int m_detector_size = 0;

  // Constants:
  // This has to be here because the data file to date has no metadata
  const std::string m_instrumentName = "SWANS";
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADSWANS_H_ */
