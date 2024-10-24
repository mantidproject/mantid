// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "KafkaTesting.h"
#include "MantidAPI/Workspace_fwd.h"

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

/**
 * Wrapper for KafkaHisto/Event stream classes that
 * handles thread stepping to prevent race conditions in the unit tests.
 *
 * Either the main thread or Kafka thread is allowed to continue, with
 * the other blocking until either: an iteration is done in Kafka or
 * the test thread explicitly calls to unblock Kafka, whilst blocking.
 *
 * This class also contains deadlock detection, for if you haven't send
 * Kafka through enough iterations or if you use the API incorrectly.
 *
 * Important!: All methods on Kafka are usable on the Kafka instance except
 * start/stop capture. Use runKafkaOneStep() or stopCapture() respectively
 * in this class to handle spinning up and down the Kafka threads or you'll
 * deadlock!
 *
 * @author David Fairbrother
 * @date 2020/04/04
 */

namespace KafkaTesting {

template <typename KafkaT> class KafkaTestThreadHelper {
public:
  KafkaTestThreadHelper(KafkaT &&testInstance) : instance(std::move(testInstance)) {
    instance.registerIterationEndCb([this] { callback(); });
    instance.registerErrorCb([this] { errCallback(); });
  }

  ~KafkaTestThreadHelper() {
    {
      std::unique_lock lock(mutex);
      blockedThread = Threads::NONE;
    }
    cv.notify_all();
  }

  KafkaT *operator->() { return &instance; }

  void runKafkaOneStep() {
    std::unique_lock lock(mutex);
    if (!isCapturing) {
      instance.startCapture();
      isCapturing = true;
    }

    blockedThread = Threads::TEST;

    cv.notify_one();
    // Make test wait until were told Kafka shouldn't be blocked
    cv.wait_for(lock, DEADLOCK_TIMEOUT, [this] { return blockedThread != Threads::TEST; });

    assert(blockedThread != Threads::TEST && "Deadlock was detected as test thread was not unblocked");

    // Kafka is now blocked test can resume
  }

  void stopCapture() {
    // Kafka spins the calling thread whilst waiting for
    // its worker thread, which is currently have paused.
    //
    // We need a third thread to spin, whilst this thread
    // keeps everything moving along nicely

    auto stopCaptureThread = std::async(std::launch::async, [this] {
      instance.stopCapture();
      {
        std::lock_guard lock(mutex);
        blockedThread = Threads::NONE;
        isCapturing = false;
      }
      cv.notify_all();
    });

    while (true) {
      Threads blockedThreadCopy;
      {
        std::lock_guard lck(mutex);
        // Take a copy since only we or Kafka can change this var
        // so if Kafka is in a wait it can't change under us
        // But makes locking easier
        blockedThreadCopy = blockedThread;
      }

      if (blockedThreadCopy == Threads::NONE) {
        break;
      }

      if (blockedThreadCopy == Threads::KAFKA) {
        runKafkaOneStep();
      }
    }

    stopCaptureThread.wait();
  }

private:
  enum class Threads { NONE, KAFKA, TEST };

  void callback() { holdKafkaForTestClass(); }

  void errCallback() {
    try {
      // Get the stored exception by calling extract data again
      instance.extractData();
    } catch (std::exception &e) {
      // We could try to port the exception from this child thread to the main
      // thread, or just print it here which is significantly easier
      std::cerr << "Exception: " << e.what() << "\n";
    }
    // Always keep incrementing so we don't deadlock
    holdKafkaForTestClass();
  }

  void holdKafkaForTestClass() {
    std::unique_lock lock(mutex);

    cv.notify_one();
    blockedThread = Threads::KAFKA;
    cv.wait_for(lock, DEADLOCK_TIMEOUT, [this] { return blockedThread != Threads::KAFKA; });

    assert(blockedThread != Threads::KAFKA && "Deadlock was detected as test thread was not unblocked");
  }

  // After ~2 minutes we almost certainly are deadlocked and not just
  // waiting for a really slow machine to run the full test
  const std::chrono::seconds DEADLOCK_TIMEOUT{120};

  KafkaT instance;
  bool isCapturing = false;

  std::mutex mutex;
  std::condition_variable cv;

  Threads blockedThread = Threads::NONE;
};

} // namespace KafkaTesting
