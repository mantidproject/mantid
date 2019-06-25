// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
} // namespace Poco

namespace Mantid {
namespace API {
class Algorithm;
using Algorithm_sptr = boost::shared_ptr<Algorithm>;

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
*/
class MANTID_API_DLL AlgorithmProxy : public IAlgorithm,
                                      public Kernel::PropertyManagerOwner {
public:
  AlgorithmProxy(Algorithm_sptr alg);
  AlgorithmProxy(const AlgorithmProxy &) = delete;
  AlgorithmProxy &operator=(const AlgorithmProxy &) = delete;
  ~AlgorithmProxy() override;

  /// The name of the algorithm
  const std::string name() const override { return m_name; }
  /// The version of the algorithm
  int version() const override { return m_version; }
  /// The category of the algorithm
  const std::string category() const override { return m_category; }
  /// Function to return all of the categories that contain this algorithm
  const std::vector<std::string> categories() const override;
  /// Function to return the seperator token for the category string. A default
  /// implementation ',' is provided
  const std::string categorySeparator() const override {
    return m_categorySeparator;
  }
  /// Function to return all of the seeAlso algorithms related to this algorithm
  const std::vector<std::string> seeAlso() const override { return m_seeAlso; };
  /// Aliases to the algorithm
  const std::string alias() const override { return m_alias; }
  /// Optional documentation URL for the real algorithm
  const std::string helpURL() const override { return m_helpURL; }
  /// function returns a summary message that will be displayed in the default
  /// GUI, and in the help.
  const std::string summary() const override { return m_summary; }

  /// The algorithmID
  AlgorithmID getAlgorithmID() const override;

  void initialize() override;
  std::map<std::string, std::string> validateInputs() override;
  bool execute() override;
  void executeAsChildAlg() override {
    throw std::runtime_error("Not implemented.");
  }
  Poco::ActiveResult<bool> executeAsync() override;
  bool isInitialized() const override;
  bool isExecuted() const override;

  /// To query whether algorithm is a child. A proxy is always at top level,
  /// returns false
  bool isChild() const override { return m_isChild; }
  void setChild(const bool val) override {
    m_isChild = val;
    setAlwaysStoreInADS(!val);
  }
  void setAlwaysStoreInADS(const bool val) override {
    m_setAlwaysStoreInADS = val;
  }
  bool getAlwaysStoreInADS() const override { return m_setAlwaysStoreInADS; }

  /// Proxies only manage parent algorithms
  void enableHistoryRecordingForChild(const bool) override{};
  void setRethrows(const bool rethrow) override;

  const std::string workspaceMethodName() const override;
  const std::vector<std::string> workspaceMethodOn() const override;
  const std::string workspaceMethodInputProperty() const override;

  /** @name PropertyManager methods */
  //@{
  /// Set the property value
  void setPropertyValue(const std::string &name,
                        const std::string &value) override;
  /// Do something after a property was set
  void afterPropertySet(const std::string &) override;
  /// Make m_properties point to the same PropertyManager as po.
  void copyPropertiesFrom(const PropertyManagerOwner &po) override;
  //@}

  void cancel() override;
  bool isRunning() const override;

  void addObserver(const Poco::AbstractObserver &observer) const override;
  void removeObserver(const Poco::AbstractObserver &observer) const override;

  /// Set logging on or off
  ///@param value :: true = logging enabled
  void setLogging(const bool value) override { m_isLoggingEnabled = value; }
  /// Is the algorithm have logging enabled
  bool isLogging() const override { return m_isLoggingEnabled; }

  /// returns the logging priority offset
  void setLoggingOffset(const int value) override { m_loggingOffset = value; }
  /// returns the logging priority offset
  int getLoggingOffset() const override { return m_loggingOffset; }
  /// disable Logging of start and end messages
  void setAlgStartupLogging(const bool enabled) override;
  /// get the state of Logging of start and end messages
  bool getAlgStartupLogging() const override;

  /// setting the child start progress
  void setChildStartProgress(const double startProgress) const override;
  /// setting the child end progress
  void setChildEndProgress(const double endProgress) const override;

  /** @name Serialization */
  //@{
  /// Serialize an object to a string
  std::string toString() const override;
  /// Serialize as a json value
  Json::Value toJson() const override;
  //@}

private:
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
      m_categorySeparator; ///< category seperator of the real algorithm
  const std::vector<std::string> m_seeAlso; ///< seeAlso of the real algorithm
  const std::string m_alias;                ///< alias to the algorithm
  const std::string m_helpURL;              ///< Optional documentation URL
  const std::string m_summary; ///< Message to display in GUI and help.
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
  bool m_setAlwaysStoreInADS;        ///< If this will save in ADS

  /// Temporary holder of external observers wishing to subscribe
  mutable std::vector<const Poco::AbstractObserver *> m_externalObservers;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHMPROXY_H_*/
