#include "MantidDataHandling/LoadLogsForSNSPulsedMagnet.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>

using namespace std;

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

    m_numpulses = -1;
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
    IntegrateProperty();


    return;
  }

  void LoadLogsForSNSPulsedMagnet::ParseDelayTimeLogFile()
  {

    char* logfilename = &m_delaytimefilename[0];

    // 1. Determine length of file
    struct stat results;
    int filesize = -1;
    if (stat(logfilename, &results) == 0){
        filesize = results.st_size;
        g_log.information() << "File Size = " << filesize << endl;
    }
    else{
        g_log.error() << "File Error!  Cannot Read File Size" << endl;
        return;
    }

    // 2. Determine number of magnetic pulses
    int numpulses = filesize/4/8;
    g_log.information() << "Number of Pulses = " << numpulses << endl;

    // 3. Parse
    ifstream logFile(logfilename, ios::in|ios::binary);
    unsigned int *pulseindices = new unsigned int[numpulses];
    unsigned int *delaytimes = new unsigned int[numpulses];
    char buffer[4];
    int index = 0;
    unsigned int chopperindices[4];
    unsigned int localdelaytimes[4];
    for (int p = 0; p < numpulses; p ++){
        // logFile.read(buffer, 4);
        for (int i = 0; i < 4; i ++){
            unsigned int chopperindex;
            unsigned int delaytime;
            logFile.read((char*)&chopperindex, sizeof(unsigned int));
            logFile.read((char*)&delaytime, sizeof(unsigned int));
            if (delaytime != 0){
                g_log.information() << "Pulse Index =  " << index << "  Chopper = " << chopperindex << "   Delay Time = " << delaytime << endl;
            }
            chopperindices[i] = chopperindex;
            localdelaytimes[i] = delaytime;
        }

        // Check
        for (int i = 0; i < 4; i ++){
            if (i != chopperindices[i]){
                g_log.information() << "Warning Here 111  Pulsed = " << index << " Chopper Index = " << chopperindices[i] << "  vs " << i << endl;
            }
        }

        index ++;
    }
    logFile.close();

    m_numpulses = numpulses;
    m_delaytimes = delaytimes;

    return;
  }

  void LoadLogsForSNSPulsedMagnet::ParsePulseIDLogFile()
  {

    char* logfilename = &m_pulseidfilename[0];

    // 1. Determine length of file
    struct stat results;
    int filesize = -1;
    if (stat(logfilename, &results) == 0){
        filesize = results.st_size;
        g_log.information() << "File Size = " << filesize << endl;
    }
    else{
        g_log.error() << "File Error!  Cannot Read File Size" << endl;
        return;
    }

    // 2. Determine number of magnetic pulses
    int structsize = 4+4+8+8;
    int numpulses = filesize/structsize;
    if (filesize%structsize != 0){
      g_log.error() << "Pulse ID File Length Incorrect!" << endl;
    }
    g_log.information() << "Number of Pulses (In Pulse ID File) = " << numpulses << endl;

    unsigned int* pulseIDhighs = new unsigned int[numpulses];
    unsigned int* pulseIDlows = new unsigned int[numpulses];

    ifstream logFile(logfilename, ios::in|ios::binary);
    for (int pid = 0; pid < numpulses; pid++){
      unsigned int pidlow, pidhigh;
      unsigned long eventid;
      double charge;
      logFile.read((char*)&pidlow, sizeof(unsigned int));
      logFile.read((char*)&pidhigh, sizeof(unsigned int));
      logFile.read((char*)&eventid, sizeof(unsigned long));
      logFile.read((char*)&charge, sizeof(double));

      pulseIDhighs[pid] = pidhigh;
      pulseIDlows[pid] = pidlow;
    }

    // 4. Set to class variable
    m_pulseidlows = pulseIDlows;
    m_pulseidhighs = pulseIDhighs;

    return;
  }

  void LoadLogsForSNSPulsedMagnet::IntegrateProperty(){

    TimeSeriesProperty<unsigned int>* m_properties = new TimeSeriesProperty<unsigned int>("PulsedMagnet");

    for (int pid = 0; pid < m_numpulses; pid++){
      long pidhigh = long(m_pulseidhighs[pid]);
      long pidlow = long(m_pulseidlows[pid]);
      DateAndTime dt(pidhigh, pidlow);
      m_properties->addValue(dt, m_delaytimes[pid]);
    }

    WS->mutableRun().addProperty(m_properties, false);
    // addProperty(Kernel::Property *prop, bool overwrite = false);

    g_log.information() << "Integration is Over!" << endl;

    return;
  }



} // namespace Mantid
} // namespace DataHandling

