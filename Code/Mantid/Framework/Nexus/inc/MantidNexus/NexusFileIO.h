#ifndef NEXUSFILEIO_H
#define NEXUSFILEIO_H
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/VectorColumn.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/scoped_array.hpp>

#include <limits.h>
#include <nexus/NeXusFile.hpp>

namespace Mantid
{
  namespace NeXus
  {
    DLLExport int getNexusEntryTypes(const std::string& fileName, std::vector<std::string>& entryName,
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

    File change history is stored at: <https://github.com/mantidproject/mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport NexusFileIO
    {
    public:
      /// Default constructor
      NexusFileIO();

      /// Contructor with Progress suplied
      NexusFileIO( API::Progress* prog );

      /// Destructor
      ~NexusFileIO() {}

      /// open the nexus file for writing
      void openNexusWrite(const std::string& fileName);
      /// write the header ifon for the Mantid workspace format
      int writeNexusProcessedHeader( const std::string& title, const std::string& wsName="") const;
      /// close the nexus file
      void closeNexusFile();
      /// Write a lgos section
      int writeNexusSampleLogs( const Mantid::API::Run& runProperties) const;
      /// write the workspace data
      int writeNexusProcessedData2D( const API::MatrixWorkspace_const_sptr& localworkspace,
                   const bool& uniformSpectra, const std::vector<int>& spec,
                  const char * group_name, bool write2Ddata) const;

      /// write table workspace
      int writeNexusTableWorkspace( const API::ITableWorkspace_const_sptr& localworkspace,
        const char * group_name) const;

      int writeNexusProcessedDataEvent( const DataObjects::EventWorkspace_const_sptr& localworkspace);

      int writeNexusProcessedDataEventCombined( const DataObjects::EventWorkspace_const_sptr& ws,
          std::vector<int64_t> & indices, double * tofs, float * weights, float * errorSquareds, int64_t * pulsetimes,
          bool compress) const;

      int writeEventList( const DataObjects::EventList & el, std::string group_name) const;

      template<class T>
      void writeEventListData( std::vector<T> events, bool writeTOF, bool writePulsetime, bool writeWeight, bool writeError) const;
      void NXwritedata( const char * name, int datatype, int rank, int * dims_array, void * data, bool compress = false) const;

      /// find size of open entry data section
      int getWorkspaceSize( int& numberOfSpectra, int& numberOfChannels, int& numberOfXpoints ,
                bool& uniformBounds, std::string& axesNames, std::string& yUnits ) const;
      /// read X values for one (or the generic if uniform) spectra
      int getXValues(MantidVec& xValues, const int& spectra) const;
      /// read values and errors for spectra
      int getSpectra(MantidVec& values, MantidVec& errors, const int& spectra) const;

      /// write bin masking information
      bool writeNexusBinMasking(API::MatrixWorkspace_const_sptr ws) const;

      /// Nexus file handle
      NXhandle fileID;

    private:
      /// C++ API file handle
      ::NeXus::File *m_filehandle;
      /// Nexus compression method
      int m_nexuscompression;
      /// Allow an externally supplied progress object to be used
      API::Progress *m_progress;
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
      void writeNxFloatArray(const std::string& name, const std::vector<double>& values,
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
            /**
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

      /** Writes a numeric log to the Nexus file
       *  @tparam T A numeric type (double, int, bool)
       *  @param timeSeries :: A pointer to the log property
       */
      template<class T>
      void writeNexusTimeLog(const Kernel::TimeSeriesProperty<T> *timeSeries) const;

      /// Return the log value type as string
      template<class T>
      std::string logValueType()const{return "unknown";}

      /// Writes given vector column to the currently open Nexus file
      template<typename Type>
      void writeNexusVectorColumn(const boost::shared_ptr< const DataObjects::VectorColumn<Type> >& column,
                                  const std::string& columnName, int nexusType,
                                  const std::string& typeName) const;
    };
    
    /**
     * Write a single valued entry to the Nexus file
     * @param name :: The name of the entry
     * @param value :: The value of the entry
     * @param nxType :: The nxType of the entry
     * @param attributes :: A list of attributes 1:1 mapped to their values in the <code>avalues</code> argument
     * @param avalues :: A list of attribute values in the same order as the <code>attributes</code> argument
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
        NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), static_cast<int>(avalues[it].size()+1), NX_CHAR);
      }
      NXputdata(fileID, (void*)&value);
      NXclosedata(fileID);
      return true;
    }

    /**
     * Write a single valued entry to the Nexus file (specialization for a string)
     * @param name :: The name of the entry
     * @param value :: The value of the entry
     * @param nxType :: The nxType of the entry
     * @param attributes :: A list of attributes 1:1 mapped to their values in the <code>avalues</code> argument
     * @param avalues :: A list of attribute values in the same order as the <code>attributes</code> argument
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
      dimensions[0] = static_cast<int>(nxstr.size() + 1);
      if( NXmakedata(fileID, name.c_str(), nxType, 1, dimensions) == NX_ERROR ) return false;
      if( NXopendata(fileID, name.c_str()) == NX_ERROR )return false;
      for(unsigned int it=0; it < attributes.size(); ++it)
      {
        NXputattr(fileID, attributes[it].c_str(), reinterpret_cast<void*>(const_cast<char*>(avalues[it].c_str())), static_cast<int>(avalues[it].size()+1), NX_CHAR);
      }
      NXputdata(fileID, reinterpret_cast<void*>(const_cast<char*>(nxstr.c_str())));
      NXclosedata(fileID);
      return true;
    }
    
    /**
     * Write a single valued NXLog entry to the Nexus file
     * @param name :: The name of the entry
     * @param value :: The value of the entry
     * @param nxType :: The nxType of the entry
     * @param attributes :: A list of attributes 1:1 mapped to their values in the <code>avalues</code> argument
     * @param avalues :: A list of attribute values in the same order as the <code>attributes</code> argument
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
        NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), static_cast<int>(avalues[it].size()+1), NX_CHAR);
      }
      NXputdata(fileID, (void*)&value);
      NXclosedata(fileID);
      NXclosegroup(fileID);
      return true;
    }

    /**
     * Write a single valued NXLog entry to the Nexus file (specialization for a string)
     * @param name :: The name of the entry
     * @param value :: The value of the entry
     * @param nxType :: The nxType of the entry
     * @param attributes :: A list of attributes 1:1 mapped to their values in the <code>avalues</code> argument
     * @param avalues :: A list of attribute values in the same order as the <code>attributes</code> argument
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
      dimensions[0] = static_cast<int>(nxstr.size() + 1); // Allow for null-terminator
      if( NXmakedata(fileID, "value", NX_CHAR, 1, dimensions) == NX_ERROR ) return false;
      if( NXopendata(fileID, "value") == NX_ERROR )return false;
      for(unsigned int it=0; it < attributes.size(); ++it)
      {
        NXputattr(fileID, attributes[it].c_str(), reinterpret_cast<void*>(const_cast<char*>(avalues[it].c_str())), static_cast<int>(avalues[it].size()+1), NX_CHAR);
      }
      NXputdata(fileID, reinterpret_cast<void*>(const_cast<char*>(nxstr.c_str())));
      NXclosedata(fileID);
      NXclosegroup(fileID);
      return true;
    }

    

    /** Writes a numeric log to the Nexus file
     *  @tparam T A numeric type (double, int, bool)
     *  @param timeSeries :: A pointer to the log property
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
        times.push_back( Kernel::DateAndTime::secondsFromDuration(time-t0));
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
      avalues.push_back( t0.toISO8601String() );

      writeNxFloatArray("time", times,  attributes, avalues);
      status=NXclosegroup(fileID);
    }

    /**
     * Writes given vector column to the currently open Nexus file.
     * @param columnName :: Name of NXdata to write to
     * @param nexusType  :: Nexus type to use to store data
     * @param typeName   :: Name of the type to use for "interpret_as" attribute
     * @param column     :: Column to write
     */
    template<typename Type>
    void NexusFileIO::writeNexusVectorColumn(const boost::shared_ptr<const DataObjects::VectorColumn<Type> >& column,
                                             const std::string& columnName, int nexusType,
                                             const std::string& typeName) const
    {
      size_t rowCount = column->size();

      // Search for the longest array amongst the cells
      size_t maxSize(0);
      for ( size_t i = 0; i < rowCount; ++i )
      {
        size_t size = column->template cell< std::vector<Type> >(i).size();

        if ( size > maxSize )
          maxSize = size;
      }

      // Set-up dimensions
      int dims[2];
      dims[0] = static_cast<int>(rowCount);
      dims[1] = static_cast<int>(maxSize);

      // Create data array
      boost::scoped_array<Type> data(new Type[rowCount * maxSize]);

      for ( size_t i = 0; i < rowCount; ++i )
      {
        auto values = column->template cell< std::vector<Type> >(i);

        // So that all the arrays are of the size maxSize
        values.resize(maxSize);

        // Copy values to the data array
        for ( size_t j = 0; j < maxSize; ++j )
          data[i*maxSize + j] = values[j];
      }

      // Write data
      NXwritedata(columnName.c_str(), nexusType, 2, dims, reinterpret_cast<void*>(data.get()), false);

      NXopendata(fileID, columnName.c_str());

      // Add sizes of rows as attributes. We can't use padding zeroes to determine that because the
      // vector stored might end with zeroes as well.
      for ( size_t i = 0; i < rowCount; ++i )
      {
        auto size = static_cast<int>( column->template cell< std::vector<Type> >(i).size() );

        std::ostringstream rowSizeAttrName;
        rowSizeAttrName << "row_size_" << i;

        NXputattr(fileID, rowSizeAttrName.str().c_str(), &size, 1, NX_INT32);
      }

      std::string units = "Not known";
      std::string interpret_as = "A vector of " + typeName;

      // Write general attributes
      NXputattr(fileID, "units",  units.c_str(), static_cast<int>(units.size()), NX_CHAR);
      NXputattr(fileID, "interpret_as",  interpret_as.c_str(), static_cast<int>(interpret_as.size()), NX_CHAR);

      NXclosedata(fileID);
    }

  } // namespace NeXus
} // namespace Mantid

#endif /* NEXUSFILEIO_H */
