#ifndef MANTID_KERNEL_ENVIRONMENTHISTORY_H_
#define MANTID_KERNEL_ENVIRONMENTHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <string>
#include <iostream>

namespace Mantid
{
namespace Kernel
{
/** This class stores information about the Environment of the computer used by the framework.

    @author Dickon Champion, ISIS, RAL
    @date 21/01/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport EnvironmentHistory
{
public:
  EnvironmentHistory();
  EnvironmentHistory(const EnvironmentHistory&);
  virtual ~EnvironmentHistory();
  /// print contents of object
  void printSelf(std::ostream&, const int indent = 0)const;
  /// returns the frameworkversion
  const std::string& frameworkVersion()  const{ return m_version;}
   /// returns the os name
  const std::string& osName()const {return m_osName;}
   /// returns the os version
  const std::string& osVersion()const {return m_osVersion;} 
   /// returns the user name
  const std::string& userName()const {return m_userName;}
 
private:
  /// Private, unimplemented copy assignment operator
  EnvironmentHistory& operator=(const EnvironmentHistory& );

  /// The version of the framework
  const std::string m_version;
  /// The name of the operating system
  const std::string m_osName;
  /// The version of the operating system
  const std::string m_osVersion;
  /// The name of the user running the framwork
  const std::string m_userName;
};

DLLExport std::ostream& operator<<(std::ostream&, const EnvironmentHistory&);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_ENVIRONMENTHISTORY_H_*/
