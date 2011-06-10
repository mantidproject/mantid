#include "MantidDataHandling/LoadLogsForSNSPulsedMagnet.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>

using std::size_t;
using std::vector;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadLogsForSNSPulsedMagnet)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadLogsForSNSPulsedMagnet::LoadLogsForSNSPulsedMagnet()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadLogsForSNSPulsedMagnet::~LoadLogsForSNSPulsedMagnet()
  {
    // TODO Auto-generated destructor stub
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadLogsForSNSPulsedMagnet::initDocs()
  {
    this->setWikiSummary("Load SNS's DelayTime log file and Pulse ID file of Pulsed Magnet ");
    this->setOptionalMessage("Both log files are in binary format");
    this->setWikiDescription("Load SNS's Delay Time log file and Pulse ID file for pulsed magnet.  The log information is added to an existing Workspace as Property ");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadLogsForSNSPulsedMagnet::init()
  {

    declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
        "The name of the workspace in which to attach the pulsed magnet log information." );

    declareProperty(new FileProperty("DelayTimeFilename", "", FileProperty::Load, ".dat"),
                      "The name (including its full or relative path) of the log file to\n"
                      "attempt to load the pulsed magnet log. The file extension must either be\n"
                      ".dat or .DAT" );

    declareProperty(new FileProperty("PulseIDFilename", "", FileProperty::Load, ".dat"),
                      "The name (including its full or relative path) of the log file to\n"
                      "attempt to load the PulseID. The file extension must either be\n"
                      ".dat or .DAT" );

    m_numpulses = 0;
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadLogsForSNSPulsedMagnet::exec()
  {
    // TODO Auto-generated execute stub

    // 1. Retrieve the information from input data file (binary)
    m_delaytimefilename = getPropertyValue("DelayTimeFileName");
    m_pulseidfilename = getPropertyValue("PulseIDFileName");

    WS = getProperty("Workspace");

    g_log.information() << "Input Files: " << m_delaytimefilename << " , " << m_pulseidfilename << std::endl;

    // 2. Parse delaytime file
    ParseDelayTimeLogFile();

    // 3. Parse pulse ID file
    ParsePulseIDLogFile();

    // 4. Integrate answer
    addProperty();


    return;
  }

  void LoadLogsForSNSPulsedMagnet::ParseDelayTimeLogFile()
  {

    char* logfilename = &m_delaytimefilename[0];

    // 1. Determine length of file
    struct stat results;
    size_t filesize = -1;
    if (stat(logfilename, &results) == 0){
        filesize = static_cast<size_t>(results.st_size);
        g_log.information() << "File Size = " << filesize << "\n";
    }
    else{
        g_log.error() << "File Error!  Cannot Read File Size" << "\n";
        return;
    }

    // 2. Determine number of magnetic pulses
    size_t numpulses = filesize/4/8;
    g_log.information() << "Number of Pulses = " << numpulses << "\n";

    // 3. Parse
    std::ifstream logFile(logfilename, std::ios::in|std::ios::binary);
    // unsigned int *pulseindices = new unsigned int[numpulses];
    unsigned int *delaytimes = new unsigned int[numpulses];
    // char buffer[4];
    size_t index = 0;
    unsigned int chopperindices[4];
    unsigned int localdelaytimes[4];
    for (size_t p = 0; p < numpulses; p ++){
        // logFile.read(buffer, 4);
        for (int i = 0; i < 4; i ++){
            unsigned int chopperindex;
            unsigned int delaytime;
            logFile.read((char*)&chopperindex, sizeof(unsigned int));
            logFile.read((char*)&delaytime, sizeof(unsigned int));
            if (delaytime != 0){
                g_log.information() << "Pulse Index =  " << index << "  Chopper = " << chopperindex << "   Delay Time = " << delaytime << "\n";
                std::cout << "Pulse Index =  " << index << "  Chopper = " << chopperindex << "   Delay Time = " << delaytime << "\n";
            }
            chopperindices[i] = chopperindex;
            localdelaytimes[i] = delaytime;
        }

        // Check
        for (unsigned int i = 0; i < 4; i ++){
            if (i != chopperindices[i]){
                g_log.information() << "Warning Here 111  Pulsed = " << index << " Chopper Index = " << chopperindices[i] << "  vs " << i << "\n";
            }
        }

        index ++;
    }
    logFile.close();

    m_numpulses = numpulses;
    m_delaytimes = delaytimes;

    return;
  }

#pragma pack(push, 4) //Make sure the structure is 16 bytes.
struct Pulse
{
  /// The number of nanoseconds since the seconds field. This is not necessarily less than one second.
  uint32_t nanoseconds;

  /// The number of seconds since January 1, 1990.
  uint32_t seconds;

  /// The index of the first event for this pulse.
  uint64_t event_index;

  /// The proton charge for the pulse.
  double pCurrent;
};
#pragma pack(pop)

  void LoadLogsForSNSPulsedMagnet::ParsePulseIDLogFile()
  {
    std::vector<Pulse> * pulses;
    BinaryFile<Pulse> pulseFile(m_pulseidfilename);
    this->m_numpulses = pulseFile.getNumElements();
    pulses = pulseFile.loadAll();
    for (std::vector<Pulse>::iterator it = pulses->begin(); it != pulses->end(); it++)
    {
      this->m_pulseidseconds.push_back(it->seconds);
      this->m_pulseidnanoseconds.push_back(it->nanoseconds);
    }
    delete pulses;
  }

  void LoadLogsForSNSPulsedMagnet::addProperty(){
//    DateAndTime epoc(int64_t(m_pulseidhighs[0]), int64_t(m_pulseidhighs[0]));
    // create values to put into the property
//    std::vector<double> times;
//    std::vector<double> values;*/

    TimeSeriesProperty<double>* property = new TimeSeriesProperty<double>("PulsedMagnetDelay");
//    property->create(start_time, times, values);
    property->setUnits("nanoseconds");

    for (size_t pulse_index = 0; pulse_index < m_numpulses; pulse_index++){
      DateAndTime dt(static_cast<int32_t>(m_pulseidseconds[pulse_index]),
                     static_cast<int32_t>(m_pulseidnanoseconds[pulse_index]));
      property->addValue(dt, static_cast<double>(m_delaytimes[pulse_index]));
//      if (m_delaytimes[pulse_index] > 0) // REMOVE
//        std::cout << pulse_index << ": " << dt << " " << static_cast<double>(m_delaytimes[pulse_index]) << std::endl; // REMOVE
    }

    WS->mutableRun().addProperty(property, false);
    // addProperty(Kernel::Property *prop, bool overwrite = false);

    g_log.information() << "Integration is Over!\n";

    return;
  }



} // namespace Mantid
} // namespace DataHandling

