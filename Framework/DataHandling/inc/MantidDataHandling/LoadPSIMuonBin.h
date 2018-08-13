#ifndef MANTID_DATAHANDLING_LOADPSIMUONBIN_H_
#define MANTID_DATAHANDLING_LOADPSIMUONBIN_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BinaryStreamReader.h"
#include <cstdint>

/** LoadPSIMuonBin : Loads a bin file from the PSI facility for muon
 spectroscopy

 Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
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