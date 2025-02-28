// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses.h"

#include <H5Cpp.h>

namespace Mantid {
namespace DataHandling {

/** LoadILLSANS; supports D11, D22 and D33 (TOF/monochromatic)
 */

class MANTID_DATAHANDLING_DLL LoadILLSANS : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLSANS();
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }
  const std::string category() const override;
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  enum MultichannelType { TOF, KINETIC, SCAN };
  struct DetectorPosition {
    double distanceSampleRear;
    double distanceSampleBottomTop;
    double distanceSampleRightLeft;
    double shiftLeft;
    double shiftRight;
    double shiftUp;
    double shiftDown;
    void operator>>(std::ostream &strm) {
      strm << "DetectorPosition : "
           << "distanceSampleRear = " << distanceSampleRear << ", "
           << "distanceSampleBottomTop = " << distanceSampleBottomTop << ", "
           << "distanceSampleRightLeft = " << distanceSampleRightLeft << ", "
           << "shiftLeft = " << shiftLeft << ", "
           << "shiftRight = " << shiftRight << ", "
           << "shiftUp = " << shiftUp << ", "
           << "shiftDown = " << shiftDown << '\n';
    }
  };

  void init() override;
  void exec() override;
  void applySensitivityMap();
  void setInstrumentName(const NeXus::NXEntry &, const std::string &);
  DetectorPosition getDetectorPositionD33(const NeXus::NXEntry &, const std::string &);

  void initWorkSpace(NeXus::NXEntry &, const std::string &);
  void initWorkSpaceD11B(NeXus::NXEntry &, const std::string &);
  void initWorkSpaceD22B(NeXus::NXEntry &, const std::string &);
  void initWorkSpaceD33(NeXus::NXEntry &, const std::string &);
  void initWorkSpaceD16(NeXus::NXEntry &, const std::string &);
  void createEmptyWorkspace(const size_t, const size_t, const MultichannelType type = MultichannelType::TOF);
  void getDataDimensions(const NeXus::NXInt &data, size_t &numberOfChannels, size_t &numberOfTubes,
                         size_t &numberOfPixelsPerTube);
  size_t loadDataFromMonitors(NeXus::NXEntry &firstEntry, size_t firstIndex = 0,
                              const MultichannelType type = MultichannelType::TOF);
  size_t loadDataFromD16ScanMonitors(const NeXus::NXEntry &firstEntry, size_t firstIndex,
                                     const std::vector<double> &binning);
  size_t loadDataFromTubes(NeXus::NXInt const &, const std::vector<double> &, size_t,
                           const MultichannelType type = MultichannelType::TOF);
  void runLoadInstrument();
  void moveDetectorsD33(const DetectorPosition &);
  void moveDetectorDistance(double distance, const std::string &componentName);
  void moveDetectorHorizontal(double, const std::string &);
  void moveDetectorVertical(double, const std::string &);
  void loadMetaData(const NeXus::NXEntry &, const std::string &);
  std::string getInstrumentFilePath(const std::string &) const;
  void rotateInstrument(double, const std::string &);
  void placeD16(double, double, const std::string &);
  void adjustTOF();
  void moveSource();
  void getMonitorIndices(const std::string &);

  std::string m_instrumentName;                    ///< Name of the instrument
  std::vector<std::string> m_supportedInstruments; ///< List of supported instruments
  API::MatrixWorkspace_sptr m_localWorkspace;      ///< to-be output workspace
  std::vector<double> m_defaultBinning;            ///< the default x-axis binning
  std::string m_resMode;                           ///< Resolution mode for D11 and D22
  bool m_isTOF;                                    ///< TOF or monochromatic flag
  double m_sourcePos;                              ///< Source Z (for D33 TOF)
  bool m_isD16Omega;                               ///< Data comes from a D16 omega scan flag
  size_t m_numberOfMonitors;                       ///< Number of monitors in this instrument
  std::vector<size_t> m_monitorIndices;            ///< Indices for monitor data in scanned variables table

  void setFinalProperties(const std::string &filename);
  std::vector<double> getVariableTimeBinning(const NeXus::NXEntry &, const std::string &, const NeXus::NXInt &,
                                             const NeXus::NXFloat &) const;
  std::vector<double> getOmegaBinning(const NeXus::NXEntry &entry, const std::string &path) const;
};

} // namespace DataHandling
} // namespace Mantid
