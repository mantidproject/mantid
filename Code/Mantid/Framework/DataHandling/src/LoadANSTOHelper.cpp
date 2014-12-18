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

namespace Mantid {
namespace DataHandling {
namespace ANSTO {
// ProgressTracker
ProgressTracker::ProgressTracker(API::Progress &progBar, const char *msg,
                                 int64_t target, size_t count)
    : m_msg(msg), m_count(count), m_step(target / count), m_next(m_step),
      m_progBar(progBar) {

  m_progBar.doReport(m_msg);
}
ProgressTracker::~ProgressTracker() { complete(); }
void ProgressTracker::update(int64_t position) {
  while (m_next <= position) {
    m_progBar.report(m_msg);

    switch (m_count) {
    case 0:
      return;

    case 1:
      m_count = 0;
      m_next = std::numeric_limits<int64_t>::max();
      return;

    default:
      m_count--;
      m_next += m_step;
    }
  }
}
void ProgressTracker::complete() {
  if (m_count != 0) {
    m_progBar.reportIncrement(m_count, m_msg);
    m_count = 0;
  }
}

// EventCounter
EventCounter::EventCounter(std::vector<size_t> &eventCounts,
                           const std::vector<bool> &mask)
    : m_eventCounts(eventCounts), m_mask(mask),
      m_tofMin(std::numeric_limits<double>::max()),
      m_tofMax(std::numeric_limits<double>::min()) {}
double EventCounter::tofMin() const {
  return m_tofMin <= m_tofMax ? m_tofMin : 0.0;
}
double EventCounter::tofMax() const {
  return m_tofMin <= m_tofMax ? m_tofMax : 0.0;
}
void EventCounter::addEvent(size_t s, double tof) {
  if (m_mask[s]) {
    if (m_tofMin > tof)
      m_tofMin = tof;
    if (m_tofMax < tof)
      m_tofMax = tof;

    m_eventCounts[s]++;
  }
}

// EventAssigner
EventAssigner::EventAssigner(std::vector<EventVector_pt> &eventVectors,
                             const std::vector<bool> &mask)
    : m_eventVectors(eventVectors), m_mask(mask) {}
void EventAssigner::addEvent(size_t s, double tof) {
  if (m_mask[s])
    m_eventVectors[s]->push_back(tof);
}

// FastReadOnlyFile
#ifdef WIN32
FastReadOnlyFile::FastReadOnlyFile(const char *filename) {
  m_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}
FastReadOnlyFile::~FastReadOnlyFile() {
  CloseHandle(m_handle);
  m_handle = NULL;
}
void *FastReadOnlyFile::handle() const { return m_handle; }
bool FastReadOnlyFile::read(void *buffer, uint32_t size) {
  DWORD bytesRead;
  return (FALSE != ReadFile(m_handle, buffer, size, &bytesRead, NULL)) &&
         (bytesRead == size);
}
bool FastReadOnlyFile::seek(int64_t offset, int whence, int64_t *newPosition) {
  return FALSE != SetFilePointerEx(m_handle, *(LARGE_INTEGER *)&offset,
                                   (LARGE_INTEGER *)newPosition, whence);
}
#else
FastReadOnlyFile::FastReadOnlyFile(const char *filename) {
  m_handle = fopen(filename, "rb");
}
FastReadOnlyFile::~FastReadOnlyFile() {
  fclose(m_handle);
  m_handle = NULL;
}
void *FastReadOnlyFile::handle() const { return m_handle; }
bool FastReadOnlyFile::read(void *buffer, uint32_t size) {
  return 1 == fread(buffer, (size_t)size, 1, m_handle);
}
bool FastReadOnlyFile::seek(int64_t offset, int whence, int64_t *newPosition) {
  return (0 == fseek(m_handle, offset, whence)) &&
         ((newPosition == NULL) ||
          (0 <= (*newPosition = (int64_t)ftell(m_handle))));
}
#endif

namespace Tar {

template <size_t N> int64_t octalToInt(char (&str)[N]) {
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
File::File(const std::string &path)
    : m_good(true), m_file(path.c_str()), m_selected((size_t)-1), m_position(0),
      m_size(0), m_bufferPosition(0), m_bufferAvailable(0) {

  m_good = m_file.handle() != NULL;
  while (m_good) {
    EntryHeader header;
    int64_t position;

    m_good &= m_file.read(&header, sizeof(EntryHeader));
    m_good &= m_file.seek(512 - sizeof(EntryHeader), SEEK_CUR, &position);
    if (!m_good)
      break;

    std::string fileName(header.FileName);
    if (fileName.length() == 0)
      return;

    FileInfo fileInfo;
    fileInfo.Offset = position;
    fileInfo.Size = octalToInt(header.FileSize);

    if (header.TypeFlag == TarTypeFlag_NormalFile) {
      m_fileNames.push_back(fileName);
      m_fileInfos.push_back(fileInfo);
    }

    size_t offset = (size_t)(fileInfo.Size % 512);
    if (offset != 0)
      offset = 512 - offset;

    m_good &= m_file.seek(fileInfo.Size + offset, SEEK_CUR);
  }
}

// properties
bool File::good() const { return m_good; }
const std::vector<std::string> &File::files() const { return m_fileNames; }
const std::string &File::selected_name() const {
  return m_fileNames[m_selected];
}
int64_t File::selected_position() const { return m_position; }
int64_t File::selected_size() const { return m_size; }

// methods
bool File::select(const char *file) {
  if (!m_good)
    return false;

  // reset buffer
  m_bufferPosition = 0;
  m_bufferAvailable = 0;

  for (size_t i = 0; i != m_fileNames.size(); i++)
    if (m_fileNames[i] == file) {
      const FileInfo &info = m_fileInfos[i];

      m_selected = i;
      m_position = 0;
      m_size = info.Size;

      return m_good &= m_file.seek(info.Offset, SEEK_SET);
    }

  m_selected = (size_t)-1;
  m_position = 0;
  m_size = 0;
  return false;
}
bool File::skip(uint64_t offset) {
  if (!m_good || (m_selected == (size_t)-1))
    return false;

  bool overrun = offset > (uint64_t)(m_size - m_position);
  if (overrun)
    offset = m_size - m_position;

  m_position += offset;

  uint64_t bufferPosition = (uint64_t)m_bufferPosition + offset;
  if (bufferPosition <= m_bufferAvailable)
    m_bufferPosition = (size_t)bufferPosition;
  else {
    m_good &= m_file.seek(bufferPosition - m_bufferAvailable, SEEK_CUR);

    m_bufferPosition = 0;
    m_bufferAvailable = 0;
  }

  return m_good && !overrun;
}
size_t File::read(void *dst, size_t size) {
  if (!m_good || (m_selected == (size_t)-1))
    return 0;

  if ((int64_t)size > (m_size - m_position))
    size = (size_t)(m_size - m_position);

  auto ptr = (uint8_t *)dst;
  size_t result = 0;

  if (m_bufferPosition != m_bufferAvailable) {
    result = m_bufferAvailable - m_bufferPosition;
    if (result > size)
      result = size;

    memcpy(ptr, m_buffer, result);
    ptr += result;

    size -= result;
    m_position += result;
    m_bufferPosition += result;
  }

  while (size != 0) {
    auto bytesToRead =
        (uint32_t)std::min<size_t>(size, std::numeric_limits<uint32_t>::max());

    m_good &= m_file.read(ptr, bytesToRead);
    if (!m_good)
      break;

    ptr += bytesToRead;

    size -= bytesToRead;
    result += bytesToRead;
    m_position += bytesToRead;
  }

  return result;
}
int File::read_byte() {
  if (!m_good || (m_selected == (size_t)-1))
    return -1;

  if (m_bufferPosition == m_bufferAvailable) {
    if (m_position >= m_size)
      return -1;

    m_bufferPosition = 0;
    m_bufferAvailable = 0;

    uint32_t size =
        (uint32_t)std::min<int64_t>(sizeof(m_buffer), m_size - m_position);
    m_good &= m_file.read(m_buffer, size);

    if (m_good)
      m_bufferAvailable = size;
    else
      return -1;
  }

  m_position++;
  return m_buffer[m_bufferPosition++];
}
}
}
} // namespace
} // namespace
