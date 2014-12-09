#ifndef MANTID_API_IALGORITHM_H_
#define MANTID_API_IALGORITHM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/IPropertyManager.h"

namespace Poco
{
  class AbstractObserver;
  template<class T> class ActiveResult;
}

namespace Mantid
{
namespace API
{

/** As we have multiple interfaces to the same logical algorithm (Algorithm & AlgorithmProxy)
 *  we need a way of uniquely identifying managed algorithms. It can be AlgorithmID.
 */
typedef void* AlgorithmID;

/**
 IAlgorithm is the interface implemented by the Algorithm base class.
 Concrete algorithms, derived from the Algorithm base class are controlled 
 via this interface.

 @author Russell Taylor, Tessella Support Services plc
 @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
 @date 11/09/2007
 
 Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MANTID_API_DLL IAlgorithm : virtual public Kernel::IPropertyManager
{
public:

  /// Virtual destructor (always needed for abstract classes)
  virtual ~IAlgorithm() {}

  /// function to return a name of the algorithm, must be overridden in all algorithms
  virtual const std::string name() const = 0;

  /// function to return a version of the algorithm, must be overridden in all algorithms
  virtual int version() const = 0;

  /// function returns a summary message that will be displayed in the default GUI, and in the help.
  virtual const std::string summary() const = 0;
  
  /// function to return a category of the algorithm.
  virtual const std::string category() const = 0;

  /// Function to return all of the categories that contain this algorithm
  virtual const std::vector<std::string> categories() const =0;

  /// Function to return the separator token for the category string
  virtual const std::string categorySeparator() const =0;

  /// function to return any aliases of the algorithm.
  virtual const std::string alias() const = 0;

  /** @name Algorithms As Methods */
  ///@{
  /// Returns a name that will be used when attached as a workspace method. Empty string indicates do not attach
  virtual const std::string workspaceMethodName() const = 0;
  /// Returns a set of class names that will have the method attached. Empty list indicates all types
  virtual const std::vector<std::string> workspaceMethodOn() const = 0;
  /// Returns the name of the input workspace property used by the calling object
  virtual const std::string workspaceMethodInputProperty() const = 0;
  ///@}

  /// Algorithm ID. Unmanaged algorithms return 0 (or NULL?) values. Managed ones have non-zero.
  virtual AlgorithmID getAlgorithmID()const = 0;

  /** Initialization method invoked by the framework. This method is responsible
   *  for any bookkeeping of initialization required by the framework itself.
   *  It will in turn invoke the init() method of the derived algorithm,
   *  and of any Child Algorithms which it creates.
   */
  virtual void initialize() = 0;

  /// Method checking errors on ALL the inputs, before execution. For use mostly in dialogs.
  virtual std::map<std::string, std::string> validateInputs() = 0;

  /// System execution. This method invokes the exec() method of a concrete algorithm.
  virtual bool execute() = 0;

  /// Asynchronous execution of the algorithm.
  virtual Poco::ActiveResult<bool> executeAsync() = 0;

  /// Execute as a Child Algorithm, with try/catch
  virtual void executeAsChildAlg() = 0;

  /// Check whether the algorithm is initialized properly
  virtual bool isInitialized() const = 0;
  /// Check whether the algorithm has already been executed
  virtual bool isExecuted() const = 0;

  /// Raises the cancel flag. interuption_point() method if called inside exec() checks this flag
  /// and if true terminates the algorithm.
  virtual void cancel() = 0;

  /// True if the algorithm is running.
  virtual bool isRunning() const = 0;

  /// To query whether algorithm is a child. Default to false
  virtual bool isChild() const = 0;


  /** To set whether algorithm is a child.
   *  @param isChild :: True - the algorithm is a child algorithm.  False - this is a full managed algorithm.
   */
  virtual void setChild(const bool isChild) = 0;

  /// If true history will be recorded for a child
  virtual void enableHistoryRecordingForChild(const bool on) = 0;

  /// Set whether we always store the output in the analysis data service
  virtual void setAlwaysStoreInADS(const bool doStore) = 0;

  /// To query whether an algorithm should rethrow exceptions when executing.
  virtual void setRethrows(const bool rethrow) = 0;

  /// Add an observer for a notification
  virtual void addObserver(const Poco::AbstractObserver& observer)const = 0;

  /// Remove an observer
  virtual void removeObserver(const Poco::AbstractObserver& observer)const = 0;

  ///Logging can be disabled by passing a value of false
  virtual void setLogging(const bool value) = 0;
  ///returns the status of logging, True = enabled
  virtual bool isLogging() const = 0;  
  ///gets the logging priority offset
  virtual void setLoggingOffset(const int value) = 0;
  ///returns the logging priority offset
  virtual int getLoggingOffset() const = 0;
  /// disable Logging of start and end messages
  virtual void setAlgStartupLogging(const bool enabled) = 0;
  /// get the state of Logging of start and end messages
  virtual bool getAlgStartupLogging() const = 0;
  ///setting the child start progress
  virtual void setChildStartProgress(const double startProgress)const = 0;
  /// setting the child end progress
  virtual void setChildEndProgress(const double endProgress)const = 0;
  /// Serialize an algorithm
  virtual std::string toString() const = 0;
};

typedef boost::shared_ptr<IAlgorithm> IAlgorithm_sptr;
typedef boost::shared_ptr<const IAlgorithm> IAlgorithm_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_IALGORITHM_H_*/
