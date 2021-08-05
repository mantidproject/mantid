// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidKernel/System.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/** LoadILLSANS2; supports D11, D22 and D33 (TOF/monochromatic)
 */

class DLLExport LoadILLSANS2 : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLSANS2();
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }
  const std::string category() const override;
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  enum MeasurementType { MONO, TOF, KINETIC };

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
  void setInstrumentName(const NeXus::NXEntry &, const std::string &);
  DetectorPosition getDetectorPositionD33(const NeXus::NXEntry &, const std::string &);

  void initWorkspace(NeXus::NXEntry &, const std::string &);
  size_t initWorkspaceD11B(NeXus::NXEntry &);
  size_t initWorkspaceD22B(NeXus::NXEntry &);
  size_t initWorkspaceD33(NeXus::NXEntry &, const std::string &);
  size_t initWorkspaceD16(NeXus::NXEntry &, const std::string &);
  void createEmptyWorkspace(const size_t, const size_t);

  size_t loadDataFromMonitors(NeXus::NXEntry &firstEntry, size_t firstIndex = 0);
  size_t loadDataFromTubes(NeXus::NXInt &, const std::vector<double> &, size_t);
  void runLoadInstrument();
  void placeInstrument(const NeXus::NXEntry &, const std::string &);
  void moveDetectorDistance(const double, const std::string &, const double angle = 0.0);
  void moveDetectorHorizontal(double, const std::string &);
  void moveDetectorVertical(double, const std::string &);
  Kernel::V3D getComponentPosition(const std::string &);
  void loadMetaData(const NeXus::NXEntry &, const std::string &);
  std::string getInstrumentFilePath(const std::string &) const;
  void rotateInstrument(double, const std::string &);
  void adjustTOF();
  void moveSource();
  std::tuple<int, int, int> getDataDimensions(NeXus::NXEntry &);
  void figureOutMeasurementType(NeXus::NXEntry &);

  LoadHelper m_loadHelper;                         ///< Load helper for metadata
  std::string m_instrumentName;                    ///< Name of the instrument
  std::vector<std::string> m_supportedInstruments; ///< List of supported instruments
  API::MatrixWorkspace_sptr m_localWorkspace;      ///< to-be output workspace
  std::vector<double> m_defaultBinning;            ///< the default x-axis binning
  std::string m_resMode;                           ///< Resolution mode for D11 and D22
  double m_sourcePos;                              ///< Source Z (for D33 TOF)
  bool m_isD16Omega;                               ///< Data come from a D16 omega scan flag
  bool m_loadInstrument;                           ///< Flag for loading the instrument geometry
  MeasurementType m_measurementType;               ///< Holds the measurement type of the data

  void setFinalProperties(const std::string &filename);
  std::vector<double> getVariableTimeBinning(const NeXus::NXEntry &, const std::string &, const NeXus::NXInt &,
                                             const NeXus::NXFloat &) const;
};

} // namespace DataHandling
} // namespace Mantid
