#ifndef MANTID_DATAHANDLING_NXCANSASDEFINITIONS_H_
#define MANTID_DATAHANDLING_NXCANSASDEFINITIONS_H_

#include <string>

namespace Mantid {
namespace DataHandling {

namespace NXcanSAS {
  // NXcanSAS Tag Definitions
  const std::string sasUnitAttr = "unit";

  // SASentry
  const std::string sasEntryClassAttr = "SASentry";
  const std::string sasEntryGroupName = "sasentry";
  const std::string sasEntryVersionAttr = "version";
  const std::string sasEntryVersionAttrValue = "1.0";
  const std::string sasEntryDefinition = "definition";
  const std::string sasEntryDefinitionFormat = "NXcanSAS";
  const std::string sasEntryTitle = "title";
  const std::string sasEntryRun = "run";

  // SASdata
  const std::string sasDataClassAttr = "SASdata";
  const std::string sasDataClassGroupName = "SASdata";
  const std::string sasDataIAxesAttr = "I_axes";
  const std::string sasDataQIndicesAttr = "Q_indices";
  const std::string sasDataMaskIndicesAttr = "Mask_indices";
  const std::string sasDataUncertaintyAttr = "uncertainty";
  const std::string sasDataQ = "Q";
  const std::string sasDataQdev = "Qdev";
  const std::string sasDataI = "I";
  const std::string sasDataIdev = "Idev";
  const std::string sasDataMask = "Mask";


  // SASinstrument
  const std::string sasInstrumentClassAttr = "SASinstrument";
  const std::string sasInstrumentGroupName = "SASinstrument";
  const std::string sasInstrumentName = "name";

  const std::string sasInstrumentSourceClassAttr = "SASSource";
  const std::string sasInstrumentSourceGroupName = "sassource";
  const std::string sasInstrumentSourceRadiation = "radiation";

  const std::string sasInstrumentCollimationClassAttr = "SAScollimation";
  const std::string sasInstrumentCollimationGroupName = "sascollimation";

  const std::string sasInstrumentDetectorClassAttr = "SASdetector";
  const std::string sasInstrumentDetectorGroupName = "sasdetector";
  const std::string sasInstrumentDetectorName = "name";
  const std::string sasInstrumentDetectorSdd = "SDD";
  const std::string sasInstrumentDetectorSddUnitAttrValue = "m";

  const std::string sasInstrumentSampleClassAttr = "SASsample";
  const std::string sasInstrumentSampleGroupAttr = "sassample";
  const std::string sasInstrumentSampleId = "ID";

  // SASprocess
  const std::string sasProcessClassAttr = "SASprocess";
  const std::string sasProcessGroupName = "sasprocess";
  const std::string sasProcessName = "name";
  const std::string sasProcessDate = "date";
  const std::string sasProcessTerm = "term";
  const std::string sasProcessTermSvn = "svn";
  const std::string sasProcessTermUserFile = "UserFile";

  //SAStransmission_spectrum
  const std::string sasTransmissionSpectrumClassAttr = "SAStransmission_spectrum";
  const std::string sasTransmissionSpectrumGroupName = "sastransmission_spectrum";
  const std::string sasTransmissionSpectrumNameAttr = "name";
  const std::string sasTransmissionSpectrumNameSampleAttrValue = "sample";
  const std::string sasTransmissionSpectrumNameCanAttrValue = "can";
  const std::string sasTransmissionSpectrumTimeStampAttr = "timestamp";
  const std::string sasTransmissionSpectrumLambda = "lambda";
  const std::string sasTransmissionSpectrumT = "T";
  const std::string sasTransmissionSpectrumTSignalAttr = "1";
  const std::string sasTransmissionSpectrumTAxesAttr = "T";
  const std::string sasTransmissionSpectrumTUncertaintyAttr = "uncertainty";
  const std::string sasTransmissionSpectrumTdev = "Tdev";

}
}
}
#endif
