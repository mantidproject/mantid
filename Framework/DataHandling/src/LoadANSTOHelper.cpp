// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadANSTOHelper.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusClasses.h"

#include <numeric>

namespace Mantid::DataHandling::ANSTO {

// Extract datasets from the group that match a regex filter
std::vector<std::string> filterDatasets(const NeXus::NXEntry &entry, const std::string &groupPath,
                                        const std::string &regexFilter) {
  std::vector<std::string> fvalues;
  auto group = entry.openNXGroup(groupPath);
  auto datasets = group.datasets();
  for (auto nxi : datasets) {
    if (std::regex_match(nxi.nxname, std::regex(regexFilter))) {
      fvalues.emplace_back(nxi.nxname);
    }
  }

  return fvalues;
}

// ProgressTracker
ProgressTracker::ProgressTracker(API::Progress &progBar, const char *msg, int64_t target, size_t count)
    : m_msg(msg), m_count(count), m_step(target / count), m_next(m_step), m_progBar(progBar) {

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

// EventProcessor
EventProcessor::EventProcessor(const std::vector<bool> &roi, const size_t stride, const double period,
                               const double phase, const int64_t startTime, const double tofMinBoundary,
                               const double tofMaxBoundary, const double timeMinBoundary, const double timeMaxBoundary)
    : m_roi(roi), m_stride(stride), m_frames(0), m_framesValid(0), m_startTime(startTime), m_period(period),
      m_phase(phase), m_tofMinBoundary(tofMinBoundary), m_tofMaxBoundary(tofMaxBoundary),
      m_timeMinBoundary(timeMinBoundary), m_timeMaxBoundary(timeMaxBoundary) {}
bool EventProcessor::validFrame() const {
  // frame boundary
  double frameTime = (static_cast<double>(m_frames) * m_period) * 1e-6; // in seconds

  return (frameTime >= m_timeMinBoundary) && (frameTime <= m_timeMaxBoundary);
}
void EventProcessor::newFrame() {
  m_frames++;
  if (validFrame())
    m_framesValid++;
}
void EventProcessor::addEvent(size_t x, size_t y, double tof) {
  // tof correction
  if (m_period > 0.0) {
    tof += m_phase;
    while (tof > m_period)
      tof -= m_period;
    while (tof < 0)
      tof += m_period;
  }

  // check if event is in valid range
  if (!validFrame())
    return;

  // ToF boundary
  if ((tof < m_tofMinBoundary) && (tof > m_tofMaxBoundary))
    return;

  // detector id
  size_t id = m_stride * x + y;

  // image size
  if ((y >= m_stride) || (id >= m_roi.size()))
    return;

  // check if neutron is in region of intreset
  if (m_roi[id]) {
    // absolute pulse time in nanoseconds
    auto frames = static_cast<double>(m_frames);
    auto frameTime = static_cast<int64_t>(m_period * frames * 1.0e3);
    int64_t pulse = m_startTime + frameTime;

    addEventImpl(id, pulse, tof);
  }
}

// EventCounter
EventCounter::EventCounter(const std::vector<bool> &roi, const size_t stride, const double period, const double phase,
                           const int64_t startTime, const double tofMinBoundary, const double tofMaxBoundary,
                           const double timeMinBoundary, const double timeMaxBoundary, std::vector<size_t> &eventCounts)
    : EventProcessor(roi, stride, period, phase, startTime, tofMinBoundary, tofMaxBoundary, timeMinBoundary,
                     timeMaxBoundary),
      m_eventCounts(eventCounts), m_tofMin(std::numeric_limits<double>::max()),
      m_tofMax(std::numeric_limits<double>::min()) {}
size_t EventCounter::numFrames() const { return m_framesValid; }
double EventCounter::tofMin() const { return m_tofMin <= m_tofMax ? m_tofMin : 0.0; }
double EventCounter::tofMax() const { return m_tofMin <= m_tofMax ? m_tofMax : 0.0; }
void EventCounter::addEventImpl(size_t id, int64_t pulse, double tof) {
  UNUSED_ARG(pulse);
  if (m_tofMin > tof)
    m_tofMin = tof;
  if (m_tofMax < tof)
    m_tofMax = tof;

  m_eventCounts[id]++;
}

// EventAssigner
EventAssigner::EventAssigner(const std::vector<bool> &roi, const size_t stride, const double period, const double phase,
                             const int64_t startTime, const double tofMinBoundary, const double tofMaxBoundary,
                             const double timeMinBoundary, const double timeMaxBoundary,
                             std::vector<EventVector_pt> &eventVectors)
    : EventProcessor(roi, stride, period, phase, startTime, tofMinBoundary, tofMaxBoundary, timeMinBoundary,
                     timeMaxBoundary),
      m_eventVectors(eventVectors) {}
void EventAssigner::addEventImpl(size_t id, int64_t pulse, double tof) {
  m_eventVectors[id]->emplace_back(tof, Types::Core::DateAndTime(pulse));
}

// EventAssignerFixedWavelength
EventAssignerFixedWavelength::EventAssignerFixedWavelength(const std::vector<bool> &roi, const size_t stride,
                                                           const double wavelength, const double period,
                                                           const double phase, const int64_t startTime,
                                                           const double tofMinBoundary, const double tofMaxBoundary,
                                                           const double timeMinBoundary, const double timeMaxBoundary,
                                                           std::vector<EventVector_pt> &eventVectors)
    : EventAssigner(roi, stride, period, phase, startTime, tofMinBoundary, tofMaxBoundary, timeMinBoundary,
                    timeMaxBoundary, eventVectors),
      m_wavelength(wavelength) {}
void EventAssignerFixedWavelength::addEventImpl(size_t id, int64_t pulse, double tof) {
  UNUSED_ARG(pulse);
  UNUSED_ARG(tof);
  m_eventVectors[id]->emplace_back(m_wavelength);
}

// ISISRawOnlyFile
#ifdef _WIN32
FastReadOnlyFile::FastReadOnlyFile(const char *filename) {
  m_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}
FastReadOnlyFile::~FastReadOnlyFile() { close(); }
void *FastReadOnlyFile::handle() const { return m_handle; }
void FastReadOnlyFile::close() {
  CloseHandle(m_handle);
  m_handle = NULL;
}
bool FastReadOnlyFile::read(void *buffer, uint32_t size) {
  DWORD bytesRead;
  return (FALSE != ReadFile(m_handle, buffer, size, &bytesRead, NULL)) && (bytesRead == size);
}
bool FastReadOnlyFile::seek(int64_t offset, int whence, int64_t *newPosition) {
  return FALSE != SetFilePointerEx(m_handle, *(LARGE_INTEGER *)&offset, (LARGE_INTEGER *)newPosition, whence);
}
#else
FastReadOnlyFile::FastReadOnlyFile(const char *filename) { m_handle = fopen(filename, "rb"); }
FastReadOnlyFile::~FastReadOnlyFile() { close(); }
void *FastReadOnlyFile::handle() const { return m_handle; }
void FastReadOnlyFile::close() {
  fclose(m_handle);
  m_handle = nullptr;
}
bool FastReadOnlyFile::read(void *buffer, uint32_t size) {
  return 1 == fread(buffer, static_cast<size_t>(size), 1, m_handle);
}
bool FastReadOnlyFile::seek(int64_t offset, int whence, int64_t *newPosition) {
  return (0 == fseek(m_handle, offset, whence)) &&
         ((newPosition == nullptr) || (0 <= (*newPosition = static_cast<int64_t>(ftell(m_handle)))));
}
#endif

namespace Tar {

void EntryHeader::writeChecksum() {
  memset(Checksum, ' ', sizeof(Checksum));
  size_t value = std::accumulate((const char *)this, (const char *)this + sizeof(EntryHeader), (size_t)0);

  std::ostringstream buffer;

  buffer << std::oct << std::setfill('0') << std::setw(static_cast<int>(sizeof(Checksum)) - 1) << value;
  std::string string = buffer.str();

  std::copy(string.cbegin(), string.cend(), Checksum);
  Checksum[string.size()] = 0;
}
void EntryHeader::writeFileSize(int64_t value) {
  std::ostringstream buffer;

  buffer << std::oct << std::setfill('0') << std::setw(static_cast<int>(sizeof(FileSize)) - 1) << value;
  std::string string = buffer.str();

  std::copy(string.cbegin(), string.cend(), FileSize);
  FileSize[string.size()] = 0;
}
int64_t EntryHeader::readFileSize() {
  int64_t result = 0;
  const char *p = FileSize;
  for (size_t n = sizeof(FileSize) - 1; n != 0; --n) { // last character is '\0'
    char c = *p++;
    if (('0' <= c) && (c <= '9'))
      result = result * 8 + (c - '0');
  }
  return result;
}

// construction
File::File(const std::string &path)
    : m_good(true), m_file(path.c_str()), m_selected(static_cast<size_t>(-1)), m_position(0), m_size(0),
      m_bufferPosition(0), m_bufferAvailable(0) {

  m_good = m_file.handle() != nullptr;
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
    fileInfo.Size = header.readFileSize();

    if (header.TypeFlag == TarTypeFlag_NormalFile) {
      m_fileNames.emplace_back(fileName);
      m_fileInfos.emplace_back(fileInfo);
    }

    auto offset = static_cast<size_t>(fileInfo.Size % 512);
    if (offset != 0)
      offset = 512 - offset;

    m_good &= m_file.seek(fileInfo.Size + offset, SEEK_CUR);
  }
}
void File::close() {
  m_good = false;
  m_file.close();
  m_fileNames.clear();
  m_fileInfos.clear();
  m_selected = static_cast<size_t>(-1);
  m_position = 0;
  m_size = 0;
  m_bufferPosition = 0;
  m_bufferAvailable = 0;
}

// properties
bool File::good() const { return m_good; }
const std::vector<std::string> &File::files() const { return m_fileNames; }
const std::string &File::selected_name() const { return m_fileNames[m_selected]; }
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

  m_selected = static_cast<size_t>(-1);
  m_position = 0;
  m_size = 0;
  return false;
}
bool File::skip(uint64_t offset) {
  if (!m_good || (m_selected == static_cast<size_t>(-1)))
    return false;

  bool overrun = offset > static_cast<uint64_t>(m_size - m_position);
  if (overrun)
    offset = m_size - m_position;

  m_position += offset;

  uint64_t bufferPosition = static_cast<uint64_t>(m_bufferPosition) + offset;
  if (bufferPosition <= m_bufferAvailable)
    m_bufferPosition = static_cast<size_t>(bufferPosition);
  else {
    m_good &= m_file.seek(bufferPosition - m_bufferAvailable, SEEK_CUR);

    m_bufferPosition = 0;
    m_bufferAvailable = 0;
  }

  return m_good && !overrun;
}
size_t File::read(void *dst, size_t size) {
  if (!m_good || (m_selected == static_cast<size_t>(-1)))
    return 0;

  if (static_cast<int64_t>(size) > (m_size - m_position))
    size = static_cast<size_t>(m_size - m_position);

  auto ptr = reinterpret_cast<uint8_t *>(dst);
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
    auto bytesToRead = static_cast<uint32_t>(std::min<size_t>(size, std::numeric_limits<uint32_t>::max()));

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
  if (!m_good || (m_selected == static_cast<size_t>(-1)))
    return -1;

  if (m_bufferPosition == m_bufferAvailable) {
    if (m_position >= m_size)
      return -1;

    m_bufferPosition = 0;
    m_bufferAvailable = 0;

    uint32_t size = static_cast<uint32_t>(std::min<int64_t>(sizeof(m_buffer), m_size - m_position));
    m_good &= m_file.read(m_buffer, size);

    if (m_good)
      m_bufferAvailable = size;
    else
      return -1;
  }

  m_position++;
  return m_buffer[m_bufferPosition++];
}
bool File::append(const std::string &path, const std::string &name, const void *buffer, size_t size) {
  std::unique_ptr<FILE, decltype(&fclose)> handle(fopen(path.c_str(), "rb+"), fclose);

  if (handle == nullptr)
    return false;

  int64_t lastHeaderPosition = 0;
  int64_t targetPosition = -1;

  bool fileStatus = true;
  while (fileStatus) {
    EntryHeader header;
    int64_t position;

    lastHeaderPosition = static_cast<int64_t>(ftell(handle.get()));

    fileStatus &= 1 == fread(&header, sizeof(EntryHeader), 1, handle.get());
    fileStatus &= 0 == fseek(handle.get(), 512 - sizeof(EntryHeader), SEEK_CUR);
    fileStatus &= 0 <= (position = static_cast<int64_t>(ftell(handle.get())));

    if (!fileStatus)
      return false;

    std::string fileName(header.FileName);
    if (fileName.length() == 0)
      break;

    if (fileName == name)
      targetPosition = lastHeaderPosition;
    else if (targetPosition != -1)
      throw std::runtime_error("format exception"); // it has to be the last file in the archive

    auto fileSize = header.readFileSize();

    auto offset = static_cast<size_t>(fileSize % 512);
    if (offset != 0)
      offset = 512 - offset;

    fileStatus &= 0 == fseek(handle.get(), static_cast<long>(fileSize + offset), SEEK_CUR);
  }

  if (targetPosition < 0)
    targetPosition = lastHeaderPosition;

  // empty buffer
  char padding[512];
  memset(padding, 0, 512);

  // prepare new header
  EntryHeader header;
  memset(&header, 0, sizeof(EntryHeader));
  memcpy(header.FileName, name.c_str(), name.size());
  memset(header.FileMode, '0', sizeof(header.FileMode) - 1);
  memset(header.OwnerUserID, '0', sizeof(header.OwnerUserID) - 1);
  memset(header.OwnerGroupID, '0', sizeof(header.OwnerGroupID) - 1);
  memset(header.LastModification, '0', sizeof(header.LastModification) - 1);

  header.TypeFlag = TarTypeFlag_NormalFile;
  header.writeFileSize(size);
  header.writeChecksum();

  // write header
  fileStatus &= 0 == fseek(handle.get(), static_cast<long>(targetPosition), SEEK_SET);
  fileStatus &= 1 == fwrite(&header, sizeof(EntryHeader), 1, handle.get());
  fileStatus &= 1 == fwrite(padding, 512 - sizeof(EntryHeader), 1, handle.get());

  // write content
  fileStatus &= 1 == fwrite(buffer, size, 1, handle.get());

  // write padding
  auto offset = static_cast<size_t>(size % 512);
  if (offset != 0) {
    offset = 512 - offset;

    fileStatus &= 1 == fwrite(padding, offset, 1, handle.get());
  }

  // write final
  fileStatus &= 1 == fwrite(padding, 512, 1, handle.get());

  return fileStatus;
}

} // namespace Tar
} // namespace Mantid::DataHandling::ANSTO
