#include "MantidDataHandling/LoadANSTOHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidNexus/NexusClasses.h"

//#include <Poco/File.h>

namespace Mantid
{
  namespace DataHandling
  {
    namespace ANSTO
    {
      // ProgressTracker
      ProgressTracker::ProgressTracker(API::Progress &progBar, const char *msg, int64_t target, size_t count) :
        _msg(msg), _count(count), _step(target / count), _next(_step),
        _progBar(progBar) {

        _progBar.doReport(_msg);
      }
      ProgressTracker::~ProgressTracker() {
        complete();
      }
      void ProgressTracker::update(int64_t position) {
          if (_next <= position) {
            if (_count != 0) {
              _progBar.report(_msg);
              _next += _step;
              _count--;
            }
            else {
              _next = -1;
            }
          }
      }
      void ProgressTracker::complete() {
        if (_count != 0) {
          _progBar.reportIncrement(_count, _msg);
          _count = 0;
        }
      }
        
      // EventCounter
      EventCounter::EventCounter(std::vector<size_t> &eventCounts, const std::vector<bool> &mask) :
        _eventCounts(eventCounts),
        _mask(mask),
        _tofMin(std::numeric_limits<double>::max()),
        _tofMax(std::numeric_limits<double>::min()) {
      }
      double EventCounter::tofMin() const {
        return _tofMin <= _tofMax ? _tofMin : 0.0;
      }
      double EventCounter::tofMax() const {
        return _tofMin <= _tofMax ? _tofMax : 0.0;
      }
      void EventCounter::addEvent(size_t s, double tof) {
        if (_mask[s]) {
          if (_tofMin > tof)
            _tofMin = tof;
          if (_tofMax < tof)
            _tofMax = tof;

          _eventCounts[s]++;
        }
      }

      // EventAssigner
      EventAssigner::EventAssigner(std::vector<EventVector_pt> &eventVectors, const std::vector<bool> &mask) :
        _eventVectors(eventVectors),
        _mask(mask) {
      }
      void EventAssigner::addEvent(size_t s, double tof) {
        if (_mask[s])
          _eventVectors[s]->push_back(tof);
      }

      // FastReadOnlyFile
#ifdef WIN32
      FastReadOnlyFile::FastReadOnlyFile(const char *filename) {
        _handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      }
      FastReadOnlyFile::~FastReadOnlyFile() {
        CloseHandle(_handle);
        _handle = NULL;
      }
      void* FastReadOnlyFile::handle() const {
        return _handle;
      }
      bool FastReadOnlyFile::read(void *buffer, uint32_t size) {
          DWORD bytesRead;
          return (FALSE != ReadFile(_handle, buffer, size, &bytesRead, NULL)) && (bytesRead == size);
      }
      bool FastReadOnlyFile::seek(int64_t offset, int whence, int64_t *newPosition) {
        return FALSE != SetFilePointerEx(
          _handle,
          *(LARGE_INTEGER*)&offset,
          (LARGE_INTEGER*)newPosition,
          whence);
      }
#else
      FastReadOnlyFile::FastReadOnlyFile(const char *filename) {
        _handle = fopen(filename, "rb");
      }
      FastReadOnlyFile::~FastReadOnlyFile() {
        fclose(_handle);
        _handle = NULL;
      }
      void* FastReadOnlyFile::handle() const {
        return _handle;
      }
      bool FastReadOnlyFile::read(void *buffer, uint32_t size) {
        return 1 == fread(buffer, (size_t)size, 1, _handle);
      }
      bool FastReadOnlyFile::seek(int64_t offset, int whence, int64_t *newPosition) {
        return
          (0 == fseek(_handle, offset, whence)) &&
          ((newPosition == NULL) || (0 <= (*newPosition = (int64_t)ftell(_handle))));
      }
#endif
      
      namespace Tar {

        template<size_t N>
        int64_t octalToInt(char (&str)[N]) {
            int64_t result = 0;
            char *p = str;
            for (size_t n = N; n > 1; --n) { // last character is '\0'
              char c = *p++;
              if (('0' <= c) && (c <= '9'))
                result = result * 8 + (c - '0');
            }
            return result;
        }

        // construction
        File::File(const std::string &path) :
          _good(true),
          _file(path.c_str()),
          _selected((size_t)-1),
          _position(0),
          _size(0),
          _bufferPosition(0),
          _bufferAvailable(0) {

          _good = _file.handle() != NULL;
          while (_good) {
            EntryHeader header;
            int64_t position;
          
            _good &= _file.read(&header, sizeof(EntryHeader));
            _good &= _file.seek(512 - sizeof(EntryHeader), SEEK_CUR, &position);
            if (!_good)
              break;

            std::string fileName(header.FileName);
            if (fileName.length() == 0)
              return;

            FileInfo fileInfo;
            fileInfo.Offset = position;
            fileInfo.Size   = octalToInt(header.FileSize);
    
            if (header.TypeFlag == TarTypeFlag_NormalFile) {
              _fileNames.push_back(fileName);
              _fileInfos.push_back(fileInfo);
            }
    
            size_t offset = (size_t)(fileInfo.Size % 512);
            if (offset != 0)
              offset = 512 - offset;
          
            _good &= _file.seek(fileInfo.Size + offset, SEEK_CUR);
          }
        }

        // properties
        bool File::good() const {
          return _good;
        }
        const std::vector<std::string>& File::files() const {
          return _fileNames;
        }
        const std::string& File::selected_name() const {
          return _fileNames[_selected];
        }
        int64_t File::selected_position() const {
          return _position;
        }
        int64_t File::selected_size() const {
          return _size;
        }

        // methods
        bool File::select(const char *file) {
          if (!_good)
            return false;

          // reset buffer
          _bufferPosition  = 0;
          _bufferAvailable = 0;

          for (size_t i = 0; i != _fileNames.size(); i++)
            if (_fileNames[i] == file) {
              const FileInfo &info = _fileInfos[i];

              _selected = i;
              _position = 0;
              _size     = info.Size;
      
              return _good &= _file.seek(info.Offset, SEEK_SET);
            }

          _selected = (size_t)-1;
          _position = 0;
          _size     = 0;
          return false;
        }
        bool File::skip(uint64_t offset) {
          if (!_good || (_selected == (size_t)-1))
            return false;
  
          bool overrun = offset > (uint64_t)(_size - _position);
          if (overrun)
            offset = _size - _position;

          _position += offset;

          uint64_t bufferPosition = (uint64_t)_bufferPosition + offset;
          if (bufferPosition <= _bufferAvailable)
            _bufferPosition = (size_t)bufferPosition;
          else {
            _good &= _file.seek(bufferPosition - _bufferAvailable, SEEK_CUR);

            _bufferPosition  = 0;
            _bufferAvailable = 0;
          }

          return _good && !overrun;
        }
        size_t File::read(void *dst, size_t size) {
          if (!_good || (_selected == (size_t)-1))
            return 0;
        
          if ((int64_t)size > (_size - _position))
            size = (size_t)(_size - _position);

          auto ptr = (uint8_t*)dst;
          size_t result = 0;

          if (_bufferPosition != _bufferAvailable) {
            result = _bufferAvailable - _bufferPosition;
            if (result > size)
              result = size;

            memcpy(ptr, _buffer, result);
            ptr += result;

            size -= result;
            _position += result;
            _bufferPosition += result;
          }
        
          while (size != 0) {
            auto bytesToRead = (uint32_t)std::min<size_t>(
              size,
              std::numeric_limits<uint32_t>::max());

            _good &= _file.read(ptr, bytesToRead);
            if (!_good)
              break;

            ptr += bytesToRead;
          
            size -= bytesToRead;
            result += bytesToRead;
            _position += bytesToRead;
          }

          return result;
        }
        int File::read_byte() {
          if (!_good || (_selected == (size_t)-1))
            return -1;

          if (_bufferPosition == _bufferAvailable) {
            if (_position >= _size)
              return -1;
    
            _bufferPosition  = 0;
            _bufferAvailable = 0;

            uint32_t size = (uint32_t)std::min<int64_t>(sizeof(_buffer), _size - _position);
            _good &= _file.read(_buffer, size);

            if (_good)
              _bufferAvailable = size;
            else
              return -1;
          }

          _position++;
          return _buffer[_bufferPosition++];
        }

      }
    }
  }//namespace
}//namespace
