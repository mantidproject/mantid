#include "MantidDataHandling/LoadLogsForSNSPulsedMagnet.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sstream>

using std::size_t;
using std::vector;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadLogsForSNSPulsedMagnet)

using namespace Mantid::Kernel;
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadLogsForSNSPulsedMagnet::LoadLogsForSNSPulsedMagnet() {
  // TODO Auto-generated constructor stub
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadLogsForSNSPulsedMagnet::~LoadLogsForSNSPulsedMagnet() {
  // TODO Auto-generated destructor stub
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadLogsForSNSPulsedMagnet::init() {

  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace in which to attach the pulsed "
                  "magnet log information.");

  declareProperty(
      new FileProperty("DelayTimeFilename", "", FileProperty::Load, ".dat"),
      "The name (including its full or relative path) of the log file to\n"
      "attempt to load the pulsed magnet log. The file extension must either "
      "be\n"
      ".dat or .DAT");

  declareProperty(
      new FileProperty("PulseIDFilename", "", FileProperty::Load, ".dat"),
      "The name (including its full or relative path) of the log file to\n"
      "attempt to load the PulseID. The file extension must either be\n"
      ".dat or .DAT");

  declareProperty(
      new PropertyWithValue<bool>("OldFormat", false, Direction::Input),
      "Delay time file have an old format");

  declareProperty(
      new PropertyWithValue<int64_t>("NumberOfChoppers", 4, Direction::Input),
      "Number of choppers used in data acquisition.  It is not required for "
      "new format Delay time file.");

  m_numpulses = 0;
  m_numchoppers = 4;
  m_delayfileinoldformat = false;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadLogsForSNSPulsedMagnet::exec() {

  // 1. Retrieve the information from input data file (binary)
  m_delaytimefilename = getPropertyValue("DelayTimeFileName");
  m_pulseidfilename = getPropertyValue("PulseIDFileName");
  m_delayfileinoldformat = getProperty("OldFormat");
  if (m_delayfileinoldformat)
    m_numchoppers = getProperty("NumberOfChoppers");

  // 1 b) check input
  if (m_numchoppers < 1) {
    throw std::invalid_argument(
        "Number of choppers cannot be smaller than 1. ");
  }

  WS = getProperty("Workspace");

  g_log.information() << "Input Files: " << m_delaytimefilename << " , "
                      << m_pulseidfilename << std::endl;

  // 2. Parse delaytime file
  ParseDelayTimeLogFile();

  // 3. Parse pulse ID file
  ParsePulseIDLogFile();

  // 4. Integrate answer
  addProperty();

  return;
}

void LoadLogsForSNSPulsedMagnet::ParseDelayTimeLogFile() {
  char *logfilename = &m_delaytimefilename[0];

  // 1. Determine length of file
  struct stat results;
  size_t filesize;
  if (stat(logfilename, &results) == 0) {
    filesize = static_cast<size_t>(results.st_size);
    g_log.debug() << "File Size = " << filesize << "\n";
  } else {
    g_log.error() << "File Error!  Cannot Read File Size"
                  << "\n";
    return;
  }

  // 2. Determine number of magnetic pulses
  size_t numpulses;
  if (m_delayfileinoldformat) {
    numpulses = filesize / sizeof(double) / m_numchoppers;
  } else {
    numpulses = filesize / (4 + 4) / m_numchoppers;
  }
  g_log.debug() << "Number of Pulses = " << numpulses
                << " Old format = " << m_delayfileinoldformat << std::endl;

  // 3. Build data structure
  unsigned int **delaytimes = new unsigned int *[numpulses];
  for (unsigned int i = 0; i < numpulses; i++)
    delaytimes[i] = new unsigned int[m_numchoppers];

  // 4. Parse
  std::ifstream logFile(logfilename, std::ios::in | std::ios::binary);
  size_t index = 0;
  unsigned int localdelaytimes[4];
  for (size_t p = 0; p < numpulses; p++) {
    for (size_t i = 0; i < static_cast<size_t>(m_numchoppers); i++) {
      unsigned int chopperindex;
      unsigned int delaytime;
      double dtime;
      if (m_delayfileinoldformat) {
        // Old format
        logFile.read((char *)&dtime, sizeof(double));

        chopperindex = (unsigned int)(i + 1);
        delaytime = (unsigned int)(dtime * 1000);

      } else {
        // New format
        logFile.read((char *)&chopperindex, sizeof(unsigned int));
        logFile.read((char *)&delaytime, sizeof(unsigned int));
      }
      if (delaytime != 0) {
        g_log.debug() << "Pulse Index =  " << index
                      << "  Chopper = " << chopperindex
                      << "   Delay Time = " << delaytime << std::endl;
      }
      localdelaytimes[i] = delaytime;
    }

    // Load
    for (int i = 0; i < 4; i++) {
      delaytimes[p][i] = localdelaytimes[i];
    }

    index++;
  }
  logFile.close();

  m_numpulses = numpulses;
  m_delaytimes = delaytimes;

  return;
}

#pragma pack(push, 4) // Make sure the structure is 16 bytes.
struct Pulse {
  /// The number of nanoseconds since the seconds field. This is not necessarily
  /// less than one second.
  uint32_t nanoseconds;

  /// The number of seconds since January 1, 1990.
  uint32_t seconds;

  /// The index of the first event for this pulse.
  uint64_t event_index;

  /// The proton charge for the pulse.
  // cppcheck-suppress unusedStructMember
  double pCurrent;
};
#pragma pack(pop)

void LoadLogsForSNSPulsedMagnet::ParsePulseIDLogFile() {
  std::vector<Pulse> *pulses;
  BinaryFile<Pulse> pulseFile(m_pulseidfilename);
  this->m_numpulses = pulseFile.getNumElements();
  pulses = pulseFile.loadAll();
  for (std::vector<Pulse>::iterator it = pulses->begin(); it != pulses->end();
       ++it) {
    this->m_pulseidseconds.push_back(it->seconds);
    this->m_pulseidnanoseconds.push_back(it->nanoseconds);
  }
  delete pulses;
}

void LoadLogsForSNSPulsedMagnet::addProperty() {
  //    DateAndTime epoc(int64_t(m_pulseidhighs[0]),
  //    int64_t(m_pulseidhighs[0]));
  // create values to put into the property
  //    std::vector<double> times;
  //    std::vector<double> values;*/

  TimeSeriesProperty<double> **property = new TimeSeriesProperty<double> *[4];
  for (int i = 0; i < 4; i++) {
    std::stringstream namess;
    namess << "PulsedMagnetDelay" << i;
    std::string tempname = namess.str();
    property[i] = new TimeSeriesProperty<double>(tempname);
    property[i]->setUnits("nanoseconds");
  }

  for (size_t pulse_index = 0; pulse_index < m_numpulses; pulse_index++) {
    DateAndTime dt(static_cast<int32_t>(m_pulseidseconds[pulse_index]),
                   static_cast<int32_t>(m_pulseidnanoseconds[pulse_index]));
    for (int i = 0; i < 4; i++) {
      property[i]->addValue(dt,
                            static_cast<double>(m_delaytimes[pulse_index][i]));
    }
  }

  for (int i = 0; i < 4; i++)
    WS->mutableRun().addProperty(property[i], false);

  g_log.debug() << "Integration is Over!\n";

  // Clean memory
  delete[] property;

  return;
}

} // namespace Mantid
} // namespace DataHandling
