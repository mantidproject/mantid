#ifndef MANTID_KERNEL_ALGORITHM_H_
#define MANTID_KERNEL_ALGORITHM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/Logger.h"

#include "boost/shared_ptr.hpp"
#include <vector>
#include <map>

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

namespace Mantid
{
namespace API
{
  ///Typedef for a shared pointer to an Algorithm
  typedef boost::shared_ptr<Algorithm> Algorithm_sptr;

/** @class Algorithm Algorithm.h Kernel/Algorithm.h

 Base class from which all concrete algorithm classes should be derived. 
 In order for a concrete algorithm class to do anything
 useful the methods init(), exec() and final() should be overridden.
 
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
 
 Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
class DLLExport Algorithm : public IAlgorithm, virtual protected Kernel::PropertyManager
{
public:
  Algorithm();
  virtual ~Algorithm();

  virtual const std::string& name() const;

  // IAlgorithm methods	  
  virtual const std::string& version() const;
  void initialize();
  void execute();
  void finalize();
  virtual bool isInitialized() const; // Protected in Gaudi version
  virtual bool isExecuted() const;
  virtual bool isFinalized() const;

  virtual void setPropertyValue(const std::string &name, const std::string &value);
  virtual std::string getPropertyValue(const std::string &name) const;

  // Make PropertyManager's setProperty methods public
  using Kernel::PropertyManager::setProperty;

  /// To query whether algorithm is a child. Default to false
  bool isChild() const;
  void setChild(const bool isChild);

  Algorithm_sptr createSubAlgorithm(const std::string& name);

  /// List of sub-algorithms (const version). Returns a reference to a vector of (sub) Algorithms
  const std::vector<Algorithm_sptr>& subAlgorithms() const
  {
    return m_subAlgms;
  }
  /// List of sub-algorithms. Returns a reference to a vector of (sub) Algorithms
  std::vector<Algorithm_sptr>& subAlgorithms()
  {
    return m_subAlgms;
  }

protected:

  // Equivalents of Gaudi's initialize, execute & finalize methods
  /// Virtual method - must be overridden by concrete algorithm
  virtual void init() = 0;
  /// Virtual method - must be overridden by concrete algorithm
  virtual void exec() = 0;
  /// Virtual method - must be overridden by concrete algorithm
  virtual void final() = 0;

  void setInitialized();
  void setExecuted(bool state);
  void setFinalized();

  
private:

  /// Private Copy constructor: NO COPY ALLOWED
  Algorithm(const Algorithm& a);
  /// Private assignment operator: NO ASSIGNMENT ALLOWED
  Algorithm& operator=(const Algorithm& rhs);

  /// Put any output workspaces into the AnalysisDataService
  void store();

  std::string m_name; ///< Algorithm's name for identification
  std::string m_version; ///< Algorithm's version
  std::vector<Algorithm_sptr> m_subAlgms; ///< Sub algorithms

  /// Static refenence to the logger class
  static Kernel::Logger& g_log;

  bool m_isInitialized; ///< Algorithm has been initialized flag
  bool m_isExecuted; ///< Algorithm is executed flag
  bool m_isFinalized; ///< Algorithm has been finalized flag

  bool m_isChildAlgorithm; ///< Algorithm is a child algorithm
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_ALGORITHM_H_*/
