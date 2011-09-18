#ifndef MANTID_DATAHANDLING_LOADLOGSFORSNSPULSEDMAGNET_H_
#define MANTID_DATAHANDLING_LOADLOGSFORSNSPULSEDMAGNET_H_
/*WIKI* 

Load SNS's Delay Time log file and Pulse ID file for pulsed magnet.  The log information is added to an existing Workspace as Property 
*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <vector>

namespace Mantid
{
namespace DataHandling
{

  /** LoadLogsForSNSPulsedMagnet : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-06-07
   */
  class DLLExport LoadLogsForSNSPulsedMagnet  : public API::Algorithm
  {
  public:
    LoadLogsForSNSPulsedMagnet();
    ~LoadLogsForSNSPulsedMagnet();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadLogsForSNSPulsedMagnet";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "General";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
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
    std::size_t m_numchoppers;
    unsigned int** m_delaytimes;
    std::vector<uint32_t> m_pulseidseconds;
    std::vector<uint32_t> m_pulseidnanoseconds;

    API::MatrixWorkspace_sptr WS;

  };


} // namespace Mantid
} // namespace DataHandling

#endif  /* MANTID_DATAHANDLING_LOADLOGSFORSNSPULSEDMAGNET_H_ */
