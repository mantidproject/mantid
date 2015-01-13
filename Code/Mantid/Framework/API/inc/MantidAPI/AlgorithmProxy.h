#ifndef MANTID_API_ALGORITHMPROXY_H_
#define MANTID_API_ALGORITHMPROXY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/PropertyManagerOwner.h"

#ifdef _MSC_VER
#pragma warning(disable : 4250) // Disable warning regarding inheritance via
                                // dominance, we have no way around it with the
                                // design
#endif

//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
namespace Poco {
template <class R, class A, class O, class S> class ActiveMethod;
template <class O> class ActiveStarter;
class Void;
}

namespace Mantid {
namespace API {
class Algorithm;
typedef boost::shared_ptr<Algorithm> Algorithm_sptr;

/**

A proxy class that stands between the user and the actual algorithm.
AlgorithmDataService stores algoruithm proxies. Actual algorithms are
created by the proxy and destroyed after execution to free memory.
Algorithm and its proxy share all properties.

@author Russell Taylor, Tessella Support Services plc
@author Based on the Gaudi class of the same name (see
http://proj-gaudi.web.cern.ch/proj-gaudi/)
@date 12/09/2007
@author Roman Tolchenov, Tessella plc
@date 03/03/2009

Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class MANTID_API_DLL AlgorithmProxy : public IAlgorithm,
                                      public Kernel::PropertyManagerOwner {
public:
  AlgorithmProxy(Algorithm_sptr alg);
  virtual ~AlgorithmProxy();

  /// The name of the algorithm
  const std::string name() const { return m_name; }
  /// The version of the algorithm
  int version() const { return m_version; }
  /// The category of the algorithm
  const std::string category() const { return m_category; }
  /// Function to return all of the categories that contain this algorithm
  const std::vector<std::string> categories() const;
  /// Function to return the sperator token for the category string. A default
  /// implementation ',' is provided
  const std::string categorySeparator() const { return m_categorySeparator; }
  /// Aliases to the algorithm
  const std::string alias() const { return m_alias; }
  /// function returns a summary message that will be displayed in the default
  /// GUI, and in the help.
  const std::string summary() const { return m_summary; }

  /// The algorithmID
  AlgorithmID getAlgorithmID() const;

  void initialize();
  std::map<std::string, std::string> validateInputs();
  bool execute();
  void executeAsChildAlg() { throw std::runtime_error("Not implemented."); }
  Poco::ActiveResult<bool> executeAsync();
  bool isInitialized() const;
  bool isExecuted() const;

  /// To query whether algorithm is a child. A proxy is always at top level,
  /// returns false
  bool isChild() const { return m_isChild; }
  void setAlwaysStoreInADS(const bool) {}
  void setChild(const bool val) { m_isChild = val; }
  /// Proxies only manage parent algorithms
  void enableHistoryRecordingForChild(const bool){};
  void setRethrows(const bool rethrow);

  const std::string workspaceMethodName() const;
  const std::vector<std::string> workspaceMethodOn() const;
  const std::string workspaceMethodInputProperty() const;

  /** @name PropertyManager methods */
  //@{
  /// Set the property value
  void setPropertyValue(const std::string &name, const std::string &value);
  /// Do something after a property was set
  void afterPropertySet(const std::string &);
  //@}

  void cancel();
  bool isRunning() const;

  void addObserver(const Poco::AbstractObserver &observer) const;
  void removeObserver(const Poco::AbstractObserver &observer) const;

  /// Set logging on or off
  ///@param value :: true = logging enabled
  void setLogging(const bool value) { m_isLoggingEnabled = value; }
  /// Is the algorithm have logging enabled
  bool isLogging() const { return m_isLoggingEnabled; }

  /// returns the logging priority offset
  void setLoggingOffset(const int value) { m_loggingOffset = value; }
  /// returns the logging priority offset
  int getLoggingOffset() const { return m_loggingOffset; }
  /// disable Logging of start and end messages
  void setAlgStartupLogging(const bool enabled);
  /// get the state of Logging of start and end messages
  bool getAlgStartupLogging() const;

  /// setting the child start progress
  void setChildStartProgress(const double startProgress) const;
  /// setting the child end progress
  void setChildEndProgress(const double endProgress) const;

  /** @name String serialization */
  //@{
  /// Serialize an object to a string
  virtual std::string toString() const;
  //@}

private:
  /// Private Copy constructor: NO COPY ALLOWED
  AlgorithmProxy(const AlgorithmProxy &);
  /// Private assignment operator: NO ASSIGNMENT ALLOWED
  AlgorithmProxy &operator=(const AlgorithmProxy &);

  void createConcreteAlg(bool initOnly = false);
  void stopped();
  void addObservers();
  void dropWorkspaceReferences();

  /// Poco::ActiveMethod used to implement asynchronous execution.
  Poco::ActiveMethod<bool, Poco::Void, AlgorithmProxy,
                     Poco::ActiveStarter<AlgorithmProxy>> *m_executeAsync;
  /// Execute asynchronous implementation
  bool executeAsyncImpl(const Poco::Void &dummy);

  const std::string m_name;     ///< name of the real algorithm
  const std::string m_category; ///< category of the real algorithm
  const std::string
      m_categorySeparator;     ///< category seperator of the real algorithm
  const std::string m_alias;   ///< alias to the algorithm
  const std::string m_summary; ///<Message to display in GUI and help.
  const int m_version;         ///< version of the real algorithm

  mutable boost::shared_ptr<Algorithm>
      m_alg;         ///< Shared pointer to a real algorithm. Created on demand
  bool m_isExecuted; ///< Executed flag
  bool m_isLoggingEnabled; ///< is the logging of the underlying algorithm
  /// enabled
  int m_loggingOffset;               ///< the logging priority offset
  bool m_isAlgStartupLoggingEnabled; /// Whether to log alg startup and
                                     /// closedown messages from the base class
                                     /// (default = true)
  bool m_rethrow;                    ///< Whether or not to rethrow exceptions.
  bool m_isChild;                    ///< Is this a child algo

  /// Temporary holder of external observers wishing to subscribe
  mutable std::vector<const Poco::AbstractObserver *> m_externalObservers;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHMPROXY_H_*/
