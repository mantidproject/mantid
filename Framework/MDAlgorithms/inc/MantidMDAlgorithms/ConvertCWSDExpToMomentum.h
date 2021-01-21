// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertCWSDExpToMomentum : TODO: DESCRIPTION
 */
class DLLExport ConvertCWSDExpToMomentum : public API::Algorithm {
public:
  ConvertCWSDExpToMomentum();

  /// Algorithm's name
  const std::string name() const override { return "ConvertCWSDExpToMomentum"; }

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Load and convert a set of files in an HB3A experiment."; }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\ConstantWavelength;DataHandling\\Text"; }

private:
  void init() override;
  void exec() override;

  void addMDEvents(bool usevirtual);

  void convertSpiceMatrixToMomentumMDEvents(const API::MatrixWorkspace_sptr &dataws, bool usevirtual,
                                            const detid_t &startdetid, const int scannumber, const int runnumber,
                                            double measuretime, int monitor_counts);

  /// Convert |Q| with detector position to Q_sample
  Kernel::V3D convertToQSample(const Kernel::V3D &samplePos, const Kernel::V3D &ki, const Kernel::V3D &detPos,
                               const double &momentum, std::vector<Mantid::coord_t> &qSample,
                               const Kernel::DblMatrix &rotationMatrix);

  API::IMDEventWorkspace_sptr createExperimentMDWorkspace();

  bool getInputs(bool virtualinstrument, std::string &errmsg);

  API::MatrixWorkspace_sptr loadSpiceData(const std::string &filename, bool &loaded, std::string &errmsg);

  void parseDetectorTable(std::vector<Kernel::V3D> &vec_detpos, std::vector<detid_t> &vec_detid);

  void setupTransferMatrix(const API::MatrixWorkspace_sptr &dataws, Kernel::DblMatrix &rotationMatrix);

  void createVirtualInstrument();

  void updateQRange(const std::vector<Mantid::coord_t> &vec_q);

  /// Remove background from
  void removeBackground(const API::MatrixWorkspace_sptr &dataws);

  API::ITableWorkspace_sptr m_expDataTableWS;
  API::ITableWorkspace_sptr m_detectorListTableWS;
  API::IMDEventWorkspace_sptr m_outputWS;
  Geometry::Instrument_sptr m_virtualInstrument;

  /// Shifts in detector position set from user (calibration): all in the unit
  /// as meter
  double m_detSampleDistanceShift;
  double m_detXShift;
  double m_detYShift;

  Kernel::V3D m_samplePos;
  Kernel::V3D m_sourcePos;

  size_t m_iColScan;
  size_t m_iColPt;
  size_t m_iColFilename;
  size_t m_iColStartDetID;
  size_t m_iMonitorCounts;
  size_t m_iTime;

  std::vector<double> m_extentMins;
  std::vector<double> m_extentMaxs;
  std::vector<size_t> m_numBins;

  std::vector<coord_t> m_minQVec;
  std::vector<coord_t> m_maxQVec;
  bool m_setQRange;

  /// Data directory
  std::string m_dataDir;
  /// Flag to use m_dataDir
  bool m_isBaseName;
  /// Background workspace
  bool m_removeBackground;
  API::MatrixWorkspace_const_sptr m_backgroundWS;
};

} // namespace MDAlgorithms
} // namespace Mantid
