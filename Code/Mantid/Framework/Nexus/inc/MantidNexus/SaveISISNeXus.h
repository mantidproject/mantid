#ifndef MANTID_NEXUS_SAVEISISNEXUS_H_
#define MANTID_NEXUS_SAVEISISNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "LoadRaw/isisraw2.h"
#include <napi.h>

#include <climits>

namespace Mantid
{
  namespace NeXus
  {
    /**
    Reads a raw file and saves it in a ISIS NeXus file.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input RAW file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data </LI>
    </UL>

    @author Roman Tolchenov, Tessella plc
    @date 03/03/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport SaveISISNexus : public API::Algorithm
    {
    public:
      /// Default constructor
      SaveISISNexus();

      /// Destructor
      ~SaveISISNexus() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveISISNexus";};
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Nexus";}

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();

      /// Overwrites Algorithm method.
      void init();

      /// Overwrites Algorithm method
      void exec();

      ISISRAW2* m_isisRaw;
      NXhandle handle;
      FILE* rawFile;
      std::vector<int> monitorData;
      /// <spectrum_index,monitor_index>. spectrum index is an index in any detector related array, not spectrum number
      std::map<int,int> monitor_index;
      int nper; ///< number of periods
      int nsp;  ///< number of spectra
      int ntc;  ///< number of time channels
      int nmon; ///< number of monitors
      int ndet; ///< number of detectors
      std::string start_time_str;
      std::vector<std::string> log_notes;

      NXlink counts_link;
      NXlink period_index_link;
      NXlink spectrum_index_link;
      NXlink time_of_flight_link;
      NXlink time_of_flight_raw_link;
      int *getMonitorData(int period,int imon);

      void saveInt(const char* name,void* data, int size = 1);
      void saveChar(const char* name,void* data, int size);
      void saveFloat(const char* name,void* data, int size);
      void saveIntOpen(const char* name,void* data, int size = 1);
      void saveCharOpen(const char* name,void* data, int size);
      void saveFloatOpen(const char* name,void* data, int size);
      int saveStringVectorOpen(const char* name,const std::vector<std::string>& str_vec,int max_str_size = -1);
      void saveString(const char* name,const std::string& str);
      void saveStringOpen(const char* name,const std::string& str);
      inline void close(){NXclosedata(handle);} ///< close an open dataset.
      inline void closegroup(){NXclosegroup(handle);} ///< close an open group.
      void putAttr(const char* name,const std::string& value);
      void putAttr(const char* name,char* value,int size);
      void putAttr(const char* name,int value,int size=1);
      void toISO8601(std::string& str);
      
      template<typename T>
      friend class getWithoutMonitors;


      /// Write vms_compat
      void write_isis_vms_compat();
      /// Write monitors
      void write_monitors();
      /// Write single monitor
      void monitor_i(int i);
      /// Write instrument
      void instrument();
      /// Write instrument/detector_1
      void detector_1();
      /// Write instrument/moderator
      void moderator();
      /// Write instrument/dae
      void dae();
      /// Write instrument/source
      void source();
      /// Create a link to some of detector_1's data
      void make_detector_1_link();
      /// Write user
      void user();
      /// Write sample
      void sample();
      /// Write runlog
      void runlog();
      /// write one run log
      void write_runlog(const char* name, void* times, void* data,int type,int size,const std::string& units);
      /// write NXlog
      void write_logOpen(const char* name, void* times, void* data,int type,int size,const std::string& units);
      /// Write selog
      void selog();
      /// Write notes from LOG_STRUCT
      void logNotes();
      /// Write run cycle
      void run_cycle();
      void write_rpb();
      void write_spb();
      void write_vpb();

      /// The name and path of the input file
      std::string inputFilename;
		
    };

  } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_NEXUS_SAVEISISNEXUS_H_*/
