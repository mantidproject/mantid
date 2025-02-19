// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidKernel/SingletonHolder.h"
#include <memory>

namespace Mantid {

namespace API {
class IAlgorithm;
class Workspace;

/** The main public API via which users interact with the Mantid framework.

    @author Russell Taylor, Tessella Support Services plc
    @date 05/10/2007
 */
class MANTID_API_DLL FrameworkManagerImpl {
public:
  FrameworkManagerImpl(const FrameworkManagerImpl &) = delete;
  FrameworkManagerImpl &operator=(const FrameworkManagerImpl &) = delete;

  /// Load framework plugins
  void loadPlugins();

  /// Set the number of OpenMP threads to use based on the config value
  void setNumOMPThreadsToConfigValue();
  /// Set the number of OpenMP threads to the given value
  void setNumOMPThreads(const int nthreads);
  /// Returns the number of OpenMP threads that will be used
  int getNumOMPThreads() const;

  /// Clears all memory associated with the AlgorithmManager, ADS & IDS
  void clear();
  /// shuts down and performs clean up tasks
  void shutdown();
  /// Clear memory associated with the AlgorithmManager
  void clearAlgorithms();

  /// Clear memory associated with the ADS
  void clearData();

  /// Clear memory associated with the IDS
  void clearInstruments();

  /// Clear memory associated with the PropertyManagers
  void clearPropertyManagers();

  /// Creates an algorithm and runs it, with variadic arguments
  std::shared_ptr<IAlgorithm> exec(const std::string &algorithmName, int count, ...);

  /// Returns a shared pointer to the workspace requested
  Workspace *getWorkspace(const std::string &wsName);

  /// Deletes a workspace from the framework
  bool deleteWorkspace(const std::string &wsName);

private:
  friend struct Mantid::Kernel::CreateUsingNew<FrameworkManagerImpl>;

  /// Private Constructor
  FrameworkManagerImpl();
  /// Private Destructor
  ~FrameworkManagerImpl() = default;

  /// Load a set of plugins using a key from the ConfigService
  void loadPluginsUsingKey(const std::string &locationKey, const std::string &excludeKey);
  /// Set up the global locale
  void setGlobalNumericLocaleToC();
  /// Starts asynchronous tasks that are done as part of Start-up
  void asynchronousStartupTasks();
  /// Setup Usage Reporting if enabled
  void setupUsageReporting();
  /// Update instrument definitions from github
  void updateInstrumentDefinitions();
  /// check if a newer version of Mantid is available
  void checkIfNewerVersionIsAvailable();
};

using FrameworkManager = Mantid::Kernel::SingletonHolder<FrameworkManagerImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::FrameworkManagerImpl>;
}
} // namespace Mantid
