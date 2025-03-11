// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------

#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidNexus/NexusClasses_fwd.h"
#include <regex>

#define TarTypeFlag_NormalFile '0'
#define TarTypeFlag_HardLink '1'
#define TarTypeFlag_SymbolicLink '2'
#define TarTypeFlag_CharacterSpecial '3'
#define TarTypeFlag_BlockSpecial '4'
#define TarTypeFlag_Directory '5'
#define TarTypeFlag_FIFO '6'
#define TarTypeFlag_ContiguousFile '7'

namespace Mantid {
namespace DataHandling {
namespace ANSTO {

/// extract datasets from a group that match a regex filter
std::vector<std::string> filterDatasets(const NeXus::NXEntry &entry, const std::string &groupPath,
                                        const std::string &regexFilter);

/// pointer to the vector of events
using EventVector_pt = std::vector<Types::Event::TofEvent> *;

/// helper class to keep track of progress
class ProgressTracker {
private:
  // fields
  const std::string m_msg;
  size_t m_count;
  int64_t m_step;
  int64_t m_next;
  // matntid
  API::Progress &m_progBar;

public:
  // construction
  ProgressTracker(API::Progress &progBar, const char *msg, int64_t target, size_t count);
  ~ProgressTracker();

  // methods
  void update(int64_t position);
  void complete();
};

class EventProcessor {
protected:
  // fields
  const std::vector<bool> &m_roi;
  const size_t m_stride;
  // number of frames
  size_t m_frames;
  size_t m_framesValid;
  int64_t m_startTime;
  // tof correction
  const double m_period;
  const double m_phase;
  // boundaries
  const double m_tofMinBoundary;
  const double m_tofMaxBoundary;
  const double m_timeMinBoundary;
  const double m_timeMaxBoundary;

  // methods
  bool validFrame() const;
  virtual void addEventImpl(size_t id, int64_t pulse, double tof) = 0;

public:
  // construction
  EventProcessor(const std::vector<bool> &roi, size_t stride, const double period, const double phase,
                 const int64_t startTime, const double tofMinBoundary, const double tofMaxBoundary,
                 const double timeMinBoundary, const double timeMaxBoundary);

  // methods
  void newFrame();
  void addEvent(size_t x, size_t y, double tof);
};

class EventCounter : public EventProcessor {
protected:
  // fields
  std::vector<size_t> &m_eventCounts;
  // tof
  double m_tofMin;
  double m_tofMax;

  // methods
  void addEventImpl(size_t id, int64_t pulse, double tof) override;

public:
  // construction
  EventCounter(const std::vector<bool> &roi, const size_t stride, const double period, const double phase,
               const int64_t startTime, const double tofMinBoundary, const double tofMaxBoundary,
               const double timeMinBoundary, const double timeMaxBoundary, std::vector<size_t> &eventCounts);

  // properties
  size_t numFrames() const;
  double tofMin() const;
  double tofMax() const;
};

class EventAssigner : public EventProcessor {
protected:
  // fields
  std::vector<EventVector_pt> &m_eventVectors;

  // methods
  void addEventImpl(size_t id, int64_t pulse, double tof) override;

public:
  // construction
  EventAssigner(const std::vector<bool> &roi, const size_t stride, const double period, const double phase,
                int64_t startTime, const double tofMinBoundary, const double tofMaxBoundary,
                const double timeMinBoundary, const double timeMaxBoundary, std::vector<EventVector_pt> &eventVectors);
};

class EventAssignerFixedWavelength : public EventAssigner {
protected:
  // fields
  double m_wavelength;

  // methods
  void addEventImpl(size_t id, int64_t pulse, double tof) override;

public:
  // construction
  EventAssignerFixedWavelength(const std::vector<bool> &roi, const size_t stride, const double wavelength,
                               const double period, const double phase, const int64_t startTime,
                               const double tofMinBoundary, const double tofMaxBoundary, const double timeMinBoundary,
                               const double timeMaxBoundary, std::vector<EventVector_pt> &eventVectors);
};

class FastReadOnlyFile {
private:
#ifdef _WIN32
  HANDLE m_handle;
#else
  FILE *m_handle;
#endif
public:
  // construction
  FastReadOnlyFile(const char *filename);
  ~FastReadOnlyFile();

  // Prevent copying of a file handle
  FastReadOnlyFile(const FastReadOnlyFile &) = delete;
  FastReadOnlyFile &operator=(FastReadOnlyFile) = delete;

  // properties
  void *handle() const;

  // methods
  void close();
  bool read(void *buffer, uint32_t size);
  bool seek(int64_t offset, int whence, int64_t *newPosition = nullptr);
};

namespace Tar {

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
  char FileSize[12]; // in bytes (octal base)
  // cppcheck-suppress unusedStructMember
  char LastModification[12]; // time in numeric Unix time format (octal)
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

  // methods
  void writeChecksum();
  void writeFileSize(int64_t value);
  int64_t readFileSize();
};

class File {

  static const auto BUFFER_SIZE = 4096;

  struct FileInfo {
    int64_t Offset;
    int64_t Size;
  };

private:
  // fields
  bool m_good;
  FastReadOnlyFile m_file;
  std::vector<std::string> m_fileNames;
  std::vector<FileInfo> m_fileInfos;
  // selected file
  size_t m_selected; // index
  int64_t m_position;
  int64_t m_size;
  // buffer
  uint8_t m_buffer[BUFFER_SIZE];
  size_t m_bufferPosition;
  size_t m_bufferAvailable;

  // not supported
  File(const File &);
  File &operator=(const File &);

public:
  // construction
  File(const std::string &path);
  void close();

  // properties
  bool good() const;
  const std::vector<std::string> &files() const;
  // from selected file
  const std::string &selected_name() const;
  int64_t selected_position() const;
  int64_t selected_size() const;

  // methods
  bool select(const char *file);
  bool skip(uint64_t offset);
  size_t read(void *dst, size_t size);
  int read_byte();

  // helpers
  static bool append(const std::string &path, const std::string &name, const void *buffer, size_t size);
};

} // namespace Tar
} // namespace ANSTO
} // namespace DataHandling
} // namespace Mantid
