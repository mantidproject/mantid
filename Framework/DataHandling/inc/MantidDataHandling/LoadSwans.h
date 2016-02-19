#ifndef MANTID_DATAHANDLING_LOADSWANS_H_
#define MANTID_DATAHANDLING_LOADSWANS_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidAPI/Algorithm.h"
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
class MANTID_DATAHANDLING_DLL LoadSwans : public API::Algorithm {
public:
  LoadSwans();
  virtual ~LoadSwans();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

private:
  void init();
  void exec();
  std::map<uint32_t, std::vector<uint32_t>> loadData();
  void loadDataIntoTheWorkspace(
      const std::map<uint32_t, std::vector<uint32_t>> &pos_tof_map);
  void setTimeAxis();
  void loadInstrument();
  void placeDetectorInSpace();
  unsigned int setDetectorSize();

  // Member variables
  DataObjects::EventWorkspace_sptr m_ws;
  unsigned int m_detector_size;

  // Constants:
  // This has to be here because the data file to date has no metadata
  const std::string m_instrumentName = "SWANS";
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADSWANS_H_ */
