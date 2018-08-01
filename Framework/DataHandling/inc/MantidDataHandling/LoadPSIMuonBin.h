#ifndef MANTID_DATAHANDLING_LOADPSIMUONBIN_H_
#define MANTID_DATAHANDLING_LOADPSIMUONBIN_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BinaryStreamReader.h"
#include <cstdint>

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

  std::vector<std::vector<double>> m_histograms;
  struct headerData m_header;
  std::vector<double> m_xAxis;
  std::vector<std::vector<double>> m_eAxis;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADPSIMUONBIN_H_*/