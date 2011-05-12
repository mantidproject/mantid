#ifndef MANTID_API_ALGORITHM_H_
#define MANTID_API_ALGORITHM_H_

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */
#define DECLARE_ALGORITHM(classname) \
    namespace { \
  Mantid::Kernel::RegistrationHelper register_alg_##classname( \
      ((Mantid::API::AlgorithmFactory::Instance().subscribe<classname>()) \
          , 0)); \
}


//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/PropertyManagerOwner.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <boost/shared_ptr.hpp>
#include <Poco/ActiveMethod.h>
#include <Poco/NotificationCenter.h>
#include <Poco/Notification.h>
#include <Poco/NObserver.h>
#include <Poco/Void.h>
#include <string>
#include <vector>
#include <map>
#include <cmath>

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

namespace Mantid
{
namespace API
{
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class AlgorithmProxy;

/**
 Base class from which all concrete algorithm classes should be derived.
 In order for a concrete algorithm class to do anything
 useful the methods init() & exec()  should be overridden.

 Further text from Gaudi file.......
 The base class provides utility methods for accessing
 standard services (event data service etc.); for declaring
 properties which may be configured by the job options
 service; and for creating sub algorithms.
 The only base class functionality which may be used in the
 constructor of a concrete algorithm is the declaration of
 member variables as properties. All other functionality,
 i.e. the use of services and the creation of sub-algorithms,
 may be used only in initialise() and afterwards (see the
 Gaudi user guide).

 @author Russell Taylor, Tessella Support Services plc
 @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
 @date 12/09/2007

 Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport Algorithm : public IAlgorithm, public Kernel::PropertyManagerOwner
{
public:

  /// Base class for algorithm notifications
  class AlgorithmNotification: public Poco::Notification
  {
  public:
    AlgorithmNotification(const Algorithm* const alg):Poco::Notification(),m_algorithm(alg){}///< Constructor
    const IAlgorithm* algorithm() const {return m_algorithm;}                       ///< The algorithm
  private:
    const IAlgorithm* const m_algorithm;///< The algorithm
  };

  /// StartedNotification is sent when the algorithm begins execution.
  class StartedNotification: public AlgorithmNotification
  {
  public:
    StartedNotification(const Algorithm* const alg):AlgorithmNotification(alg){}///< Constructor
    virtual std::string name() const{return "StartedNotification";}///< class name
  };

  /// FinishedNotification is sent after the algorithm finishes its execution
  class FinishedNotification: public AlgorithmNotification
  {
  public:
    FinishedNotification(const Algorithm* const alg, bool res):AlgorithmNotification(alg),success(res){}///< Constructor
    virtual std::string name() const{return "FinishedNotification";}///< class name
    bool success;///< true if the finished algorithm was successful or false if it failed.
  };

  /// An algorithm can report its progress by sending ProgressNotification. Use
  /// Algorithm::progress(double) function to send a preogress notification.
  class ProgressNotification: public AlgorithmNotification
  {
  public:
    ProgressNotification(const Algorithm* const alg, double p,const std::string& msg):AlgorithmNotification(alg),progress(p),message(msg){}///< Constructor
    virtual std::string name() const{return "ProgressNotification";}///< class name
    double progress;///< Current progress. Value must be between 0 and 1.
    std::string message;///< Message sent with notification
  };

  /// ErrorNotification is sent when an exception is caught during execution of the algorithm.
  class ErrorNotification: public AlgorithmNotification
  {
  public:
    /// Constructor
    ErrorNotification(const Algorithm* const alg, const std::string& str):AlgorithmNotification(alg),what(str){}
    virtual std::string name() const{return "ErrorNotification";}///< class name
    std::string what;///< message string
  };

  /// CancelException is thrown to cancel execution of the algorithm. Use Algorithm::cancel() to
  /// terminate an algorithm. The execution will only be stopped if Algorithm::exec() method calls
  /// periodically Algorithm::interuption_point() which checks if Algorithm::cancel() has been called
  /// and throws CancelException if needed.
  class CancelException : public std::exception
  {
  public:
    CancelException():outMessage("Algorithm terminated"){}
    CancelException(const CancelException& A):outMessage(A.outMessage){}///< Copy constructor
    /// Assignment operator
    CancelException& operator=(const CancelException& A);
    /// Destructor
    ~CancelException() throw() {}

    /// Returns the message string.
    const char* what() const throw()
    {
      return outMessage.c_str();
    }
  private:
    /// The message returned by what()
    std::string outMessage;

  };

  Algorithm();
  virtual ~Algorithm();
  /// function to return a name of the algorithm, must be overridden in all algorithms
  virtual const std::string name() const = 0;
  /// function to return a version of the algorithm, must be overridden in all algorithms
  virtual int version() const = 0;
  /// function to return a category of the algorithm. A default implementation is provided
  virtual const std::string category() const {return "Misc";}
  /// function to return any aliases to the algorithm;  A default implementation is provided
  virtual const std::string alias() const {return "";}

  /// Algorithm ID. Unmanaged algorithms return 0 (or NULL?) values. Managed ones have non-zero.
  AlgorithmID getAlgorithmID()const{return m_algorithmID;}

  // IAlgorithm methods
  void initialize();
  bool execute();
  virtual bool isInitialized() const;
  virtual bool isExecuted() const;
  // End of IAlgorithm methods
  using Kernel::PropertyManagerOwner::getProperty;

  /// To query whether algorithm is a child. Default to false
  bool isChild() const;
  void setChild(const bool isChild);

  void setRethrows(const bool rethrow);

  /// Asynchronous execution.
  Poco::ActiveResult<bool> executeAsync();

  /// Set the maximum number of cores from Properties files
  void getOpenMPCores();

  /// Execute as a sub-algorithm
  void executeAsSubAlg();

  /// Add an observer for a notification
  void addObserver(const Poco::AbstractObserver& observer)const;

  /// Remove an observer
  void removeObserver(const Poco::AbstractObserver& observer)const;

  /// Raises the cancel flag. interuption_point() method if called inside exec() checks this flag
  /// and if true terminates the algorithm.
  void cancel()const;
  /// True if the algorithm is running asynchronously.
  bool isRunningAsync(){return m_runningAsync;}
  /// True if the algorithm is running.
  bool isRunning(){return m_running;}

  ///Logging can be disabled by passing a value of false
  void setLogging(const bool value){g_log.setEnabled(value);}
  ///returns the status of logging, True = enabled
  bool isLogging() const {return g_log.getEnabled();}
  /// Returns a reference to the logger.
  Kernel::Logger& getLogger() const { return g_log; }
  /// Returns the cancellation state
  bool getCancel() const { return m_cancel; }

  /// Sets documentation strings for this algorithm
  virtual void initDocs() {};

  /// function returns an optional message that will be displayed in the default GUI, at the top.
  const std::string getOptionalMessage() const { return m_OptionalMessage;}

  /// Set  an optional message that will be displayed in the default GUI, at the top.
  void setOptionalMessage(const std::string optionalMessage) { m_OptionalMessage = optionalMessage;}

  /// Set a summary to be used in the wiki page. Normally, this is approx. the same as the optional message.
  void setWikiSummary(const std::string WikiSummary) { m_WikiSummary = WikiSummary;}

  ///setting the child start progress
  void setChildStartProgress(const double startProgress)const{m_startChildProgress=startProgress;}
  /// setting the child end progress
  void setChildEndProgress(const double endProgress)const{m_endChildProgress=endProgress;}

  /** @name Serialization functions */
  //@{
  /// Serialize an object to a string
  virtual std::string toString() const;
  /// De-serialize an object from a string
  static IAlgorithm_sptr fromString(const std::string & input);
  /// Construct an object from a history entry
  static IAlgorithm_sptr fromHistory(const AlgorithmHistory & history);
  //@}

  //creates a sub algorithm for use in this algorithm
  boost::shared_ptr<Algorithm> createSubAlgorithm(const std::string& name, const double startProgress = -1.,
      const double endProgress = -1., const bool enableLogging=true, const int& version = -1);

protected:

  // Equivalents of Gaudi's initialize & execute  methods
  /// Virtual method - must be overridden by concrete algorithm
  virtual void init() = 0;
  /// Virtual method - must be overridden by concrete algorithm
  virtual void exec() = 0;

  friend class AlgorithmProxy;
  /// Initialize with properties from an algorithm proxy
  void initializeFromProxy(const AlgorithmProxy&);

  void setInitialized();
  void setExecuted(bool state);

//  // Make PropertyManager's declareProperty methods protected in Algorithm
//  using Kernel::PropertyManagerOwner::declareProperty;

  /// Sends notifications to observers. Observers can subscribe to notificationCenter
  /// using Poco::NotificationCenter::addObserver(...);
  mutable Poco::NotificationCenter m_notificationCenter;

  friend class Progress;
  /// Sends ProgressNotification. p must be between 0 (just started) and 1 (finished)
  void progress(double p, const std::string& msg = "");
  /// Interrupts algorithm execution if Algorithm::cancel() has been called.
  /// Does nothing otherwise.
  void interruption_point();

  ///Observation slot for child algorithm progress notification messages, these are scaled and then signalled for this algorithm.
  void handleChildProgressNotification(const Poco::AutoPtr<ProgressNotification>& pNf);
  ///Child algorithm progress observer
  Poco::NObserver<Algorithm, ProgressNotification> m_progressObserver;
  ///checks that the value was not set by users, uses the value in EMPTY_DBL()
  static bool isEmpty(double toCheck) { return std::abs( (toCheck - EMPTY_DBL())/(EMPTY_DBL()) ) < 1e-8  ;}
  ///checks that the value was not set by users, uses the value in EMPTY_INT()
  static bool isEmpty(int toCheck) { return toCheck == EMPTY_INT(); }
  ///checks the property is a workspace property
  bool isWorkspaceProperty(const Kernel::Property* const prop) const;
  /// checks the property is input workspace property
  bool isInputWorkspaceProperty(const Kernel::Property* const prop) const;
  /// checks the property is output workspace property
  bool isOutputWorkspaceProperty(const Kernel::Property* const prop) const;
  /// This method checks the members workspaces are of similar names (example group_1,group_2) and returns true if they are.
	bool isGroupWorkspacesofSimilarNames(const std::string&,const std::vector<std::string>& grpmembersNames); 
		
  /// process workspace groups
  virtual bool processGroups(WorkspaceGroup_sptr ingrpws_sptr,const std::vector<Mantid::Kernel::Property*>&props);
  /// virtual method to set non workspace properties for an algorithm,it's useful for checking the period number when a member in a group workspace is executed
  virtual void setOtherProperties(IAlgorithm* alg,const std::string & propertyName,const std::string &propertyValue,int perioidNum);

  mutable bool m_cancel;    ///< set to true to stop execution
  bool m_parallelException; ///< Set if an exception is thrown, and not caught, within a parallel region
  Kernel::Logger& g_log;    ///< reference to the logger class

private:

  /// Private Copy constructor: NO COPY ALLOWED
  Algorithm(const Algorithm&);
  /// Private assignment operator: NO ASSIGNMENT ALLOWED
  Algorithm& operator=(const Algorithm&);

  void store();
  void fillHistory(Mantid::Kernel::DateAndTime, double,std::size_t);
  void findWorkspaceProperties(std::vector<Workspace_sptr>& inputWorkspaces,
      std::vector<Workspace_sptr>& outputWorkspaces) const;
  void algorithm_info() const;

  /// setting the input properties for an algorithm - to handle workspace groups
  bool setInputWSProperties(IAlgorithm* pAlg,Mantid::Kernel::Property* prop,const std::string& inMemberWSName );
  /// setting the output properties for an algorithm -to handle workspace groups
  bool setOutputWSProperties(IAlgorithm* pAlg,Mantid::Kernel::Property* prop,const int nPeriod,
      const std::string& inmemberwsName,WorkspaceGroup_sptr& outwsgrp_sptr,bool bSimilarNames,bool bequal);

  /// This method gets the input workspace name
  void getInputGroupWorkspaceName(const std::vector<Mantid::Kernel::Property*>& props,std::string& ingroupwsName);
  /// This method returns true if the input and output workspaces are same
  bool isInputequaltoOutPut(const std::vector<Mantid::Kernel::Property*>&props);


  /// Poco::ActiveMethod used to implement asynchronous execution.
  Poco::ActiveMethod<bool, Poco::Void, Algorithm> m_executeAsync;
  /** executeAsync() implementation.
      @param i :: Unused argument
   */
  bool executeAsyncImpl(const Poco::Void & i);


  bool m_isInitialized; ///< Algorithm has been initialized flag
  bool m_isExecuted; ///< Algorithm is executed flag
  bool m_isChildAlgorithm; ///< Algorithm is a child algorithm
  bool m_runningAsync; ///< Algorithm is running asynchronously
  bool m_running; ///< Algorithm is running
  bool m_rethrow; ///< Algorithm should rethrow exceptions while executing
  mutable double m_startChildProgress; ///< Keeps value for algorithm's progress at start of an sub-algorithm
  mutable double m_endChildProgress; ///< Keeps value for algorithm's progress at sub-algorithm's finish
  AlgorithmID m_algorithmID; ///< Algorithm ID for managed algorithms
  std::string m_OptionalMessage; ///< An optional message string to be displayed in the GUI.
  std::string m_WikiSummary; ///< A summary line for the wiki page.
  std::vector<IAlgorithm_wptr> m_ChildAlgorithms; ///< A list of weak pointers to any child algorithms created

  static size_t g_execCount; ///< Counter to keep track of algorithm execution order
};

///Typedef for a shared pointer to an Algorithm
typedef boost::shared_ptr<Algorithm> Algorithm_sptr;

} // namespace API
} // namespace Mantid


#endif /*MANTID_API_ALGORITHM_H_*/
