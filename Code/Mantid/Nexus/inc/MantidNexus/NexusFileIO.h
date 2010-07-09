#ifndef NEXUSFILEIO_H
#define NEXUSFILEIO_H
#include <napi.h>
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "boost/date_time/local_time_adjustor.hpp"
#include "boost/date_time/c_local_time_adjustor.hpp"
#include <limits.h>
namespace Mantid
{
  namespace NeXus
  {
    /** @class NexusFileIO NexusFileIO.h NeXus/NexusFileIO.h

    Utility method for saving NeXus format of Mantid Workspace
    This class interfaces to the C Nexus API. This is written for use by
    Save and Load NexusProcessed classes, though it could be extended to
    other Nexus formats. It might be replaced in future by methods using
    the new Nexus C++ API.

    Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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
    class DLLExport NexusFileIO
    {
    public:
      /// Default constructor
      NexusFileIO();

      /// Destructor
      ~NexusFileIO() {}

      /// open the nexus file for writing
      int openNexusWrite(const std::string& fileName);
      /// open the nexus file for reading
      int openNexusRead(const std::string& fileName, int& workspaceNumber);
      /// write the header ifon for the Mantid workspace format
      int writeNexusProcessedHeader( const std::string& title);
      /// write sample related data
      /*int writeNexusProcessedSample( const std::string& title,
							  const boost::shared_ptr<Mantid::API::Sample>& sample);*/

	  int writeNexusProcessedSample( const std::string& title,const Mantid::API::Sample& sample);
      /// read sample data
	  int readNexusProcessedSample(Mantid::API::Sample& sample);
      /// write the workspace data
      int writeNexusProcessedData( const API::MatrixWorkspace_const_sptr& localworkspace,
							const bool& uniformSpectra, const std::vector<int>& spec);
      /// find size of open entry data section
      int getWorkspaceSize( int& numberOfSpectra, int& numberOfChannels, int& numberOfXpoints ,
               bool& uniformBounds, std::string& axesNames, std::string& yUnits );
      /// read X values for one (or the generic if uniform) spectra
      int getXValues(MantidVec& xValues, const int& spectra);
      /// read values and errors for spectra
      int getSpectra(MantidVec& values, MantidVec& errors, const int& spectra);

      /// write the algorithm and environment information
      int writeNexusProcessedProcess(const API::MatrixWorkspace_const_sptr& localworkspace);
      /// write the source XML file used, if it exists
      bool writeNexusInstrumentXmlName(const std::string& instrumentXml,const std::string& date,
                            const std::string& version);
      /// read the source XML file used
      bool readNexusInstrumentXmlName(std::string& instrumentXml,std::string& date,
                            std::string& version);
      /// read the source XML file used
      std::string readNexusInstrumentName();
      /// write an instrument section - currently only the name
      bool writeNexusInstrument(const API::IInstrument_const_sptr& instrument);
      /// write any spectra map information to Nexus file
      bool writeNexusProcessedSpectraMap(const API::MatrixWorkspace_const_sptr& localWorkspace, const std::vector<int>& spec);
      /// read spectra map information
      bool readNexusProcessedSpectraMap(API::MatrixWorkspace_sptr localWorkspace);
      /// Read the vertical axis (axis(1)) values
      bool readNexusProcessedAxis(API::MatrixWorkspace_sptr localWorkspace);
      /// write instrument parameters
      bool writeNexusParameterMap(API::MatrixWorkspace_const_sptr ws);
      /// read instrument parameters
      bool readNexusParameterMap(API::MatrixWorkspace_sptr ws);
      /// close the nexus file
      int closeNexusFile();

    private:
      /// Nexus file handle
      NXhandle fileID;
      /// Nexus format to use for writing
      NXaccess m_nexusformat;
      /// Nexus compression method
      int m_nexuscompression;
      /// write a text value plus possible attribute
      bool writeNxText( const std::string& name, const std::string& value, const std::vector<std::string>& attributes,
				 const std::vector<std::string>& avalues);
      /// read a text value plus possible attribute
      bool readNxText(const std::string& name, std::string& value, std::vector<std::string>& attributes,
				 std::vector<std::string>& avalues);
      /// write an NXnote with standard fields (but NX_CHAR rather than NX_BINARY data)
      bool writeNxNote(const std::string& noteName, const std::string& author, const std::string& date,
                         const std::string& description, const std::string& pairValues);
      /// write a flost value plus possible attributes
      bool writeNxFloat(const std::string& name, const double& value, const std::vector<std::string>& attributes,
	                           const std::vector<std::string>& avalues);
      /// read a float value and possible attributes
      bool readNxFloat(const std::string& name, double& value, std::vector<std::string>& attributes,
	                           std::vector<std::string>& avalues);
      /// write a float array along with any defined attributes
      bool writeNxFloatArray(const std::string& name, const std::vector<double>& values, const std::vector<std::string>& attributes,
	                           const std::vector<std::string>& avalues);
      /// write a char array along with any defined attributes
      bool writeNxStringArray(const std::string& name, const std::vector<std::string>& values, const std::vector<std::string>& attributes,
	                           const std::vector<std::string>& avalues);
      /// Write NXlog data for given double TimeSeriesProperty
      void writeNexusDoubleLog(const Kernel::TimeSeriesProperty<double> *d_timeSeries);
      /// Write NXlog data for given string TimeSeriesProperty
      void writeNexusStringLog(const Kernel::TimeSeriesProperty<std::string> *s_timeSeries);
      /// read the named NXlog and create TimeSeriesProp in sample
      void readNXlog(const char* nxname, Mantid::API::Sample& sample);
      /// check if the gievn item exists in the current level
      bool checkEntryAtLevel(const std::string& item) const;
      /// check if given attribute name is in currently opened entry
      bool checkAttributeName(const std::string& target) const;
      /// Look for entry with given attribute (eg "signal")
      bool checkEntryAtLevelByAttribute(const std::string& attribute, std::string& entry) const;
      /// write test field
      int writeNexusTextField( const NXhandle& h, const std::string& name, const std::string& value);
      /// search for exisiting MantidWorkpace_n entries in opened file
      int findMantidWSEntries();
      /// convert posix time to time_t
      std::time_t to_time_t(boost::posix_time::ptime t) ///< convert posix time to time_t
      {
            /*!
            Take the input Posix time, subtract the unix epoch, and return the seconds
            as a std::time_t value.
            @param t :: time of interest as ptime
            @return :: time_t value of t
            */
            if( t == boost::posix_time::neg_infin )
               return 0;
            else if( t == boost::posix_time::pos_infin )
               return LONG_MAX;
            boost::posix_time::ptime start(boost::gregorian::date(1970,1,1));
            return (t-start).total_seconds();
       } 

      /// nexus file name
      std::string m_filename;
      ///static reference to the logger class
      static Kernel::Logger& g_log;
      // copied from TimesSeriesProperty.h
      /// Create time_t instance from a ISO 8601 yyyy-mm-ddThh:mm:ss input string
      std::time_t createTime_t_FromString(const std::string &str)
      {
          std::tm time_since_1900;
          time_since_1900.tm_isdst = -1;
 
         // create tm struct
         time_since_1900.tm_year = atoi(str.substr(0,4).c_str()) - 1900;
         time_since_1900.tm_mon = atoi(str.substr(5,2).c_str()) - 1;
         time_since_1900.tm_mday = atoi(str.substr(8,2).c_str());
         time_since_1900.tm_hour = atoi(str.substr(11,2).c_str());
         time_since_1900.tm_min = atoi(str.substr(14,2).c_str());
         time_since_1900.tm_sec = atoi(str.substr(17,2).c_str());
    
         return std::mktime(&time_since_1900);
      }

      /** Writes a numeric log to the Nexus file
       *  @tparam T A numeric type (double, int, bool)
       *  @param timeSeries A pointer to the log property
       */
      template<class T>
      void writeNexusNumericLog(const Kernel::TimeSeriesProperty<T> *timeSeries)
      {
          // write NXlog section for double values
          NXstatus status;
          // get a name for the log, possibly removing the the path component
          std::string logName=timeSeries->name();
          size_t ipos=logName.find_last_of("/\\");
          if(ipos!=std::string::npos)
              logName=logName.substr(ipos+1);
          // extract values from timeseries
          std::map<time_t, T> dV=timeSeries->valueAsMap();
          std::vector<double> values;
          std::vector<double> times;
          time_t t0; // ,time;
          bool first=true;
          for(typename std::map<time_t, T>::const_iterator dv=dV.begin();dv!=dV.end();dv++)
          {
              T val = dv->second;
              time_t time = dv->first;
              values.push_back(val);
              if(first)
              {
                  t0=time; // start time of log
                  first=false;
              }
              times.push_back(static_cast<double>(time-t0));
          }
          // create log
          status=NXmakegroup(fileID,logName.c_str(),"NXlog");
          if(status==NX_ERROR)
              return;
          status=NXopengroup(fileID,logName.c_str(),"NXlog");
          // write log data
          std::vector<std::string> attributes,avalues;
          attributes.push_back("type");
          avalues.push_back(logValueType<T>());
          writeNxFloatArray("value", values,  attributes, avalues);
          attributes.clear();
          avalues.clear();
          // get ISO time, if t0 valid
          char buffer [25];
          if(t0>0)
          {
              strftime (buffer,25,"%Y-%m-%dT%H:%M:%S",localtime(&t0));
              attributes.push_back("start");
              avalues.push_back(buffer);
          }
          else
          {
              g_log.warning("Bad start time in log file " + logName);
          }

          writeNxFloatArray("time", times,  attributes, avalues);
          //
          status=NXclosegroup(fileID);
          //
      }

      /// Return the log value type as string
      template<class T>
      std::string logValueType()const{return "unknown";}

    };

  } // namespace NeXus
} // namespace Mantid

#endif /* NEXUSFILEIO_H */
