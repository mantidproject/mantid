#ifndef MANTID_DATAHANDLING_LOADPSIMUONBIN_H_
#define MANTID_DATAHANDLING_LOADPSIMUONBIN_H_

#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include <cstdint>

namespace Mantid {
namespace DataHandling {

class DLLExport LoadPSIMuonBin
    : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Default constructor
  LoadPSIMuonBin();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override;

  /// Algorithm's version for identification overriding a virtual method
  int version() const override;

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Mantid::DescriptorType &descriptor) const override;

  /// Returns a value indicating whether or not loader wants to load multiple
  /// files into a single workspace
  bool loadMutipleAsOne() override;

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  // Define the variables of the header
  int16_t tdcResolution;
  int16_t tdcOverflow;
  int16_t numberOfRuns;
  int16_t lengthOfHistograms;
  int16_t numberOfHistograms;
  char sample[11]; // char arrays are 1 larger to allow for '/0' if needed
  char temp[11];
  char field[11];
  char orientation[11];
  char comment[63];
  char dateStart[10];
  char dateEnd[10];
  char timeStart[9];
  char timeEnd[9];
  int32_t totalEvents;
  int32_t labelsOfHistograms[16];
  int16_t integerT0[16];
  int16_t firstGood[16];
  int16_t lastGood[16];
  float real_t0[16];
  int32_t scalars[18];
  char labels_scalars[18][5]; // 18 lots of 4 char arrays with '/0' at the end
  float histogramBinWidth;
  float temperatures[4];
  float temperatureDeviation[4];
  float monLow[4]; //No idea what this is
  float monHigh[4];
  int32_t monNumberOfevents;
  char monDeviation[12];
  int16_t numberOfDataRecordsFile; //numdef
  int16_t lengthOfDataRecordsBin; //lendef
  int16_t numberOfDataRecordsHistogram; //kdafhi
  int16_t numberOfHistogramsPerRecord; //khidaf
  int32_t periodOfSave;
  int32_t periodOfMon;
}
} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADPSIMUONBIN_H_*/