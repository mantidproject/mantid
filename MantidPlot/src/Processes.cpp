// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Processes.h"

#include <QCoreApplication>
#include <QFileInfo>

#if defined(Q_OS_LINUX)
#include <QDir>
#include <QFile>
#elif defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#include "MantidQtWidgets/Common/QStringUtils.h"
#include <Psapi.h>
#elif defined(Q_OS_MAC)
#include <cstdlib>
#include <libproc.h>
#include <sys/sysctl.h>
#endif

namespace {

bool isOtherInstance(int64_t otherPID, QString otherExeName) {
  static const int64_t ourPID(QCoreApplication::applicationPid());
  if (otherPID == ourPID)
    return false;

  static const QString ourExeName(
      QFileInfo(QCoreApplication::applicationFilePath()).fileName());
  if (ourExeName == otherExeName)
    return true;
  else
    return false;
}

} // namespace

namespace Processes {
/**
 * Returns true is another instance of Mantid is running
 * on this machine
 * @return True if another instance is running
 * @throws std::runtime_error if this cannot be determined
 */
#ifdef Q_OS_LINUX
unsigned int numberOfMantids() {
  QDir procfs{"/proc"};

  int counter = 0;
  const QStringList entries{procfs.entryList(QDir::Dirs)};
  for (const auto &pidStr : entries) {
    bool isDigit(false);
    const long long pid{pidStr.toLongLong(&isDigit)};
    if (!isDigit)
      continue;

    // /proc/pid/exe should point to executable
    QFileInfo exe{"/proc/" + pidStr + "/exe"};
    if (!exe.exists() || !exe.isSymLink())
      continue;

    if (isOtherInstance(pid, QFileInfo(exe.symLinkTarget()).fileName())) {
      ++counter;
    }
  }
  return counter;
}
#elif defined(Q_OS_WIN)
unsigned int numberOfMantids() {
  using MantidQt::API::toQStringInternal;
  // Inspired by psutil.psutil_get_pids at
  // https://github.com/giampaolo/psutil/blob/master/psutil/arch/windows/process_info.c

  // EnumProcesses in Win32 SDK says the only way to know if our process array
  // wasn't large enough is to check the returned size and make
  // sure that it doesn't match the size of the array.
  // If it does we allocate a larger array and try again

  std::vector<DWORD> processes;
  // Stores the byte size of the returned array from EnumProcesses
  DWORD enumReturnSz{0};
  do {
    processes.resize(processes.size() + 1024);
    const DWORD procArrayByteSz =
        static_cast<DWORD>(processes.size()) * sizeof(DWORD);
    if (!EnumProcesses(processes.data(), procArrayByteSz, &enumReturnSz)) {
      throw std::runtime_error("Unable to determine running process list");
    }
  } while (enumReturnSz == processes.size());
  // Set the vector back to the appropriate size
  processes.resize(enumReturnSz / sizeof(DWORD));

  int counter = 0;
  wchar_t exe[MAX_PATH];
  for (const auto pid : processes) {
    // system-idle process
    if (pid == 0)
      continue;
    auto procHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!procHandle)
      continue;
    DWORD exeSz = GetProcessImageFileNameW(procHandle, exe, MAX_PATH);
    CloseHandle(procHandle);
    if (exeSz > 0 &&
        isOtherInstance(pid, QFileInfo(toQStringInternal(exe)).fileName())) {
      ++counter;
    }
  }
  return counter;
}
#elif defined(Q_OS_MAC)
unsigned int numberOfMantids() {
  kinfo_proc *processes[] = {nullptr};
  size_t processesLength(0);
  int sysctlQuery[3] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL};
  /*
   * We start by calling sysctl with ptr == NULL and size == 0.
   * That will succeed, and set size to the appropriate length.
   * We then allocate a buffer of at least that size and call
   * sysctl with that buffer.  If that succeeds, we're done.
   * If that call fails with ENOMEM, we throw the buffer away
   * and try again.
   * Note that the loop calls sysctl with NULL again.  This is
   * is necessary because the ENOMEM failure case sets size to
   * the amount of data returned, not the amount of data that
   * could have been returned.
   */
  int attempts = 8; // An arbitrary number of attempts to try
  void *memory{nullptr};
  while (attempts-- > 0) {
    size_t size = 0;
    if (sysctl((int *)sysctlQuery, 3, nullptr, &size, nullptr, 0) == -1) {
      throw std::runtime_error("Unable to retrieve process list");
    }
    const size_t size2 =
        size + (size >> 3); // add some to cover more popping in
    if (size2 > size) {
      memory = malloc(size2);
      if (memory == nullptr)
        memory = malloc(size);
      else
        size = size2;
    } else {
      memory = malloc(size);
    }
    if (memory == nullptr)
      throw std::runtime_error(
          "Unable to allocate memory to retrieve process list");
    if (sysctl((int *)sysctlQuery, 3, memory, &size, nullptr, 0) == -1) {
      free(memory);
      throw std::runtime_error("Unable to retrieve process list");
    } else {
      *processes = (kinfo_proc *)memory;
      processesLength = size / sizeof(kinfo_proc);
      break;
    }
  }

  kinfo_proc *processListBegin = processes[0];
  kinfo_proc *processIter = processListBegin;
  char exePath[PATH_MAX];
  int counter = 0;
  for (size_t i = 0; i < processesLength; ++i) {
    const auto pid = processIter->kp_proc.p_pid;
    if (proc_pidpath(pid, exePath, PATH_MAX) <= 0) {
      // assume process is dead...
      continue;
    }
    if (isOtherInstance(pid,
                        QFileInfo(QString::fromAscii(exePath)).fileName())) {
      ++counter;
    }
    ++processIter;
  }
  free(processListBegin);

  return counter;
}
#endif
long long getProcessID() { return QCoreApplication::applicationPid(); }
} // namespace Processes
