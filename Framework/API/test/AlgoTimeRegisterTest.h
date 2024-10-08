// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgoTimeRegister.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef __linux__ // this works only in linux
using Mantid::Instrumentation::AlgoTimeRegister;
#endif
using Mantid::Kernel::ConfigService;
using std::filesystem::exists;
using std::filesystem::remove_all;

class AlgoTimeRegisterTest : public CxxTest::TestSuite {
public:
  static AlgoTimeRegisterTest *createSuite() { return new AlgoTimeRegisterTest(); }
  static void destroySuite(AlgoTimeRegisterTest *suite) { delete suite; }

  struct timeEntry {
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    std::string threadId;
  };

#ifdef __linux__ // this works only in linux
  AlgoTimeRegisterTest() {
    if (mkdir(m_directory.c_str(), 0777) == -1) {
      std::cerr << "Error :  " << strerror(errno) << std::endl;
    }
    ConfigService::Instance().setString("performancelog.filename", m_directory + "test.log");
    ConfigService::Instance().setString("performancelog.write", "On");
    AlgoTimeRegister::Instance();
  }

  ~AlgoTimeRegisterTest() override {
    remove_all(m_directory);
    ConfigService::Instance().setString("performancelog.filename", "");
    ConfigService::Instance().setString("performancelog.write", "Off");
  }

  void countLines(const int entryCount, const std::string filename = "test.log") {
    std::ifstream fs;
    int lineCount = -1; // Start at -1 to account for the first line

    fs.open(m_directory + filename);
    TS_ASSERT(fs.is_open());

    if (fs.is_open()) {
      std::string line;
      while (std::getline(fs, line)) {
        lineCount++;
      }
    }
    fs.close();
    TS_ASSERT_EQUALS(lineCount, entryCount);
  }

  void checkTimeEntry(const std::vector<timeEntry> entries, const std::string filename = "test.log") {
    std::ifstream fs;
    fs.open(m_directory + filename);
    TS_ASSERT(fs.is_open());

    std::string line;
    std::chrono::nanoseconds startPoint(0);

    // Read the first line to get the START_POINT
    if (std::getline(fs, line)) {
      std::istringstream ss(line);
      std::string startPointLabel, maxThreadLabel;
      int64_t startPointValue;
      int maxThread;

      // Parse the START_POINT and MAX_THREAD from the line
      ss >> startPointLabel >> startPointValue >> maxThreadLabel >> maxThread;
      startPoint = std::chrono::nanoseconds(startPointValue);
    }

    for (const auto &entry : entries) {
      bool entryFound = false;

      fs.clear();
      fs.seekg(0, std::ios::beg);
      std::getline(fs, line); // Skip the first line

      // Loop through the rest of the file to find the matching entry
      while (std::getline(fs, line)) {
        // Parse the thread ID, algorithm name, start time, and end time
        std::string parsedAlgorithmName, parsedThreadId;
        int64_t parsedStartTime, parsedEndTime;
        size_t threadIdPos = line.find("ThreadID=");
        size_t namePos = line.find(", AlgorithmName=");
        size_t startTimePos = line.find(", StartTime=");
        size_t endTimePos = line.find(", EndTime=");

        if (threadIdPos != std::string::npos && namePos != std::string::npos && startTimePos != std::string::npos &&
            endTimePos != std::string::npos) {

          parsedThreadId = line.substr(threadIdPos + 9, namePos - (threadIdPos + 9));
          parsedAlgorithmName = line.substr(namePos + 16, startTimePos - (namePos + 16));
          parsedStartTime = std::stoll(line.substr(startTimePos + 12, endTimePos - (startTimePos + 12)));
          parsedEndTime = std::stoll(line.substr(endTimePos + 10));

          // Calculate the absolute times by adding START_POINT
          std::chrono::nanoseconds absoluteStartTime = startPoint + std::chrono::nanoseconds(parsedStartTime);
          std::chrono::nanoseconds absoluteEndTime = startPoint + std::chrono::nanoseconds(parsedEndTime);

          // Compare the calculated absolute times with the expected times
          if (parsedAlgorithmName == entry.name && absoluteStartTime == entry.startTime.time_since_epoch() &&
              absoluteEndTime == entry.endTime.time_since_epoch() && parsedThreadId == entry.threadId) {
            entryFound = true;
            break;
          }
        }
      }
      // Assert that the entry was found
      TSM_ASSERT("Expected time entry not found in file", entryFound);
    }
    fs.close();
  }

  void test_addTime() {
    // Record some dummy times
    auto startTime = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    auto endTime = std::chrono::high_resolution_clock::now();
    std::thread::id id = std::this_thread::get_id();
    std::ostringstream ss;
    ss << id;
    std::vector<timeEntry> entries = {{"TestAlgorithm", startTime, endTime, ss.str()}};
    // timeEntry entry = {"TestAlgorithm", startTime, endTime, ss.str()};

    // Add the time entry
    AlgoTimeRegister::Instance().addTime("TestAlgorithm", startTime, endTime);
    checkTimeEntry(entries);
    countLines(1);
  }

  void test_threadedWrite() {
    ConfigService::Instance().setString("performancelog.filename", m_directory + "threadedWrite.log");

    std::vector<timeEntry> entries;
    int entryCount = 0;

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < 24; i++) {
      // Record some dummy times
      auto startTime = std::chrono::high_resolution_clock::now();
      std::this_thread::sleep_for(std::chrono::milliseconds(47));
      auto endTime = std::chrono::high_resolution_clock::now();
      std::thread::id id = std::this_thread::get_id();
      std::ostringstream ss;
      ss << id;
      m_mutex.lock();
      entries.push_back({"TestMultiThreaded", startTime, endTime, ss.str()});
      entryCount++;
      m_mutex.unlock();
      AlgoTimeRegister::Instance().addTime("TestMultiThreaded", startTime, endTime);
    }

    checkTimeEntry(entries, "threadedWrite.log");
    countLines(entryCount, "threadedWrite.log");
  }

  void test_writeDisabled() {
    ConfigService::Instance().setString("performancelog.write", "Off");
    ConfigService::Instance().setString("performancelog.filename", m_directory + "noWrite.log");

    // Record some dummy times
    auto startTime = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    auto endTime = std::chrono::high_resolution_clock::now();
    std::thread::id id = std::this_thread::get_id();
    std::ostringstream ss;
    ss << id;
    std::vector<timeEntry> entries = {{"TestAlgorithm", startTime, endTime, ss.str()}};

    // Add the time entry
    AlgoTimeRegister::Instance().addTime("TestAlgorithm", startTime, endTime);
    TS_ASSERT(!exists(m_directory + "noWrite.log"));
  }
#endif

  void test_skipAddTime() { TS_TRACE("This test is only available on Linux"); }

private:
  const std::string m_directory = "AlgoTimeRegisterTest/";
  std::mutex m_mutex;
};
