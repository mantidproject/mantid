#ifndef MANTID_MDALGORITHMS_LOADILLASCII_H_
#define MANTID_MDALGORITHMS_LOADILLASCII_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

#include "MantidMDAlgorithms/LoadILLAsciiHelper.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"

namespace Mantid {
namespace MDAlgorithms {

/** LoadILLAscii :

 This loader loads ILL data in Ascii format.
 For more details on data format, please see:
 <http://www.ill.eu/instruments-support/computing-for-science/data-analysis/raw-data/>

 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadILLAscii : public API::IFileLoader<Kernel::FileDescriptor>,
                               public API::DeprecatedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;
  void loadInstrumentName(ILLParser &);
  void loadExperimentDetails(ILLParser &p);
  void loadIDF(API::MatrixWorkspace_sptr &workspace);
  void loadsDataIntoTheWS(API::MatrixWorkspace_sptr &,
                          const std::vector<int> &);
  API::IMDEventWorkspace_sptr
  mergeWorkspaces(std::vector<API::MatrixWorkspace_sptr> &);
  void setWorkspaceRotationAngle(API::MatrixWorkspace_sptr,
                                 double rotationAngle);

  std::string m_instrumentName; ///< Name of the instrument
  double m_wavelength = 0;
  std::vector<std::string> m_supportedInstruments{"D2B"};
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_LOADILLASCII_H_ */
