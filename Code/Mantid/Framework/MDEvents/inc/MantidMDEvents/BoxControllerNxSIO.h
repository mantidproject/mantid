#ifndef MANTID_MDEVENTS_BOXCONTROLLER_NEXUSS_IO_H
#define MANTID_MDEVENTS_BOXCONTROLLER_NEXUSS_IO_H

#include "MantidAPI/IBoxControllerIO.h"
#include "MantidAPI/BoxController.h"

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  /** The class responsible for saving events into nexus file using generic box controller interface

      @date March 15, 2013

      Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport BoxControllerNxSIO : public API::IBoxControllerIO
    {
        public:
            BoxControllerNxSIO(API::BoxController_sptr theBC);
            virtual bool openFile(const std::string &fileName,const std::string &mode);
           ///@return true if the file to write events is opened and false otherwise
            virtual bool isOpened()const
            {
                return  (m_File!=NULL);
            }
            /// get the full file name of the file used for IO operations
            virtual const std::string &getFileName()const
            {
                return m_fileName;
            }

            virtual void saveBlock(void const * const /* Block */, const uint64_t /*blockPosition*/,const size_t /*blockSize*/);
            virtual void loadBlock(void  * const  /* Block */, const uint64_t /*blockPosition*/,const size_t /*blockSize*/);
            virtual void flushData();
            virtual void closeFile();

            virtual ~BoxControllerNxSIO();
            /**Return the size of the NeXus data block used by NeXus*/ 
            size_t getDataChunk()const
            {
                return m_dataChunk;
            }
            virtual void setDataType(const size_t coordSize, const std::string &typeName);
            virtual void getDataType(size_t &coordSize, std::string &typeName);

    private:
        /// full file name (with path) of the Nexis file responsible for the IO operations (as NeXus filename has very strange properties and often trunkated to 64 bytes)
        std::string m_fileName;
        // the file Handler responsible for Nexus IO operations;
        ::NeXus::File * m_File;
        /// The size of the events block which can be written in the neXus array at once (continious part of the data block)
        size_t m_dataChunk;
        // shared pointer to the box controller, which is repsoponsible for this IO
        API::BoxController_sptr m_bc;

        /// number of bytes in the event coorinates (coord_t length). Set by  setDataType but can be defined statically with coord_t 
        unsigned int m_CoordSize;

        /// possible event types this class understands. The enum numbers have to correspond to the numbers of symbolic event types, 
        /// defined in EVENT_TYPES_SUPPORTED vector
        static enum EventType
        {
            LeanEvent=0, //< the event consisting of signal error and event coordinate
            FatEvent=1   //< the event havint the same as lean event plus RunID and detID
        };

        /// the type of event (currently MD event or MDLean event this class is deals with. 
        EventType m_EventType;
        /// the event specific data column size, which describes how many column an event is composed into and this class reads/writres
        size_t m_dataColumnSize;
       /// identifier if the file open only for reading or is  in read/write 
        bool m_ReadOnly;

        /// Default size of the events block which can be written in the NeXus array at once identified by efficiency or some other external reasons
        static enum {DATA_CHUNK=10000};


        /// the symblolic description of the event types currently supported by the class
        std::vector<std::string> m_EventsTypesSupported;
        /// data headers used for different events types
        std::vector<std::string> m_EventsTypeHeaders;

        /// The version of the MDEvents data block
        std::string m_EventsVersion;
        /// the name of the MD workspace group. Should be common with save/load, who uses this group to put other pieces of information about the workspace.
        static std::string g_EventWSGroupName;
        /// the name of the Nexus data group for saving the events
        static std::string g_EventGroupName; 
        /// the group name to save disk buffer data
        static std::string g_DBDataName;

    // helper functions:
        // prepare to write event nexus data in current data version format
        void CreateEventGroup();
        void CreateWSGroup();
        void OpenAndCheckWSGroup();
        void OpenAndCheckEventGroup();
        void getDiskBufferFileData();
        void prepareNxSToWrite_CurVersion();
        void prepareNxSToRead_CurVersion();

        static EventType TypeFromString(const std::vector<std::string> &typesSupported,const std::string typeName);

    };

}
}
#endif