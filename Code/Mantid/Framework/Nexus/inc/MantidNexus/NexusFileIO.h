#ifndef NEXUSFILEIO_H
#define NEXUSFILEIO_H
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <limits.h>
#include <nexus/NeXusFile.hpp>

namespace Mantid
{
  namespace NeXus
  {
    DLLExport int getNexusEntryTypes(const std::string& fileName, std::vector<std::string>& entryName,
                           std::vector<std::string>& definition );
    DLLExport int getNexusEntryTypes(::NeXus::File * handle, std::vector<std::string>& entryName,
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
      int writeNexusProcessedHeader( const std::string& title) const;
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

      /// write bin masking information
      bool writeNexusBinMasking(API::MatrixWorkspace_const_sptr ws) const;
      /// Nexus file handle
      NXhandle fileID;

    private:
      /// C++ API file handle
      ::NeXus::File *m_filehandle;
      /// Nexus compression method
      int m_nexuscompression;
      /// Nexus cpp compression method
      ::NeXus::NXcompression m_cppcompression;
      /// Allow an externally supplied progress object to be used
      API::Progress *m_progress;

      template<class T>
      void writeEventListData( std::vector<T> events, bool writeTOF, bool writePulsetime, bool writeWeight, bool writeError) const;
      /// Write a simple value plus possible attributes
      template<class TYPE>
      void writeNxValue(const std::string& name, const TYPE& value,
                        const std::vector<std::string>& attributes,
                        const std::vector<std::string>& avalues) const;
      void writeNXdata( const std::string& name, ::NeXus::NXnumtype datatype, std::vector<int64_t>& dims_array,
                        void * data, bool compress = false) const;
      template<class TYPE>
      void writeNXdata( const std::string& name, std::vector<TYPE>& data, bool compress = false) const;
      /// Add attributes to the name data
      void putAttr(const std::string& name, const std::vector<std::string>& attributes,
                   const std::vector<std::string>& avalues) const;
      /// Add attributes to the currently open node
      void putAttr(const std::vector<std::string>& attributes,
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
      void writeNxNote(const std::string& noteName, const std::string& author, const std::string& date,
                       const std::string& description, const std::string& pairValues) const;
      /// write a char array along with any defined attributes
      void writeNxStringArray(const std::string& name, const std::vector<std::string>& values,
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

      /// nexus file name
      std::string m_filename;
      ///static reference to the logger class
      static Kernel::Logger& g_log;

      /** Writes a numeric log to the Nexus file
       *  @tparam T A numeric type (double, int, bool)
       *  @param timeSeries :: A pointer to the log property
       */
      template<class T>
      void writeNexusTimeLog(const Kernel::TimeSeriesProperty<T> *timeSeries) const;

      /// Return the log value type as string
      template<class T>
      std::string logValueType()const{return "unknown";}

    };



  } // namespace NeXus
} // namespace Mantid

#endif /* NEXUSFILEIO_H */
