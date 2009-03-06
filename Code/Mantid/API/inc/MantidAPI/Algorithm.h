#ifndef MANTID_KERNEL_ALGORITHM_H_
#define MANTID_KERNEL_ALGORITHM_H_

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
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/PropertyManagerOwner.h"
#include "MantidKernel/Property.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Progress.h"

#include <boost/shared_ptr.hpp>
#include <Poco/ActiveMethod.h>
#include <Poco/NotificationCenter.h>
#include <Poco/Notification.h>
#include <Poco/NObserver.h>
#include <string>
#include <vector>
#include <map>

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

namespace Mantid
{
namespace API
{
/** @class Algorithm Algorithm.h Kernel/Algorithm.h

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

 Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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

class AlgorithmProxy;

class DLLExport Algorithm : public IAlgorithm, public Kernel::PropertyManagerOwner
{
public:

    /// Base class for algorithm notifications
    class AlgorithmNotification: public Poco::Notification
    {
    public:
        AlgorithmNotification(Algorithm* alg):Poco::Notification(),m_algorithm(alg){}///< Constructor
        const IAlgorithm *algorithm()const{return m_algorithm;}                       ///< The algorithm
    private:
        IAlgorithm *m_algorithm;///< The algorithm
    };

    /// StartedNotification is sent when the algorithm begins execution.
    class StartedNotification: public AlgorithmNotification
    {
    public:
        StartedNotification(Algorithm* alg):AlgorithmNotification(alg){}///< Constructor
        virtual std::string name() const{return "StartedNotification";}///< class name
    };

    /// FinishedNotification is sent after the algorithm finishes its execution
    class FinishedNotification: public AlgorithmNotification
    {
    public:
        FinishedNotification(Algorithm* alg, bool res):AlgorithmNotification(alg),success(res){}///< Constructor
        virtual std::string name() const{return "FinishedNotification";}///< class name
        bool success;///< true if the finished algorithm was successful or false if it failed.
    };

    /// An algorithm can report its progress by sending ProgressNotification. Use
    /// Algorithm::progress(double) function to send a preogress notification.
    class ProgressNotification: public AlgorithmNotification
    {
    public:
        ProgressNotification(Algorithm* alg, double p,const std::string& msg):AlgorithmNotification(alg),progress(p),message(msg){}///< Constructor
        virtual std::string name() const{return "ProgressNotification";}///< class name
        double progress;///< Current progress. Value must be between 0 and 1.
        std::string message;///< Message sent with notification
    };

    /// ErrorNotification is sent when an exception is caught during execution of the algorithm.
    class ErrorNotification: public AlgorithmNotification
    {
    public:
        /// Constructor
        ErrorNotification(Algorithm* alg, const std::string& str):AlgorithmNotification(alg),what(str){}
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
  virtual const std::string name() const {throw Kernel::Exception::AbsObjMethod("Algorithm");}
  /// function to return a version of the algorithm, must be overridden in all algorithms
  virtual const int version() const {throw Kernel::Exception::AbsObjMethod("Algorithm");}
  /// function to return a category of the algorithm. A default implementation is provided
  virtual const std::string category() const {return "Misc";}

  /// Algorithm ID. Unmanaged algorithms return 0 (or NULL?) values. Managed ones have non-zero.
  AlgorithmID getAlgorithmID()const{return m_algorithmID;}

  // IAlgorithm methods
  void initialize();
  bool execute();
  virtual bool isInitialized() const; // Protected in Gaudi version
  virtual bool isExecuted() const;
  //virtual void setPropertyOrdinal(const int &index, const std::string &value);
  //virtual void setPropertyValue(const std::string &name, const std::string &value);
  //virtual void setProperties(const std::string& propertiesArray);
  //virtual std::string getPropertyValue(const std::string &name) const;
  //virtual const std::vector< Mantid::Kernel::Property* >& getProperties() const;
  // End of IAlgorithm methods
  using Kernel::PropertyManagerOwner::getProperty;

  /// To query whether algorithm is a child. Default to false
  bool isChild() const;
  void setChild(const bool isChild);

  /// Asynchronous execution.
  Poco::ActiveResult<bool> executeAsync(){return _executeAsync(0);}

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

protected:

  // Equivalents of Gaudi's initialize & execute  methods
  /// Virtual method - must be overridden by concrete algorithm
  virtual void init() = 0;
  /// Virtual method - must be overridden by concrete algorithm
  virtual void exec() = 0;

  friend class AlgorithmProxy;
  /// Initialize with properties from an algorithm proxy
  void initializeFromProxy(const AlgorithmProxy&);

  //creates a sub algorithm for use in this algorithm
  IAlgorithm_sptr createSubAlgorithm(const std::string& name, double startProgress = -1., double endProgress = -1.);

  void setInitialized();
  void setExecuted(bool state);

  // Make PropertyManager's declareProperty methods protected in Algorithm
  using Kernel::PropertyManagerOwner::declareProperty;

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
  ///Child algoirthm progress observer
	Poco::NObserver<Algorithm, ProgressNotification> m_progressObserver;

private:

  /// Poco::ActiveMethod is used to implement asynchronous execution.
  Poco::ActiveMethod<bool, int, Algorithm> _executeAsync;
  /// Private Copy constructor: NO COPY ALLOWED
  Algorithm(const Algorithm&);
  /// Private assignment operator: NO ASSIGNMENT ALLOWED
  Algorithm& operator=(const Algorithm&);

  void store();
  void fillHistory(AlgorithmHistory::dateAndTime, double);
  void findWorkspaceProperties(std::vector<Workspace_sptr>& inputWorkspaces,
                               std::vector<Workspace_sptr>& outputWorkspaces) const;
  void algorithm_info() const;

  /// Static refenence to the logger class
  static Kernel::Logger& g_log;

  bool m_isInitialized; ///< Algorithm has been initialized flag
  bool m_isExecuted; ///< Algorithm is executed flag

  bool m_isChildAlgorithm; ///< Algorithm is a child algorithm

  bool executeAsyncImpl(const int&);///< executeAsync implementation.
  mutable bool m_cancel; ///< set to true to stop execution
  bool m_runningAsync; ///< Algorithm is running asynchronously
  bool m_running; ///< Algorithm is running

  double m_startChildProgress; ///< Keeps value for algorithm's progress at start of an sub-algorithm
  double m_endChildProgress; ///< Keeps value for algorithm's progress at sub-algorithm's finish

  AlgorithmID m_algorithmID; ///< Algorithm ID for managed algorithms
};

///Typedef for a shared pointer to an Algorithm
typedef boost::shared_ptr<Algorithm> Algorithm_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_ALGORITHM_H_*/
