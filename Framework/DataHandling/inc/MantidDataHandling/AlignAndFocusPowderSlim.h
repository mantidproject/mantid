// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/SpectraProcessingData.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/TimeSplitter.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/TimeROI.h"

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

/** AlignAndFocusPowderSlim : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL AlignAndFocusPowderSlim : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;

  API::MatrixWorkspace_sptr createOutputWorkspace(size_t num_hist);
  SpectraProcessingData initializeSpectraProcessingData(const API::MatrixWorkspace_sptr &outputWS);
  void storeSpectraProcessingData(const SpectraProcessingData &processingData,
                                  const API::MatrixWorkspace_sptr &outputWS);
  API::MatrixWorkspace_sptr editInstrumentGeometry(API::MatrixWorkspace_sptr &wksp, const double l1,
                                                   const std::vector<double> &polars,
                                                   const std::vector<specnum_t> &specids,
                                                   const std::vector<double> &l2s,
                                                   const std::vector<double> &azimuthals);
  API::MatrixWorkspace_sptr convertToTOF(API::MatrixWorkspace_sptr &wksp);
  void initCalibrationConstants(API::MatrixWorkspace_sptr &wksp, const std::vector<double> &difc_focus);
  void loadCalFile(const API::Workspace_sptr &inputWS, const std::string &filename,
                   const std::vector<double> &difc_focus);
  void initScaleAtSample(const API::MatrixWorkspace_sptr &wksp);
  std::vector<std::pair<size_t, size_t>> determinePulseIndices(const API::MatrixWorkspace_sptr &wksp,
                                                               const Kernel::TimeROI &filterROI);
  static std::vector<std::pair<int, std::pair<size_t, size_t>>>
  determinePulseIndicesTargets(const API::MatrixWorkspace_sptr &wksp, const Kernel::TimeROI &filterROI,
                               const DataObjects::TimeSplitter &timeSplitter);
  Kernel::TimeROI getFilterROI(const API::MatrixWorkspace_sptr &wksp);
  DataObjects::TimeSplitter timeSplitterFromSplitterWorkspace(const Types::Core::DateAndTime &);

  std::map<detid_t, double> m_calibration; ///< detid: 1/difc
  /**
   * Multiplicative 0<value<1 to move neutron TOF at sample.
   * It will only be filled if time at sample is requested.
   */
  std::map<detid_t, double> m_scale_at_sample;
  std::set<detid_t> m_masked;
  bool is_time_filtered{false};
  /// Index to load start at in the file
  std::vector<int64_t> loadStart;
  /// How much to load in the file
  std::vector<int64_t> loadSize;
  // map of detectorID to output spectrum number
  std::map<detid_t, size_t> detIDToSpecNum;
};

// these properties are public to simplify testing and calling from other code
namespace PropertyNames {
const std::string FILENAME("Filename");
const std::string CAL_FILE("CalFileName");
const std::string FILTER_TIMESTART("FilterByTimeStart");
const std::string FILTER_TIMESTOP("FilterByTimeStop");
const std::string GROUPING_WS("GroupingWorkspace");
const std::string SPLITTER_WS("SplitterWorkspace");
const std::string SPLITTER_RELATIVE("RelativeTime");
const std::string CORRECTION_TO_SAMPLE("CorrectionToSample");
const std::string PROCESS_BANK_SPLIT_TASK("ProcessBankSplitTask");
const std::string FULL_TIME("UseFullTime");
const std::string FILTER_BAD_PULSES("FilterBadPulses");
const std::string FILTER_BAD_PULSES_LOWER_CUTOFF("BadPulsesLowerCutoff");
const std::string X_MIN("XMin");
const std::string X_MAX("XMax");
const std::string X_DELTA("XDelta");
const std::string BIN_UNITS("BinningUnits");
const std::string BINMODE("BinningMode");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string READ_SIZE_FROM_DISK("ReadSizeFromDisk");
const std::string EVENTS_PER_THREAD("EventsPerThread");
const std::string ALLOW_LOGS("LogAllowList");
const std::string BLOCK_LOGS("LogBlockList");
const std::string OUTPUT_SPEC_NUM("OutputSpectrumNumber");
// focus positions
const std::string L1("L1");
const std::string L2("L2");
const std::string POLARS("Polar");
const std::string AZIMUTHALS("Azimuthal");
} // namespace PropertyNames

} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
