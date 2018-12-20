// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_TESTHELPERS_PARALLELRUNNER_H_
#define MANTID_TESTHELPERS_PARALLELRUNNER_H_

#include "MantidParallel/Communicator.h"
#include "MantidParallel/DllConfig.h"

#include <boost/make_shared.hpp>

#include <functional>
#include <thread>

namespace ParallelTestHelpers {

/** Runs a callable in parallel. This is mainly a helper for testing code with
  MPI calls. ParallelRunner passes a Communicator as first argument to the
  callable. In runs with only a single MPI rank the callable is executed in
  threads to mimic MPI ranks.
*/
class ParallelRunner {
public:
  ParallelRunner();
  ParallelRunner(const int threads);

  int size() const;

  template <class Function, class... Args>
  void run(Function &&f, Args &&... args);

private:
  boost::shared_ptr<Mantid::Parallel::detail::ThreadingBackend> m_backend;
  boost::shared_ptr<Mantid::Parallel::detail::ThreadingBackend> m_serialBackend;
};

template <class Function, class... Args>
void ParallelRunner::run(Function &&f, Args &&... args) {
  // 1. Run serial
  f(Mantid::Parallel::Communicator(m_serialBackend, 0),
    std::forward<Args>(args)...);
  // 2. Run parallel
  if (!m_backend) {
    Mantid::Parallel::Communicator comm;
    f(comm, std::forward<Args>(args)...);
  } else {
    std::vector<std::thread> threads;
    for (int t = 0; t < m_backend->size(); ++t) {
      Mantid::Parallel::Communicator comm(m_backend, t);
      threads.emplace_back(std::forward<Function>(f), comm,
                           std::forward<Args>(args)...);
    }
    for (auto &t : threads) {
      t.join();
    }
  }
}

template <class... Args> void runParallel(Args &&... args) {
  ParallelRunner runner;
  runner.run(std::forward<Args>(args)...);
}

} // namespace ParallelTestHelpers

#endif /* MANTID_TESTHELPERS_PARALLELRUNNER_H_ */
