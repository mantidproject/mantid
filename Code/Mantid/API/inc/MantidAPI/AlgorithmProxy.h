#ifndef MANTID_API_ALGORITHMPROXY_H_
#define MANTID_API_ALGORITHMPROXY_H_

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
/** @class AlgorithmProxy AlgorithmProxy.h Kernel/AlgorithmProxy.h

   A proxy calss that stands between the user and the actual algorithm. 
   AlgorithmDataService stores algorthm proxies. Actual algorithms are 
   created by the proxy and destroyed after execution to free memory. 
   Algorithm and its proxy share all properties.

 @author Russell Taylor, Tessella Support Services plc
 @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
 @date 12/09/2007
 @author Roman Tolchenov, Tessella plc
 @date 03/03/2009

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
class Algorithm;
typedef boost::shared_ptr<Algorithm> Algorithm_sptr;

class DLLExport AlgorithmProxy : public IAlgorithm, public Kernel::PropertyManagerOwner
{
public:

  AlgorithmProxy(IAlgorithm_sptr alg);
  virtual ~AlgorithmProxy();

  /// function to return a name of the algorithm
  const std::string name() const {return m_name;}
  /// function to return a version of the algorithm
  const int version() const {return m_version;}
  /// function to return a category of the algorithm. 
  const std::string category() const {return m_category;}

  /// Algorithm ID. Unmanaged algorithms return 0 (or NULL?) values. Managed ones have non-zero.
  AlgorithmID getAlgorithmID()const{return AlgorithmID(this);}

  void initialize();
  bool execute();
  bool isInitialized() const;
  bool isExecuted() const;


  /// To query whether algorithm is a child. Default to false
  bool isChild() const{return false;} ///< A proxy is always at top level, returns false
  void setChild(const bool isChild){} ///< Do nothing

  /// Asynchronous execution.
  Poco::ActiveResult<bool> executeAsync(){return _executeAsync(0);}

  /// Raises the cancel flag. interuption_point() method if called inside exec() checks this flag
  /// and if true terminates the algorithm.
  void cancel()const;

  /// True if the algorithm is running asynchronously.
  bool isRunningAsync();

  /// True if the algorithm is running.
  bool isRunning();

  /// Add an observer for a notification
  void addObserver(const Poco::AbstractObserver& observer)const;

  /// Remove an observer
  void removeObserver(const Poco::AbstractObserver& observer)const;

protected:

  // Equivalents of Gaudi's initialize & execute  methods
  /// Virtual method - must be overridden by concrete algorithm
    void init(){}
  /// Virtual method - must be overridden by concrete algorithm
    void exec(){}

  // Make PropertyManager's declareProperty methods protected in Algorithm
  using Kernel::PropertyManagerOwner::declareProperty;

private:

    /// Private Copy constructor: NO COPY ALLOWED
    AlgorithmProxy(const AlgorithmProxy&);
    /// Private assignment operator: NO ASSIGNMENT ALLOWED
    AlgorithmProxy& operator=(const AlgorithmProxy&);

    /// Clean up when the real algorithm stops
    void stopped();
    /// Add observers stored previously in m_externalObservers
    void addObservers();

  /// Poco::ActiveMethod used to implement asynchronous execution.
  Poco::ActiveMethod<bool, int, AlgorithmProxy> _executeAsync;
    /** executeAsync() implementation. Calls Algorithm::executeAsync() and when it has finished
      deletes the real algorithm.
      @param i Unused argument
    */
    bool executeAsyncImpl(const int& i);

    std::string m_name;     ///< name of the real algorithm
    std::string m_category; ///< category of the real algorithm
    int m_version;          ///< version of the real algorithm

    Algorithm_sptr m_alg;  ///< Pointer to the real algorithm, only defined when the algorithm is running
    bool m_isExecuted;     ///< Executed flag

    /// Temporary holder of external observers wishing to subscribe
    mutable std::vector<const Poco::AbstractObserver*> m_externalObservers;

    /// Static refenence to the logger class
    static Kernel::Logger& g_log;

};

///Typedef for a shared pointer to an Algorithm
typedef boost::shared_ptr<AlgorithmProxy> AlgorithmProxy_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHMPROXY_H_*/
