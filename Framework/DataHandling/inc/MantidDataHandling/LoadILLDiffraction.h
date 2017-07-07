#ifndef MANTID_DATAHANDLING_LOADILLDIFFRACTION_H_
#define MANTID_DATAHANDLING_LOADILLDIFFRACTION_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/V3D.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/** LoadILLDiffraction : Loads ILL diffraction nexus files.

  @date 15/05/17

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
  enum ScanType : size_t { NoScan = 0, DetectorScan = 1, OtherScan = 2 };

  struct ScannedVariables {
    int axis;
    int scanned;
    std::string name;
    std::string property;
    std::string unit;

    ScannedVariables(std::string n, std::string p, std::string u)
        : axis(0), scanned(0), name(n), property(p), unit(u) {}

    void setAxis(int a) { axis = a; }
    void setScanned(int s) { scanned = s; }
  };

  void init() override;
  void exec() override;

  std::vector<Kernel::DateAndTime>
  getAbsoluteTimes(const NeXus::NXDouble &) const;
  std::vector<double> getAxis(const NeXus::NXDouble &) const;
  std::vector<double> getDurations(const NeXus::NXDouble &) const;
  std::vector<double> getMonitor(const NeXus::NXDouble &) const;
  std::string getInstrumentFilePath(const std::string &) const;

  void fillDataScanMetaData(const NeXus::NXDouble &);
  std::vector<double>
  getScannedVaribleByPropertyName(const NeXus::NXDouble &scan,
                                  const std::string &propertyName) const;
  void fillMovingInstrumentScan(const NeXus::NXUInt &, const NeXus::NXDouble &);
  void fillStaticInstrumentScan(const NeXus::NXUInt &, const NeXus::NXDouble &,
                                const NeXus::NXFloat &);

  void initStaticWorkspace();
  void initMovingWorkspace(const NeXus::NXDouble &scan);
  Kernel::V3D getReferenceComponentPosition(
      const API::MatrixWorkspace_sptr &instrumentWorkspace);
  void calculateRelativeRotations(std::vector<double> &instrumentAngles,
                                  const Kernel::V3D &tube1Position);
  void loadDataScan();
  void loadMetaData();
  void loadScanVars();
  void loadStaticInstrument();
  API::MatrixWorkspace_sptr loadEmptyInstrument();
  void moveTwoThetaZero(double);
  void resolveInstrument();
  void resolveScanType();

  size_t m_sizeDim1;            ///< size of dim1, either tubes or detectors
  size_t m_sizeDim2;            ///< size of dim2, used for pixels within tubes
  size_t m_numberDetectorsRead; ///< number of cells read from file
  size_t m_numberDetectorsActual; ///< number of cells actually active
  size_t m_numberScanPoints;      ///< number of scan points
  size_t m_resolutionMode;        ///< resolution mode; 1:low, 2:nominal, 3:high

  std::string m_instName;            ///< instrument name to load the IDF
  std::set<std::string> m_instNames; ///< supported instruments
  std::string m_fileName;            ///< file name to load
  Kernel::DateAndTime m_startTime;   ///< start time of acquisition
  ScanType m_scanType;               ///< NoScan, DetectorScan or OtherScan

  std::vector<ScannedVariables> m_scanVar;  ///< holds the scan info
  LoadHelper m_loadHelper;                  ///< a helper for metadata
  API::MatrixWorkspace_sptr m_outWorkspace; ///< output workspace
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLDIFFRACTION_H_ */
