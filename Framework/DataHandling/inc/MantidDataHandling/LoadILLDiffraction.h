#ifndef MANTID_DATAHANDLING_LOADILLDIFFRACTION_H_
#define MANTID_DATAHANDLING_LOADILLDIFFRACTION_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidNexus/NexusClasses.h"

namespace {
enum ScanType : size_t {
    NoScan = 0,
    DetectorScan = 1,
    OtherScan = 2
};

struct ScannedVariables {
  int axis;
  int scanned;
  std::string name;
  std::string property;
  std::string unit;

  ScannedVariables(int a, int s, std::string n, std::string p, std::string u)
      : axis(a), scanned(s), name(n), property(p), unit(u) {}
};
}

namespace Mantid {
namespace DataHandling {

/** LoadILLDiffraction : Loads ILL diffraction nexus files.

  @author Gagik Vardanyan, vardanyan@ill.fr

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

  void loadDataScan(NeXus::NXEntry &);
  void loadScannedVariables(NeXus::NXEntry &);

  void loadMetadata();
  void loadStaticInstrument();

  void fillMovingInstrumentScan(const NeXus::NXUInt &,
                                const NeXus::NXDouble &) {}
  void fillStaticInstrumentScan(const NeXus::NXUInt &,
                                const NeXus::NXDouble &);

  void initWorkspace();
  void resolveInstrument(const std::string &);
  void resolveScanType();

  int m_numberDetectorsRead; ///< number of cells read from file
  int m_numberDetectorsActual; ///< number of cells actually active
  int m_numberScanPoints; ///< number of scan points

  std::string m_instName; ///< instrument name to load the IDF
  std::set<std::string> m_instNames; ///< supported instruments
  std::string m_fileName; ///< file name to load
  std::vector<ScannedVariables> m_scanVars; ///< holds the info on what is scanned
  ScanType m_scanType; ///< scan type

  LoadHelper m_loadHelper; ///< a helper for metadata
  API::MatrixWorkspace_sptr m_outWorkspace; ///< output workspace
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLDIFFRACTION_H_ */
