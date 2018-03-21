#ifndef MANTID_DATAHANDLING_NXCANSASDEFINITIONS_H_
#define MANTID_DATAHANDLING_NXCANSASDEFINITIONS_H_

#include <string>

namespace Mantid {
namespace DataHandling {

namespace NXcanSAS {
// General
enum class WorkspaceDimensionality { oneD, twoD, other };

// NXcanSAS Tag Definitions
const std::string sasUnitAttr = "units";
const std::string sasUncertaintyAttr = "uncertainty";
const std::string sasSignal = "signal";
const std::string sasSeparator = ",";
const std::string sasAngstrom = "A";
const std::string sasNone = "none";
const std::string sasIntensity = "1/cm";
const std::string sasMomentumTransfer = "1/A";
const std::string sasNxclass = "NX_class";
const std::string sasCanSASclass = "canSAS_class";

// SASentry
const std::string sasEntryClassAttr = "SASentry";
const std::string nxEntryClassAttr = "NXentry";
const std::string sasEntryGroupName = "sasentry";
const std::string sasEntryVersionAttr = "version";
const std::string sasEntryVersionAttrValue = "1.0";
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
const std::string sasDataQIndicesAttr = "Q_indices";
const std::string sasDataQUncertaintyAttr = "Q_uncertainty";
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
const std::string sasInstrumentSourceRadiation = "radiation";

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

// SAStransmission_spectrum
const std::string sasTransmissionSpectrumClassAttr = "SAStransmission_spectrum";
const std::string nxTransmissionSpectrumClassAttr = "NXdata";
const std::string sasTransmissionSpectrumGroupName = "sastransmission_spectrum";
const std::string sasTransmissionSpectrumTIndices = "T_indices";
const std::string sasTransmissionSpectrumTUncertainty = "T_uncertainty";
const std::string sasTransmissionSpectrumNameAttr = "name";
const std::string sasTransmissionSpectrumNameSampleAttrValue = "sample";
const std::string sasTransmissionSpectrumNameCanAttrValue = "can";
const std::string sasTransmissionSpectrumTimeStampAttr = "timestamp";
const std::string sasTransmissionSpectrumLambda = "lambda";
const std::string sasTransmissionSpectrumT = "T";
const std::string sasTransmissionSpectrumTdev = "Tdev";
}
}
}
#endif
