#ifndef MANTID_API_FRAMEWORKMANAGER_H_
#define MANTID_API_FRAMEWORKMANAGER_H_

#include <string>

#ifdef MPI_BUILD
#include <boost/mpi/environment.hpp>
#endif

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidKernel/SingletonHolder.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {

namespace API {
class IAlgorithm;
class Workspace;

/** The main public API via which users interact with the Mantid framework.

    @author Russell Taylor, Tessella Support Services plc
    @date 05/10/2007

    Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
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
  /// Creates and instance of an algorithm
  IAlgorithm *createAlgorithm(const std::string &algName,
                              const int &version = -1);

  /// Creates an instance of an algorithm and sets the properties provided
  IAlgorithm *createAlgorithm(const std::string &algName,
                              const std::string &propertiesArray,
                              const int &version = -1);

  /// Creates an instance of an algorithm, sets the properties provided & then
  /// executes it.
  IAlgorithm *exec(const std::string &algName,
                   const std::string &propertiesArray, const int &version = -1);

  /// Creates an algorithm and runs it, with variadic arguments
  boost::shared_ptr<IAlgorithm> exec(const std::string &algorithmName,
                                     int count, ...);

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
  void loadPluginsUsingKey(const std::string &locationKey,
                           const std::string &excludeKey);
  /// Set up the global locale
  void setGlobalNumericLocaleToC();
  /// Silence NeXus output
  void disableNexusOutput();
  /// Starts asynchronous tasks that are done as part of Start-up
  void asynchronousStartupTasks();
  /// Setup Usage Reporting if enabled
  void setupUsageReporting();
  /// Update instrument definitions from github
  void updateInstrumentDefinitions();
  /// check if a newer version of Mantid is available
  void checkIfNewerVersionIsAvailable();

#ifdef MPI_BUILD
  /** Member variable that initialises the MPI environment on construction (in
   * the
   *  FrameworkManager constructor) and finalises it on destruction.
   *  The class has no non-static member functions, so is not exposed in the
   * class interface.
   */
  boost::mpi::environment m_mpi_environment;
  int argc = 0;
  char **argv;
#endif
};

using FrameworkManager = Mantid::Kernel::SingletonHolder<FrameworkManagerImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::FrameworkManagerImpl>;
}
} // namespace Mantid

#endif /*MANTID_API_FRAMEWORKMANAGER_H_*/
