// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
*/
class MANTID_DATAHANDLING_DLL LoadILLDiffraction
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }
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
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;

  void calculateRelativeRotations(std::vector<double> &instrumentAngles,
                                  const Kernel::V3D &firstTubePosition);

  void fillDataScanMetaData(const NeXus::NXDouble &);
  void fillMovingInstrumentScan(const NeXus::NXUInt &, const NeXus::NXDouble &);
  void fillStaticInstrumentScan(const NeXus::NXUInt &, const NeXus::NXDouble &,
                                const NeXus::NXFloat &);

  std::vector<Types::Core::DateAndTime>
  getAbsoluteTimes(const NeXus::NXDouble &) const;
  std::vector<double> getAxis(const NeXus::NXDouble &) const;
  std::vector<double> getDurations(const NeXus::NXDouble &) const;
  std::vector<double> getMonitor(const NeXus::NXDouble &) const;
  std::string getInstrumentFilePath(const std::string &) const;
  Kernel::V3D getReferenceComponentPosition(
      const API::MatrixWorkspace_sptr &instrumentWorkspace);
  bool containsCalibratedData(const std::string &filename) const;

  std::vector<double>
  getScannedVaribleByPropertyName(const NeXus::NXDouble &scan,
                                  const std::string &propertyName) const;

  void initStaticWorkspace();
  void initMovingWorkspace(const NeXus::NXDouble &scan,
                           const std::string &start_time);

  void loadDataScan();
  API::MatrixWorkspace_sptr loadEmptyInstrument(const std::string &start_time);
  void loadMetaData();
  void loadScanVars();
  void loadStaticInstrument();
  void moveTwoThetaZero(double);
  void resolveInstrument();
  void resolveScanType();
  void setSampleLogs();
  void computeThetaOffset();

  double
      m_offsetTheta; ///< the 2theta offset for D20 to account for dead pixels
  size_t m_sizeDim1; ///< size of dim1, number of tubes (D2B) or the whole
                     /// detector (D20)
  size_t m_sizeDim2; ///< size of dim2, number of pixels (1 for D20!)
  size_t m_numberDetectorsRead;   ///< number of cells read from file
  size_t m_numberDetectorsActual; ///< number of cells actually active
  size_t m_numberScanPoints;      ///< number of scan points
  size_t m_resolutionMode;        ///< resolution mode; 1:low, 2:nominal, 3:high

  std::string m_instName;               ///< instrument name to load the IDF
  std::set<std::string> m_instNames;    ///< supported instruments
  std::string m_filename;               ///< file name to load
  Types::Core::DateAndTime m_startTime; ///< start time of acquisition
  ScanType m_scanType;                  ///< NoScan, DetectorScan or OtherScan
  double m_pixelHeight{0.};             ///< height of the pixel in D2B
  double m_maxHeight{0.}; ///< maximum absolute height of the D2B tubes

  std::vector<ScannedVariables> m_scanVar;  ///< holds the scan info
  LoadHelper m_loadHelper;                  ///< a helper for metadata
  API::MatrixWorkspace_sptr m_outWorkspace; ///< output workspace
  bool m_useCalibratedData{false}; ///< whether to use the calibrated data in
                                   ///< the nexus (D2B only)
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLDIFFRACTION_H_ */
