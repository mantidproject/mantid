#ifndef MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUS_H_
#define MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** AppendGeometryToSNSNexus : Appends geometry information to a NeXus file.

  @date 2012-06-01

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
class DLLExport AppendGeometryToSNSNexus : public API::Algorithm {
public:
  AppendGeometryToSNSNexus();
  ~AppendGeometryToSNSNexus() override;

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Appends the resolved instrument geometry (detectors and monitors "
           "for now) to a SNS ADARA NeXus file.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  /// The filename of the NeXus file to append geometry info to
  std::string m_filename;

  /// Instrument name
  std::string m_instrument;

  /// IDF filename
  std::string m_idf_filename;

  /// Get the instrument name from the NeXus file
  std::string getInstrumentName(const std::string &nxfilename);

  /// Run LoadInstrument as a Child Algorithm
  bool runLoadInstrument(const std::string &idf_filename,
                         API::MatrixWorkspace_sptr localWorkspace,
                         Algorithm *alg);

  /// Load logs from the NeXus file
  static bool runLoadNexusLogs(const std::string &nexusFileName,
                               API::MatrixWorkspace_sptr localWorkspace,
                               Algorithm *alg);

  /// Are we going to make a copy of the NeXus file to operate on ?
  bool m_makeNexusCopy;

  /// The workspace to load instrument and logs
  API::MatrixWorkspace_sptr ws;

  /// Was the instrument loaded?
  bool m_instrumentLoadedCorrectly;

  /// Were the logs loaded?
  bool m_logsLoadedCorrectly;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUS_H_ */
