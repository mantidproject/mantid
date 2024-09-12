// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <atomic>
#include <mutex>

namespace Mantid {
namespace Kernel {

/** Thread-safety check
 * Checks the workspace to ensure it is suitable for multithreaded access.
 * NULL workspaces are assumed suitable
 * @param workspace pointer to workspace to verify.
 * @return Whether workspace is threadsafe.
 */
template <typename Arg>
inline typename std::enable_if<std::is_pointer<Arg>::value, bool>::type threadSafe(Arg workspace) {
  return !workspace || workspace->threadSafe();
}

/** Thread-safety check
 * Checks the workspace to ensure it is suitable for multithreaded access.
 * NULL workspaces are assumed suitable
 * @param workspace pointer to workspace to verify.
 * @param others pointers to all other workspaces which need to be checked.
 * @return whether workspace is threadsafe.
 */
template <typename Arg, typename... Args>
inline typename std::enable_if<std::is_pointer<Arg>::value, bool>::type threadSafe(Arg workspace, Args &&...others) {
  return (!workspace || workspace->threadSafe()) && threadSafe(std::forward<Args>(others)...);
}

/** Thread-safety check
 * Checks the workspace to ensure it is suitable for multithreaded access.
 * @param workspace reference to workspace to verify.
 * @return Whether workspace is threadsafe.
 */
template <typename Arg>
inline typename std::enable_if<!std::is_pointer<Arg>::value, bool>::type threadSafe(const Arg &workspace) {
  return workspace.threadSafe();
}

/** Thread-safety check
 * Checks the workspace to ensure it is suitable for multithreaded access.
 * @param workspace reference to workspace to verify.
 * @param others references or pointers to all other workspaces which need to be
 * checked.
 * @return whether workspace is threadsafe.
 */
template <typename Arg, typename... Args>
inline typename std::enable_if<!std::is_pointer<Arg>::value, bool>::type threadSafe(const Arg &workspace,
                                                                                    Args &&...others) {
  return workspace.threadSafe() && threadSafe(std::forward<Args>(others)...);
}

/** Uses std::compare_exchange_weak to update the atomic value f = op(f, d)
 * Used to improve parallel scaling in algorithms MDNormDirectSC and MDNormSCD
 * @param f atomic variable being updated
 * @param d second element in binary operation
 * @param op binary operation on elements f and d
 */
template <typename T, typename BinaryOp> void AtomicOp(std::atomic<T> &f, T d, BinaryOp op) {
  T old = f.load();
  T desired;
  do {
    desired = op(old, d);
  } while (!f.compare_exchange_weak(old, desired));
}

} // namespace Kernel
} // namespace Mantid

// The syntax used to define a pragma within a macro is different on windows and
// GCC
#ifdef _MSC_VER
#define PRAGMA __pragma
#define PARALLEL_SET_CONFIG_THREADS
#else //_MSC_VER
#define PRAGMA(x) _Pragma(#x)
#define PARALLEL_SET_CONFIG_THREADS                                                                                    \
  setMaxCoresToConfig();                                                                                               \
  PARALLEL_SET_DYNAMIC(false);
#endif //_MSC_VER

/** Begins a block to skip processing is the algorithm has been interupted
 * Note the end of the block if not defined that must be added by including
 * PARALLEL_END_INTERRUPT_REGION at the end of the loop
 */
#define PARALLEL_START_INTERRUPT_REGION                                                                                \
  if (!m_parallelException && !m_cancel) {                                                                             \
    try {

/** Ends a block to skip processing is the algorithm has been interupted
 * Note the start of the block if not defined that must be added by including
 * PARALLEL_START_INTERRUPT_REGION at the start of the loop
 */
#define PARALLEL_END_INTERRUPT_REGION                                                                                  \
  } /* End of try block in PARALLEL_START_INTERRUPT_REGION */                                                          \
  catch (std::exception & ex) {                                                                                        \
    if (!m_parallelException) {                                                                                        \
      m_parallelException = true;                                                                                      \
      g_log.error() << this->name() << ": " << ex.what() << "\n";                                                      \
    }                                                                                                                  \
  }                                                                                                                    \
  catch (...) {                                                                                                        \
    m_parallelException = true;                                                                                        \
  }                                                                                                                    \
  } // End of if block in PARALLEL_START_INTERRUPT_REGION

/** Adds a check after a Parallel region to see if it was interupted
 */
#define PARALLEL_CHECK_INTERRUPT_REGION                                                                                \
  if (m_parallelException) {                                                                                           \
    g_log.debug("Exception thrown in parallel region");                                                                \
    throw std::runtime_error(this->name() + ": error (see log)");                                                      \
  }                                                                                                                    \
  interruption_point();

// _OPENMP is automatically defined if openMP support is enabled in the
// compiler.
#ifdef _OPENMP

#include "MantidKernel/ConfigService.h"
#include <omp.h>

/** Includes code to add OpenMP commands to run the next for loop in parallel.
 *   This includes an arbirary check: condition.
 *   "condition" must evaluate to TRUE in order for the
 *   code to be executed in parallel
 */
#define PARALLEL_FOR_IF(condition)                                                                                     \
  PARALLEL_SET_CONFIG_THREADS                                                                                          \
  PRAGMA(omp parallel for if (condition) )

/** Includes code to add OpenMP commands to run the next for loop in parallel.
 *   This includes no checks to see if workspaces are suitable
 *   and therefore should not be used in any loops that access workspaces.
 */
#define PARALLEL_FOR_NO_WSP_CHECK()                                                                                    \
  PARALLEL_SET_CONFIG_THREADS                                                                                          \
  PRAGMA(omp parallel for)

/** Includes code to add OpenMP commands to run the next for loop in parallel.
 *  and declare the variables to be firstprivate.
 *  This includes no checks to see if workspaces are suitable
 *  and therefore should not be used in any loops that access workspace.
 */
#define PARALLEL_FOR_NOWS_CHECK_FIRSTPRIVATE(variable)                                                                 \
  PARALLEL_SET_CONFIG_THREADS                                                                                          \
  PRAGMA(omp parallel for firstprivate(variable) )

#define PARALLEL_FOR_NO_WSP_CHECK_FIRSTPRIVATE2(variable1, variable2)                                                  \
  PARALLEL_SET_CONFIG_THREADS                                                                                          \
  PRAGMA(omp parallel for firstprivate(variable1, variable2) )

/** Ensures that the next execution line or block is only executed if
 * there are multple threads execting in this region
 */
#define IF_PARALLEL if (omp_get_num_threads() > 1)

/** Ensures that the next execution line or block is only executed if
 * there is only one thread in operation
 */
#define IF_NOT_PARALLEL if (omp_get_num_threads() == 1)

/** Specifies that the next code line or block will only allow one thread
 * through at a time
 */
#define PARALLEL_CRITICAL(name) PRAGMA(omp critical(name))

/** Allows only one thread at a time to write to a specific memory location
 */
#define PARALLEL_ATOMIC PRAGMA(omp atomic)

#define PARALLEL_SET_NUM_THREADS(MaxCores) omp_set_num_threads(MaxCores);

/** A value that indicates if the number of threads available in subsequent
 * parallel region
 *  can be adjusted by the runtime. If nonzero, the runtime can adjust the
 * number of threads,
 *  if zero, the runtime will not dynamically adjust the number of threads.
 */
#define PARALLEL_SET_DYNAMIC(val) omp_set_dynamic(val)

#define PARALLEL_NUMBER_OF_THREADS omp_get_num_threads()

#define PARALLEL_GET_MAX_THREADS omp_get_max_threads()

#define PARALLEL_THREAD_NUMBER omp_get_thread_num()

#define PARALLEL PRAGMA(omp parallel)

#define PARALLEL_SECTIONS PRAGMA(omp sections nowait)

#define PARALLEL_SECTION PRAGMA(omp section)

inline void setMaxCoresToConfig() {
  const auto maxCores = Mantid::Kernel::ConfigService::Instance().getValue<int>("MultiThreaded.MaxCores");
  if (maxCores.value_or(0) > 0) {
    PARALLEL_SET_NUM_THREADS(maxCores.value());
  }
}

/** General purpose define for OpenMP, becomes the equivalent of
 * #pragma omp EXPRESSION
 * (if your compiler supports OpenMP)
 */
#define PRAGMA_OMP(expression) PRAGMA(omp expression)

#else //_OPENMP

/// Empty definitions - to enable set your complier to enable openMP
#define PARALLEL_FOR_IF(condition)
#define PARALLEL_FOR_NO_WSP_CHECK()
#define PARALLEL_FOR_NOWS_CHECK_FIRSTPRIVATE(variable)
#define PARALLEL_FOR_NO_WSP_CHECK_FIRSTPRIVATE2(variable1, variable2)
#define IF_PARALLEL if (false)
#define IF_NOT_PARALLEL
#define PARALLEL_CRITICAL(name)
#define PARALLEL_ATOMIC
#define PARALLEL_THREAD_NUMBER 0
#define PARALLEL_SET_NUM_THREADS(MaxCores)
#define PARALLEL_SET_DYNAMIC(val)
#define PARALLEL_NUMBER_OF_THREADS 1
#define PARALLEL_GET_MAX_THREADS 1
#define PARALLEL
#define PARALLEL_SECTIONS
#define PARALLEL_SECTION
#define PRAGMA_OMP(expression)
#endif //_OPENMP
