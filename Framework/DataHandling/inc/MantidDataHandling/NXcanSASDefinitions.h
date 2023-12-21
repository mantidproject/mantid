// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

namespace Mantid {
namespace DataHandling {

namespace NXcanSAS {
// General
enum class WorkspaceDimensionality { oneD, twoD, other };

// NXcanSAS Tag Definitions
const std::string sasUnitAttr = "units";
const std::string sasSignal = "signal";
const std::string sasSeparator = ",";
const std::string sasAngstrom = "A";
const std::string sasNone = "none";
const std::string sasIntensity = "1/cm";
const std::string sasMomentumTransfer = "1/A";
const std::string sasNxclass = "NX_class";
const std::string sasCanSASclass = "canSAS_class";

/**
 * Standards state that "uncertainties" should be used, however different
 * facilities interpret the standards differently.
 * At time of writing, 01/19, both "uncertainty" and "uncertainties" are needed
 * so that Mantid NXcanSAS output is compatible with all other NXcanSAS files.
 **/
const std::string sasUncertaintyAttr = "uncertainty";
const std::string sasUncertaintiesAttr = "uncertainties";

// SASentry
const std::string sasEntryClassAttr = "SASentry";
const std::string nxEntryClassAttr = "NXentry";
const std::string sasEntryGroupName = "sasentry";
const std::string sasEntryVersionAttr = "version";
const std::string sasEntryVersionAttrValue = "1.1";
const std::string sasEntryDefinition = "definition";
const std::string sasEntryDefinitionFormat = "NXcanSAS";
const std::string sasEntryTitle = "title";
const std::string sasEntryRun = "run";
const std::string sasEntryRunInLogs = "run_number";

// SASdata
const std::string sasDataClassAttr = "SASdata";
const std::string nxDataClassAttr = "NXdata";
const std::string sasDataGroupName = "sasdata";
const std::string sasDataIAxesAttr = "I_axes";
const std::string sasDataIUncertaintyAttr = "I_uncertainty";
const std::string sasDataIUncertaintiesAttr = "I_uncertainties";
const std::string sasDataQIndicesAttr = "Q_indices";
const std::string sasDataQUncertaintyAttr = "Q_uncertainty";
const std::string sasDataQUncertaintiesAttr = "Q_uncertainties";
const std::string sasDataMaskIndicesAttr = "Mask_indices";

const std::string sasDataQ = "Q";
const std::string sasDataQx = "Qx";
const std::string sasDataQy = "Qy";
const std::string sasDataQdev = "Qdev";

const std::string sasDataI = "I";
const std::string sasDataIdev = "Idev";
const std::string sasDataMask = "Mask";

// SASinstrument
const std::string sasInstrumentClassAttr = "SASinstrument";
const std::string nxInstrumentClassAttr = "NXinstrument";
const std::string sasInstrumentGroupName = "sasinstrument";
const std::string sasInstrumentName = "name";

const std::string sasInstrumentSourceClassAttr = "SASsource";
const std::string nxInstrumentSourceClassAttr = "NXsource";

const std::string sasInstrumentSourceGroupName = "sassource";
const std::string sasInstrumentSourceRadiation = "type";

const std::string sasInstrumentApertureClassAttr = "SASaperture";
const std::string nxInstrumentApertureClassAttr = "NXaperture";

const std::string sasInstrumentApertureGroupName = "sasaperture";
const std::string sasInstrumentApertureShape = "shape";
const std::string sasInstrumentApertureGapWidth = "x_gap";
const std::string sasInstrumentApertureGapHeight = "y_gap";

const std::string sasInstrumentCollimationClassAttr = "SAScollimation";
const std::string nxInstrumentCollimationClassAttr = "NXcollimator";

const std::string sasInstrumentCollimationGroupName = "sascollimation";

const std::string sasInstrumentDetectorClassAttr = "SASdetector";
const std::string nxInstrumentDetectorClassAttr = "NXdetector";

const std::string sasInstrumentDetectorGroupName = "sasdetector";
const std::string sasInstrumentDetectorName = "name";
const std::string sasInstrumentDetectorSdd = "SDD";
const std::string sasInstrumentDetectorSddUnitAttrValue = "m";

const std::string sasInstrumentSampleClassAttr = "SASsample";
const std::string nxInstrumentSampleClassAttr = "NXsample";

const std::string sasInstrumentSampleGroupAttr = "sassample";
const std::string sasInstrumentSampleId = "ID";
const std::string sasInstrumentSampleThickness = "thickness";

const std::string sasBeamAndSampleSizeUnitAttrValue = "mm";

const std::string sasInstrumentIDF = "idf";

// SASprocess
const std::string sasProcessClassAttr = "SASprocess";
const std::string nxProcessClassAttr = "NXprocess";
const std::string sasProcessGroupName = "sasprocess";
const std::string sasProcessName = "name";
const std::string sasProcessNameValue = "Mantid_generated_NXcanSAS";
const std::string sasProcessDate = "date";
const std::string sasProcessTermSvn = "svn";
const std::string sasProcessTermCan = "can_trans_run";
const std::string sasProcessTermUserFile = "user_file";
const std::string sasProcessUserFileInLogs = "UserFile";
const std::string sasProcessTermBatchFile = "batch_file";
const std::string sasProcessBatchFileInLogs = "BatchFile";

// SASnote
const std::string sasNoteClassAttr = "SASnote";
const std::string nxNoteClassAttr = "NXnote";
const std::string sasNoteGroupName = "sasnote";
const std::string sasProcessTermSampleTrans = "sample_trans_run";
const std::string sasProcessTermSampleDirect = "sample_direct_run";
const std::string sasProcessTermCanScatter = "can_scatter_run";
const std::string sasProcessTermCanDirect = "can_direct_run";
const std::string sasProcessTermScaledBgSubWorkspace = "scaled_bgsub_workspace";
const std::string sasProcessTermScaledBgSubScaleFactor = "scaled_bgsub_scale_factor";

// SAStransmission_spectrum
const std::string sasTransmissionSpectrumClassAttr = "SAStransmission_spectrum";
const std::string nxTransmissionSpectrumClassAttr = "NXdata";
const std::string sasTransmissionSpectrumGroupName = "sastransmission_spectrum";
const std::string sasTransmissionSpectrumTIndices = "T_indices";
const std::string sasTransmissionSpectrumTUncertainty = "T_uncertainty";
const std::string sasTransmissionSpectrumTUncertainties = "T_uncertainties";
const std::string sasTransmissionSpectrumNameAttr = "name";
const std::string sasTransmissionSpectrumNameSampleAttrValue = "sample";
const std::string sasTransmissionSpectrumNameCanAttrValue = "can";
const std::string sasTransmissionSpectrumTimeStampAttr = "timestamp";
const std::string sasTransmissionSpectrumLambda = "lambda";
const std::string sasTransmissionSpectrumT = "T";
const std::string sasTransmissionSpectrumTdev = "Tdev";
} // namespace NXcanSAS
} // namespace DataHandling
} // namespace Mantid
