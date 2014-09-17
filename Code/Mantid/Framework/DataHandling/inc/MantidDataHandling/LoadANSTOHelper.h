#ifndef DATAHANDING_ANSTO_H_
#define DATAHANDING_ANSTO_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------

#include "MantidAPI/IFileLoader.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidNexus/NexusClasses.h"

#define TarTypeFlag_NormalFile        '0'
#define TarTypeFlag_HardLink          '1'
#define TarTypeFlag_SymbolicLink      '2'
#define TarTypeFlag_CharacterSpecial  '3'
#define TarTypeFlag_BlockSpecial      '4'
#define TarTypeFlag_Directory         '5'
#define TarTypeFlag_FIFO              '6'
#define TarTypeFlag_ContiguousFile    '7'

namespace Mantid
{
  namespace DataHandling
  {
    namespace ANSTO
    {
      // pointer to the vector of events
      typedef std::vector<DataObjects::TofEvent> *EventVector_pt;

      // helper class to keep track
      class ProgressTracker {
      private:
        // fields
        const std::string _msg;
        size_t _count;
        int64_t _step;
        int64_t _next;
        // matntid
        API::Progress &_progBar;

      public:
        // construction
        ProgressTracker(API::Progress &progBar, const char *msg, int64_t target, size_t count);
        ~ProgressTracker();

        // methods
        void update(int64_t position);
        void complete();
      };
    
      class EventCounter {
      private:
        // fields
        std::vector<size_t> &_eventCounts;
        const std::vector<bool> &_mask;
        double _tofMin;
        double _tofMax;

      public:
        // construction
        EventCounter(std::vector<size_t> &eventCounts, const std::vector<bool> &mask);

        // properties
        double tofMin() const;
        double tofMax() const;

        // methods
        void addEvent(size_t s, double tof);
      };
    
      class EventAssigner {
      private:
        // fields
        std::vector<EventVector_pt> &_eventVectors;
        const std::vector<bool> &_mask;

      public:
        // construction
        EventAssigner(std::vector<EventVector_pt> &eventVectors, const std::vector<bool> &mask);
      
        // methods
        void addEvent(size_t s, double tof);
      };

      class FastReadOnlyFile {
      private:
#ifdef WIN32
        HANDLE _handle;
#else
        FILE *_handle;
#endif
      public:
        // construction
        FastReadOnlyFile(const char *filename);
        ~FastReadOnlyFile();

        // properties
        void* handle() const;

        // methods
        bool read(void *buffer, uint32_t size);
        bool seek(int64_t offset, int whence, int64_t *newPosition = NULL);
      };
      
      namespace Tar
      {
        struct EntryHeader {
          // cppcheck-suppress unusedStructMember
          char FileName[100];
          // cppcheck-suppress unusedStructMember
          char FileMode[8];
          // cppcheck-suppress unusedStructMember
          char OwnerUserID[8];
          // cppcheck-suppress unusedStructMember
          char OwnerGroupID[8];
          // cppcheck-suppress unusedStructMember
          char FileSize[12];          // in bytes (octal base)
          // cppcheck-suppress unusedStructMember
          char LastModification[12];  // time in numeric Unix time format (octal)
          // cppcheck-suppress unusedStructMember
          char Checksum[8];
          // cppcheck-suppress unusedStructMember
          char TypeFlag;
          // cppcheck-suppress unusedStructMember
          char LinkedFileName[100];
          // cppcheck-suppress unusedStructMember
          char UStar[8];
          // cppcheck-suppress unusedStructMember
          char OwnerUserName[32];
          // cppcheck-suppress unusedStructMember
          char OwnerGroupName[32];
          // cppcheck-suppress unusedStructMember
          char DeviceMajorNumber[8];
          // cppcheck-suppress unusedStructMember
          char DeviceMinorNumber[8];
          // cppcheck-suppress unusedStructMember
          char FilenamePrefix[155];
        };
      
        template<size_t N>
        int64_t octalToInt(char (&str)[N]);
      
        class File {

          static const auto BufferSize = 4096;
  
          struct FileInfo {
            int64_t Offset;
            int64_t Size;
          };

        private:
          // fields
          bool  _good;
          FastReadOnlyFile _file;
          std::vector<std::string> _fileNames;
          std::vector<FileInfo>    _fileInfos;
          // selected file
          size_t _selected; // index
          int64_t _position;
          int64_t _size;
          // buffer
          uint8_t _buffer[BufferSize];
          size_t _bufferPosition;
          size_t _bufferAvailable;

          // not supported
          File(const File&);
          File& operator =(const File&);

        public:
          // construction
          File(const std::string &path);

          // properties
          bool good() const;
          const std::vector<std::string>& files() const;
          // from selected file
          const std::string& selected_name() const;
          int64_t selected_position() const;
          int64_t selected_size() const;
  
          // methods
          bool select(const char *file);
          bool skip(uint64_t offset);
          size_t read(void *dst, size_t size);
          int read_byte();
        };

      }
    }
  }
}
#endif //DATAHANDING_ANSTO_H_