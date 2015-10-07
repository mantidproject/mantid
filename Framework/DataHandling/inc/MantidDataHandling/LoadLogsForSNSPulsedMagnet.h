#ifndef MANTID_DATAHANDLING_LOADLOGSFORSNSPULSEDMAGNET_H_
#define MANTID_DATAHANDLING_LOADLOGSFORSNSPULSEDMAGNET_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <vector>

namespace Mantid {
namespace DataHandling {

/** LoadLogsForSNSPulsedMagnet : TODO: DESCRIPTION
 *
 * @author
 * @date 2011-06-07
 */
class DLLExport LoadLogsForSNSPulsedMagnet : public API::Algorithm {
public:
  LoadLogsForSNSPulsedMagnet();
  ~LoadLogsForSNSPulsedMagnet();

  /// Algorithm's name for identification
  virtual const std::string name() const {
    return "LoadLogsForSNSPulsedMagnet";
  };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Both log files are in binary format";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling\\Logs"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  /// Parse DelayTime log file
  void ParseDelayTimeLogFile();

  /// Parse PulseID log file
  void ParsePulseIDLogFile();

  /// Integrate to TimeSeriesProperty
  void addProperty();

  std::string m_delaytimefilename;
  std::string m_pulseidfilename;

  bool m_delayfileinoldformat;

  std::size_t m_numpulses;
  int m_numchoppers;
  unsigned int **m_delaytimes;
  std::vector<uint32_t> m_pulseidseconds;
  std::vector<uint32_t> m_pulseidnanoseconds;

  API::MatrixWorkspace_sptr WS;
};

} // namespace Mantid
} // namespace DataHandling

#endif /* MANTID_DATAHANDLING_LOADLOGSFORSNSPULSEDMAGNET_H_ */
