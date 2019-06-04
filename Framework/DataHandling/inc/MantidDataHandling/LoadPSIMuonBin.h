// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADPSIMUONBIN_H_
#define MANTID_DATAHANDLING_LOADPSIMUONBIN_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BinaryStreamReader.h"
#include <cstdint>

/** LoadPSIMuonBin : Loads a bin file from the PSI facility for muon
 spectroscopy
 */

namespace Mantid {
namespace DataHandling {

struct headerData {
  int16_t tdcResolution;
  int16_t tdcOverflow;
  int16_t numberOfRuns;
  int16_t lengthOfHistograms;
  int16_t numberOfHistograms;
  int32_t totalEvents;
  float histogramBinWidth;
  int32_t monNumberOfevents;
  int16_t numberOfDataRecordsFile;
  int16_t lengthOfDataRecordsBin;
  int16_t numberOfDataRecordsHistogram;
  int16_t numberOfHistogramsPerRecord;
  int32_t periodOfSave;
  int32_t periodOfMon;
  std::string sample;
  std::string temp;
  std::string field;
  std::string orientation;
  std::string comment;
  std::string dateStart;
  std::string dateEnd;
  std::string timeStart;
  std::string timeEnd;
  std::string monDeviation;
  int32_t scalars[18];
  std::string labels_scalars[18]; // 18 Strings with 4 max length
  int32_t labelsOfHistograms[16];
  int16_t integerT0[16];
  int16_t firstGood[16];
  int16_t lastGood[16];
  float realT0[16];
  float temperatures[4];
  float temperatureDeviation[4];
  float monLow[4];
  float monHigh[4];
};

struct temperatureHeaderData {
  std::vector<std::string> titles;
  std::string startDateTime;
  bool delimeterOfTitlesIsBackSlash;
};

class DLLExport LoadPSIMuonBin
    : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::string category() const override;
  int confidence(Kernel::FileDescriptor &descriptor) const override;
  bool loadMutipleAsOne() override;

private:
  void init() override;
  void exec() override;
  std::string getFormattedDateTime(std::string date, std::string time);
  void assignOutputWorkspaceParticulars(
      DataObjects::Workspace2D_sptr &outputWorkspace);
  void readSingleVariables(Mantid::Kernel::BinaryStreamReader &streamReader);
  void readStringVariables(Mantid::Kernel::BinaryStreamReader &streamReader);
  void readArrayVariables(Mantid::Kernel::BinaryStreamReader &streamReader);
  void readInHeader(Mantid::Kernel::BinaryStreamReader &streamReader);
  void readInHistograms(Mantid::Kernel::BinaryStreamReader &streamReader);
  void generateUnknownAxis();

  // Temperature file processing
  void readInTemperatureFile(DataObjects::Workspace2D_sptr &ws);
  std::string detectTempFile();
  void processLine(const std::string &line, DataObjects::Workspace2D_sptr &ws);
  void readInTemperatureFileHeader(const std::string &contents);
  void processHeaderLine(const std::string &line);
  void processDateHeaderLine(const std::string &line);
  void processTitleHeaderLine(const std::string &line);

  // Sample log helper functions
  Mantid::API::Algorithm_sptr
  createSampleLogAlgorithm(DataObjects::Workspace2D_sptr &ws);
  void addToSampleLog(const std::string &logName, const std::string &logText,
                      DataObjects::Workspace2D_sptr &ws);
  void addToSampleLog(const std::string &logName, const double &logNumber,
                      DataObjects::Workspace2D_sptr &ws);
  void addToSampleLog(const std::string &logName, const int &logNumber,
                      DataObjects::Workspace2D_sptr &ws);

  std::vector<std::vector<double>> m_histograms;
  struct headerData m_header;
  struct temperatureHeaderData m_tempHeader;
  std::vector<double> m_xAxis;
  std::vector<std::vector<double>> m_eAxis;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADPSIMUONBIN_H_*/