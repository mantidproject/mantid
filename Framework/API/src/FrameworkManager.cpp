// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/UsageService.h"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <Poco/ActiveResult.h>

#include <clocale>
#include <cstdarg>
#include <memory>

#include "tbb/global_control.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

#ifdef __linux__
#include <csignal>
#include <execinfo.h>
#endif

namespace {
// We use a raw pointer over a unique_ptr to avoid problems with static deallocation order of static
// variables. In certain circumstances, e.g. unit testing, the shutdown method on FrameworkManager is
// not called. In this situation unique_ptr causes the tbb::global_control object to be destroyed
// *after* its own internal static data has been destroyed and we see a random segfault.
// We have technically introduced a leak by not deleting this object in the situation where
// `FrameworkManager::shutdown` is not called but the memory cannot be used again by the program since
// it survives for the duration of the program. It will ultimately be reclaimed by the OS.
tbb::global_control *GLOBAL_TBB_CONTROL = nullptr;
} // namespace

namespace Mantid {
using Kernel::ConfigService;
using Kernel::LibraryManager;
using Kernel::LibraryManagerImpl;
using Kernel::UsageService;
namespace API {
namespace {
/// static logger
Kernel::Logger g_log("FrameworkManager");
/// Key to define the location of the framework plugins
const char *PLUGINS_DIR_KEY = "framework.plugins.directory";
/// Key to define the location of the plugins to exclude from loading
const char *PLUGINS_EXCLUDE_KEY = "framework.plugins.exclude";
} // namespace

/** This is a function called every time NeXuS raises an error.
 * This swallows the errors and outputs nothing.
 */
// Prevent clang-tidy trying to change the signature for ext. interface
// NOLINTNEXTLINE(readability-non-const-parameter)
void NexusErrorFunction(void *, const char *) {
  // Do nothing.
}

#ifdef __linux__
/**
 * Print current backtrace to the given stream
 * @param os A reference to an output stream
 */
void backtraceToStream(std::ostream &os) {
  void *trace_elems[32];
  int trace_elem_count(backtrace(trace_elems, 32));
  char **stack_syms(backtrace_symbols(trace_elems, trace_elem_count));
  for (int i = 0; i < trace_elem_count; ++i) {
    os << ' ' << stack_syms[i] << '\n';
  }
  free(stack_syms);
}

/**
 * Designed as a handler function for std::set_terminate. It prints
 * a header message and then a backtrace to std::cerr. It calls exit
 * with code 1
 */
void terminateHandler() {
  std::cerr << "\n********* UNHANDLED EXCEPTION *********\n";
  try {
    std::rethrow_exception(std::current_exception());
  } catch (const std::exception &exc) {
    std::cerr << "  what(): " << exc.what() << "\n\n";
  } catch (...) {
    std::cerr << "  what(): Unknown exception type. No more information "
                 "available\n\n";
  }
  std::cerr << "Backtrace:\n";
  backtraceToStream(std::cerr);
  exit(1);
}

#endif

/// Default constructor
FrameworkManagerImpl::FrameworkManagerImpl() {
#ifdef __linux__
  std::set_terminate(terminateHandler);
#endif
  setGlobalNumericLocaleToC();
  Kernel::MemoryOptions::initAllocatorOptions();

#ifdef _WIN32
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
  // This causes the exponent to consist of two digits.
  // VC++ >=1900 use standards conforming behaviour and only
  // uses the number of digits required
  _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

  ConfigService::Instance();
  g_log.notice() << Mantid::welcomeMessage() << '\n';
  loadPlugins();
  disableNexusOutput();
  setNumOMPThreadsToConfigValue();

  g_log.debug() << "FrameworkManager created.\n";

  asynchronousStartupTasks();
}

/**
 * Load all plugins from the framework
 */
void FrameworkManagerImpl::loadPlugins() { loadPluginsUsingKey(PLUGINS_DIR_KEY, PLUGINS_EXCLUDE_KEY); }

/**
 * Set the number of OpenMP cores to use based on the config value
 */
void FrameworkManagerImpl::setNumOMPThreadsToConfigValue() {
  // Set the number of threads to use for this process
  auto maxCores = Kernel::ConfigService::Instance().getValue<int>("MultiThreaded.MaxCores");
  if (maxCores.value_or(0) > 0) {
    setNumOMPThreads(maxCores.value());
  }
}

/**
 * Set the number of OpenMP cores to use based on the config value
 * @param nthreads :: The maximum number of threads to use
 */
void FrameworkManagerImpl::setNumOMPThreads(const int nthreads) {
  g_log.debug() << "Setting maximum number of threads to " << nthreads << "\n";
  PARALLEL_SET_NUM_THREADS(nthreads);
  if (GLOBAL_TBB_CONTROL) {
    delete GLOBAL_TBB_CONTROL; // Have to reset to change the number of threads at runtime
  }

  GLOBAL_TBB_CONTROL = new tbb::global_control(tbb::global_control::max_allowed_parallelism, nthreads);
}

/**
 * Returns the number of OpenMP threads that will be used
 * @returns The number of OpenMP threads that will be used in the next
 * parallel
 * call
 */
int FrameworkManagerImpl::getNumOMPThreads() const { return PARALLEL_GET_MAX_THREADS; }

/** Clears all memory associated with the AlgorithmManager
 *  and with the Analysis & Instrument data services.
 */
void FrameworkManagerImpl::clear() {
  clearAlgorithms();
  clearInstruments();
  clearData();
  clearPropertyManagers();
}

void FrameworkManagerImpl::shutdown() {
  Kernel::UsageService::Instance().shutdown();
  // Ensure we don't run into static init ordering issues with TBB
  delete GLOBAL_TBB_CONTROL;
  clear();
}

/**
 * Clear memory associated with the AlgorithmManager
 */
void FrameworkManagerImpl::clearAlgorithms() { AlgorithmManager::Instance().clear(); }

/**
 * Clear memory associated with the ADS
 */
void FrameworkManagerImpl::clearData() { AnalysisDataService::Instance().clear(); }

/**
 * Clear memory associated with the IDS
 */
void FrameworkManagerImpl::clearInstruments() { InstrumentDataService::Instance().clear(); }

/**
 * Clear memory associated with the PropertyManagers
 */
void FrameworkManagerImpl::clearPropertyManagers() {
  using Kernel::PropertyManagerDataService;
  PropertyManagerDataService::Instance().clear();
}

/** Run any algorithm with a variable number of parameters
 *
 * @param algorithmName
 * @param count :: number of arguments given.
 * @return the algorithm created
 */
IAlgorithm_sptr FrameworkManagerImpl::exec(const std::string &algorithmName, int count, ...) {
  if (count % 2 == 1) {
    throw std::runtime_error("Must have an even number of parameter/value string arguments");
  }

  // Create the algorithm
  auto alg = AlgorithmManager::Instance().createUnmanaged(algorithmName, -1);
  alg->initialize();
  if (!alg->isInitialized())
    throw std::runtime_error(algorithmName + " was not initialized.");

  va_list Params;
  va_start(Params, count);
  for (int i = 0; i < count; i += 2) {
    std::string paramName = va_arg(Params, const char *);
    std::string paramValue = va_arg(Params, const char *);
    alg->setPropertyValue(paramName, paramValue);
  }
  va_end(Params);

  alg->execute();
  return alg;
}

/** Returns a shared pointer to the workspace requested
 *
 *  @param wsName :: The name of the workspace
 *  @return A pointer to the workspace
 *
 *  @throw NotFoundError If workspace is not registered with analysis data
 *service
 */
Workspace *FrameworkManagerImpl::getWorkspace(const std::string &wsName) {
  Workspace *space;
  try {
    space = AnalysisDataService::Instance().retrieve(wsName).get();
  } catch (Kernel::Exception::NotFoundError &) {
    throw Kernel::Exception::NotFoundError("Unable to retrieve workspace", wsName);
  }
  return space;
}

/** Removes and deletes a workspace from the data service store.
 *
 *  @param wsName :: The user-given name for the workspace
 *  @return true if the workspace was found and deleted
 *
 *  @throw NotFoundError Thrown if workspace cannot be found
 */
bool FrameworkManagerImpl::deleteWorkspace(const std::string &wsName) {
  bool retVal = false;
  std::shared_ptr<Workspace> ws_sptr;
  try {
    ws_sptr = AnalysisDataService::Instance().retrieve(wsName);
  } catch (Kernel::Exception::NotFoundError &ex) {
    g_log.error() << ex.what() << '\n';
    return false;
  }

  std::shared_ptr<WorkspaceGroup> ws_grpsptr = std::dynamic_pointer_cast<WorkspaceGroup>(ws_sptr);
  if (ws_grpsptr) {
    //  selected workspace is a group workspace
    AnalysisDataService::Instance().deepRemoveGroup(wsName);
  }
  // Make sure we drop the references so the memory will get freed when we
  // expect it to
  ws_sptr.reset();
  ws_grpsptr.reset();
  try {
    AnalysisDataService::Instance().remove(wsName);
    retVal = true;
  } catch (Kernel::Exception::NotFoundError &) {
    // workspace was not found
    g_log.error() << "Workspace " << wsName << " could not be found.\n";
    retVal = false;
  }
  return retVal;
}

/**
 * Load a set of plugins from the path pointed to by the given config key
 * @param locationKey A string containing a key to lookup in the
 * ConfigService
 * @param excludeKey A string
 */
void FrameworkManagerImpl::loadPluginsUsingKey(const std::string &locationKey, const std::string &excludeKey) {
  const auto &cfgSvc = Kernel::ConfigService::Instance();
  const auto pluginDir = cfgSvc.getString(locationKey);
  if (pluginDir.length() > 0) {
    std::vector<std::string> excludes;
    const auto excludeStr = cfgSvc.getString(excludeKey);
    boost::split(excludes, excludeStr, boost::is_any_of(";"));
    g_log.debug("Loading libraries from '" + pluginDir + "', excluding '" + excludeStr + "'");
    LibraryManager::Instance().openLibraries(pluginDir, LibraryManagerImpl::NonRecursive, excludes);
  } else {
    g_log.debug("No library directory found in key \"" + locationKey + "\"");
  }
}

/**
 * Set the numeric formatting category of the C locale to classic C.
 */
void FrameworkManagerImpl::setGlobalNumericLocaleToC() {
  // Some languages, for example German, using different decimal separators.
  // By default C/C++ operations attempting to extract numbers from a stream
  // will use the system locale. For those locales where numbers are formatted
  // differently we see issues, particularly with opencascade, where Mantid
  // will hang or throw an exception while trying to parse text.
  //
  // The following tells all numerical extraction operations to use classic
  // C as the locale.
  setlocale(LC_NUMERIC, "C");
}

/// Silence NeXus output
void FrameworkManagerImpl::disableNexusOutput() { ::NeXus::setError(nullptr, NexusErrorFunction); }

/// Starts asynchronous tasks that are done as part of Start-up.
void FrameworkManagerImpl::asynchronousStartupTasks() {
  auto instrumentUpdates = Kernel::ConfigService::Instance().getValue<bool>("UpdateInstrumentDefinitions.OnStartup");

  if (instrumentUpdates.value_or(false)) {
    updateInstrumentDefinitions();
  } else {
    g_log.information() << "Instrument updates disabled - cannot update "
                           "instrument definitions.\n";
  }

  auto newVersionCheck = Kernel::ConfigService::Instance().getValue<bool>("CheckMantidVersion.OnStartup");
  if (newVersionCheck.value_or(false)) {
    checkIfNewerVersionIsAvailable();
  } else {
    g_log.information() << "Version check disabled.\n";
  }

  setupUsageReporting();
}

void FrameworkManagerImpl::setupUsageReporting() {
  auto &configSvc = ConfigService::Instance();
  auto interval = configSvc.getValue<int>("Usage.BufferCheckInterval");
  auto &usageSvc = UsageService::Instance();
  if (interval.value_or(0) > 0) {
    usageSvc.setInterval(interval.value());
  }
  auto enabled = configSvc.getValue<bool>("usagereports.enabled");
  usageSvc.setEnabled(enabled.value_or(false));
  usageSvc.registerStartup();
}

/// Update instrument definitions from github
void FrameworkManagerImpl::updateInstrumentDefinitions() {
  try {
    auto algDownloadInstrument = Mantid::API::AlgorithmManager::Instance().create("DownloadInstrument");
    algDownloadInstrument->setAlgStartupLogging(false);
    algDownloadInstrument->executeAsync();
  } catch (Kernel::Exception::NotFoundError &) {
    g_log.debug() << "DowndloadInstrument algorithm is not available - cannot "
                     "update instrument definitions.\n";
  }
}

/// Check if a newer release of Mantid is available
void FrameworkManagerImpl::checkIfNewerVersionIsAvailable() {
  try {
    auto algCheckVersion = Mantid::API::AlgorithmManager::Instance().create("CheckMantidVersion");
    algCheckVersion->setAlgStartupLogging(false);
    algCheckVersion->executeAsync();
  } catch (Kernel::Exception::NotFoundError &) {
    g_log.debug() << "CheckMantidVersion algorithm is not available - cannot "
                     "check if a newer version is available.\n";
  }
}

} // namespace API
} // Namespace Mantid
