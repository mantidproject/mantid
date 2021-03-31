// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

namespace Mantid {
namespace DataHandling {

class DLLExport LoadDetectorInfo : public API::Algorithm {
public:
  LoadDetectorInfo();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadDetectorInfo"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads delay times, tube pressures, tube wall thicknesses and, if "
           "necessary, the detectors positions from a given special format "
           "file";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadRaw", "LoadNexus"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Raw"; }

private:
  /// Simple data holder for passing the detector info around when
  /// dealing with the NeXus data
  struct DetectorInfo {
    std::vector<detid_t> ids;
    std::vector<int32_t> codes;
    std::vector<double> delays;
    std::vector<double> l2, theta, phi;
    std::vector<double> pressures, thicknesses;
  };

  void init() override;
  void exec() override;

  /// Cache the user input that will be frequently accessed
  void cacheInputs();
  /// Use a .dat or .sca file as input
  void loadFromDAT(const std::string &filename);
  /// Use a .raw file as input
  void loadFromRAW(const std::string &filename);
  /// Use a isis raw nexus or event file as input
  void loadFromIsisNXS(const std::string &filename);
  /// Read data from old-style libisis NeXus file
  void readLibisisNxs(::NeXus::File &nxsFile, DetectorInfo &detInfo) const;
  /// Read data from old-style libisis NeXus file
  void readNXSDotDat(::NeXus::File &nxsFile, DetectorInfo &detInfo) const;

  /// Update the parameter map with the new values for the given detector
  void updateParameterMap(Geometry::DetectorInfo &detectorInfo, const size_t detIndex, Geometry::ParameterMap &pmap,
                          const double l2, const double theta, const double phi, const double delay,
                          const double pressure, const double thickness) const;

  /// Cached instrument for this workspace
  Geometry::Instrument_const_sptr m_baseInstrument;
  /// Cached sample position for this workspace
  Kernel::V3D m_samplePos;
  /// If set to true then update the detector positions base on the information
  /// in the given file
  bool m_moveDets;
  /// store a pointer to the user selected workspace
  API::MatrixWorkspace_sptr m_workspace;

  /// Delta value that has been set on the instrument
  double m_instDelta;
  /// Pressure value set on the instrument level
  double m_instPressure;
  /// Wall thickness value set on the instrument level
  double m_instThickness;
};

} // namespace DataHandling
} // namespace Mantid
