#ifndef MANTID_DATAHANDLING_LOADILLDIFFRACTION_H_
#define MANTID_DATAHANDLING_LOADILLDIFFRACTION_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/** LoadILLDiffraction : Loads ILL diffraction nexus files.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_DATAHANDLING_DLL LoadILLDiffraction
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  int confidence(Kernel::NexusDescriptor &descriptor) const override;
  LoadILLDiffraction();

private:
  void init() override;
  void exec() override;

  void initWorkspace();
  void loadDataScan(NeXus::NXEntry &);
  void loadMovingInstrument();
  void loadMetadata();
  void loadStaticInstrument();
  void fillMovingInstrumentScan(const NeXus::NXUInt &,
                                const NeXus::NXDouble &);
  void fillStaticInstrumentScan(const NeXus::NXUInt &,
                                const NeXus::NXDouble &);
  void resolveInstrument(const std::string &);

  int m_numberScanPoints;
  int m_numberDetectorsRead;
  int m_numberDetectorsActual;
  bool m_isDetectorScan;

  std::vector<int> m_scannedVarIndices;
  std::string m_instName;
  std::set<std::string> m_instNames;
  std::string m_fileName;

  LoadHelper m_loadHelper;
  API::MatrixWorkspace_sptr m_outWorkspace;
  std::unique_ptr<API::Progress> m_progress;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLDIFFRACTION_H_ */
