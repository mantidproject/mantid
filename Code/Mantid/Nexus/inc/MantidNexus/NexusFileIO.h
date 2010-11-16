#ifndef NEXUSFILEIO_H
#define NEXUSFILEIO_H
#include <napi.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "boost/date_time/local_time_adjustor.hpp"
#include "boost/date_time/c_local_time_adjustor.hpp"
#include <limits.h>

namespace Mantid
{
  namespace NeXus
  {
    int getNexusEntryTypes(const std::string& fileName, std::vector<std::string>& entryName,
                           std::vector<std::string>& definition );

    /** @class NexusFileIO NexusFileIO.h NeXus/NexusFileIO.h

    Utility method for saving NeXus format of Mantid Workspace
    This class interfaces to the C Nexus API. This is written for use by
    Save and Load NexusProcessed classes, though it could be extended to
    other Nexus formats. It might be replaced in future by methods using
    the new Nexus C++ API.

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
      /// write the header ifon for the Mantid workspace format
      int writeNexusProcessedHeader( const std::string& title) const;
      /// close the nexus file
      int closeNexusFile();
      /// Write a Nexus sample section
      int writeNexusProcessedSample( const std::string& title, const API::Sample & sample,
				     const Mantid::API::Run& runProperties) const;
      /// write the workspace data
      int writeNexusProcessedData( const API::MatrixWorkspace_const_sptr& localworkspace,
				   const bool& uniformSpectra, const std::vector<int>& spec) const;
      /// find size of open entry data section
      int getWorkspaceSize( int& numberOfSpectra, int& numberOfChannels, int& numberOfXpoints ,
			    bool& uniformBounds, std::string& axesNames, std::string& yUnits ) const;
      /// read X values for one (or the generic if uniform) spectra
      int getXValues(MantidVec& xValues, const int& spectra) const;
      /// read values and errors for spectra
      int getSpectra(MantidVec& values, MantidVec& errors, const int& spectra) const;

      /// write the algorithm and environment information
      int writeNexusProcessedProcess(const API::MatrixWorkspace_const_sptr& localworkspace) const;
      /// write the source XML file used, if it exists
      bool writeNexusInstrumentXmlName(const std::string& instrumentXml,const std::string& date,
                            const std::string& version) const;
      /// write an instrument section - currently only the name
      bool writeNexusInstrument(const Geometry::IInstrument_const_sptr& instrument) const;
      /// write any spectra map information to Nexus file
      bool writeNexusProcessedSpectraMap(const API::MatrixWorkspace_const_sptr& localWorkspace, const std::vector<int>& spec) const;
      /// write instrument parameters
      bool writeNexusParameterMap(API::MatrixWorkspace_const_sptr ws) const;
      /// write bin masking information
      bool writeNexusBinMasking(API::MatrixWorkspace_const_sptr ws) const;

    private:
      /// Nexus file handle
      NXhandle fileID;
      /// Nexus format to use for writing
      NXaccess m_nexusformat;
      /// Nexus compression method
      int m_nexuscompression;
      /// Write a simple value plus possible attributes
      template<class TYPE>
      bool writeNxValue(const std::string& name, const TYPE& value, const int nxType, 
			const std::vector<std::string>& attributes,
			const std::vector<std::string>& avalues) const;
      /// Returns true if the given property is a time series property
      bool isTimeSeries(Kernel::Property* prop) const;
      /// Write a time series log entry
      bool writeTimeSeriesLog(Kernel::Property* prop) const;
      /// Write a single value log entry
      bool writeSingleValueLog(Kernel::Property* prop) const;
      /// Writes a numeric log to the Nexus file
      template<class T>
      void writeNumericTimeLog(const Kernel::TimeSeriesProperty<T> *timeSeries) const;
      /// Write a single valued NXLog entry to the Nexus file
      template<class TYPE>
      bool writeSingleValueNXLog(const std::string& name, const TYPE& value, const int nxType,
				 const std::vector<std::string>& attributes,
				 const std::vector<std::string>& avalues) const;
      /// write an NXnote with standard fields (but NX_CHAR rather than NX_BINARY data)
      bool writeNxNote(const std::string& noteName, const std::string& author, const std::string& date,
                         const std::string& description, const std::string& pairValues) const;
      /// write a float array along with any defined attributes
      bool writeNxFloatArray(const std::string& name, const std::vector<double>& values, 
			     const std::vector<std::string>& attributes,const std::vector<std::string>& avalues) const;
      /// write a char array along with any defined attributes
      bool writeNxStringArray(const std::string& name, const std::vector<std::string>& values, 
			      const std::vector<std::string>& attributes, 
			      const std::vector<std::string>& avalues) const;
      /// Write NXlog data for given string TimeSeriesProperty
      void writeNumericTimeLog_String(const Kernel::TimeSeriesProperty<std::string> *s_timeSeries) const;
      /// check if the gievn item exists in the current level
      bool checkEntryAtLevel(const std::string& item) const;
      /// check if given attribute name is in currently opened entry
      bool checkAttributeName(const std::string& target) const;
      /// Look for entry with given attribute (eg "signal")
      bool checkEntryAtLevelByAttribute(const std::string& attribute, std::string& entry) const;
      /// search for exisiting MantidWorkpace_n entries in opened file
      int findMantidWSEntries() const;
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

      /** Writes a numeric log to the Nexus file
       *  @tparam T A numeric type (double, int, bool)
       *  @param timeSeries A pointer to the log property
       */
      template<class T>
      void writeNexusTimeLog(const Kernel::TimeSeriesProperty<T> *timeSeries) const;

      /// Return the log value type as string
      template<class T>
      std::string logValueType()const{return "unknown";}

    };
    
    /**
     * Write a single valued entry to the Nexus file
     * @param name The name of the entry
     * @param value The value of the entry
     * @param nxType The nxType of the entry
     * @param attributes A list of attributes 1:1 mapped to their values in the <code>avalues</code> argument
     * @param avalues A list of attribute values in the same order as the <code>attributes</code> argument
     * @returns A boolean indicating success or failure
     */
    template<class TYPE>
    bool NexusFileIO::writeNxValue(const std::string& name, const TYPE& value, const int nxType,
				   const std::vector<std::string>& attributes,
				   const std::vector<std::string>& avalues) const
    {
      int dimensions[1] = { 1 };
      if( NXmakedata(fileID, name.c_str(), nxType, 1, dimensions) == NX_ERROR ) return false;
      if( NXopendata(fileID, name.c_str()) == NX_ERROR )return false;
      for(unsigned int it=0; it < attributes.size(); ++it)
      {
        NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size()+1, NX_CHAR);
      }
      NXputdata(fileID, (void*)&value);
      NXclosedata(fileID);
      return true;
    }

    /**
     * Write a single valued entry to the Nexus file (specialization for a string)
     * @param name The name of the entry
     * @param value The value of the entry
     * @param nxType The nxType of the entry
     * @param attributes A list of attributes 1:1 mapped to their values in the <code>avalues</code> argument
     * @param avalues A list of attribute values in the same order as the <code>attributes</code> argument
     * @returns A boolean indicating success or failure
     */
    template<>
    inline bool NexusFileIO::writeNxValue(const std::string& name, const std::string & value, const int nxType,
						const std::vector<std::string>& attributes,
						const std::vector<std::string>& avalues) const
    {
      (void)nxType;
      int dimensions[1] = { 0 };
      std::string nxstr = value;
      if( nxstr.empty() ) nxstr += " ";
      dimensions[0] = nxstr.size() + 1;
      if( NXmakedata(fileID, name.c_str(), nxType, 1, dimensions) == NX_ERROR ) return false;
      if( NXopendata(fileID, name.c_str()) == NX_ERROR )return false;
      for(unsigned int it=0; it < attributes.size(); ++it)
      {
        NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size()+1, NX_CHAR);
      }
      NXputdata(fileID, (void*)nxstr.c_str());
      NXclosedata(fileID);
      return true;
    }
    
    /**
     * Write a single valued NXLog entry to the Nexus file
     * @param name The name of the entry
     * @param value The value of the entry
     * @param nxType The nxType of the entry
     * @param attributes A list of attributes 1:1 mapped to their values in the <code>avalues</code> argument
     * @param avalues A list of attribute values in the same order as the <code>attributes</code> argument
     * @returns A boolean indicating success or failure
     */
    template<class TYPE>
    bool NexusFileIO::writeSingleValueNXLog(const std::string& name, const TYPE& value, const int nxType,
					    const std::vector<std::string>& attributes,
					    const std::vector<std::string>& avalues) const
    {
      if( NXmakegroup(fileID, name.c_str(),"NXlog") == NX_ERROR ) return false;
      NXopengroup(fileID, name.c_str(), "NXlog");
      int dimensions[1] = { 1 };
      if( NXmakedata(fileID, "value", nxType, 1, dimensions) == NX_ERROR ) return false;
      if( NXopendata(fileID, "value") == NX_ERROR )return false;
      for(unsigned int it=0; it < attributes.size(); ++it)
      {
        NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size()+1, NX_CHAR);
      }
      NXputdata(fileID, (void*)&value);
      NXclosedata(fileID);
      NXclosegroup(fileID);
      return true;
    }

    /**
     * Write a single valued NXLog entry to the Nexus file (specialization for a string)
     * @param name The name of the entry
     * @param value The value of the entry
     * @param nxType The nxType of the entry
     * @param attributes A list of attributes 1:1 mapped to their values in the <code>avalues</code> argument
     * @param avalues A list of attribute values in the same order as the <code>attributes</code> argument
     * @returns A boolean indicating success or failure
     */
    template<>
    inline bool NexusFileIO::writeSingleValueNXLog(const std::string& name, const std::string& value, const int nxType,
							 const std::vector<std::string>& attributes,
							 const std::vector<std::string>& avalues) const
    {
      (void)nxType;
      if( NXmakegroup(fileID, name.c_str(),"NXlog") == NX_ERROR ) return false;
      NXopengroup(fileID, name.c_str(), "NXlog");
      int dimensions[1] = { 0 };
      std::string nxstr = value;
      if( nxstr.empty() ) nxstr += " ";
      dimensions[0] = nxstr.size() + 1; // Allow for null-terminator
      if( NXmakedata(fileID, "value", NX_CHAR, 1, dimensions) == NX_ERROR ) return false;
      if( NXopendata(fileID, "value") == NX_ERROR )return false;
      for(unsigned int it=0; it < attributes.size(); ++it)
      {
        NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size()+1, NX_CHAR);
      }
      NXputdata(fileID, (void*)nxstr.c_str());
      NXclosedata(fileID);
      NXclosegroup(fileID);
      return true;
    }

    

    /** Writes a numeric log to the Nexus file
     *  @tparam T A numeric type (double, int, bool)
     *  @param timeSeries A pointer to the log property
     */
    template<class T>
    void NexusFileIO::writeNumericTimeLog(const Kernel::TimeSeriesProperty<T> *timeSeries) const
    {
      // write NXlog section for double values
      NXstatus status;
      // get a name for the log, possibly removing the the path component
      std::string logName=timeSeries->name();
      size_t ipos=logName.find_last_of("/\\");
      if(ipos!=std::string::npos)
        logName=logName.substr(ipos+1);
      // extract values from timeseries
      std::map<Kernel::DateAndTime, T> dV=timeSeries->valueAsMap();
      std::vector<double> values;
      std::vector<double> times;
      Kernel::DateAndTime t0;
      bool first=true;
      for(typename std::map<Kernel::DateAndTime, T>::const_iterator dv=dV.begin();dv!=dV.end();dv++)
      {
        T val = dv->second;
        Kernel::DateAndTime time = dv->first;
        values.push_back(val);
        if(first)
        {
          t0=time; // start time of log
          first=false;
        }
        times.push_back( Kernel::DateAndTime::seconds_from_duration(time-t0));
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
      // get ISO time, and save it as an attribute
      attributes.push_back("start");
      avalues.push_back( t0.to_ISO8601_string() );

      writeNxFloatArray("time", times,  attributes, avalues);
      status=NXclosegroup(fileID);
    }


  } // namespace NeXus
} // namespace Mantid

#endif /* NEXUSFILEIO_H */
