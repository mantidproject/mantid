#ifndef MANTID_ALGORITHMS_COPYLOGS_H_
#define MANTID_ALGORITHMS_COPYLOGS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{

  /** CopyLogs

    An algorithm to copy the sample logs from one workspace to another.
    This algorithm also provides several options for merging sample logs from one workspace to another.

    @author Samuel Jackson, STFC, RAL
    @date 11/10/2013
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport CopyLogs  : public API::Algorithm
  {
  public:
    CopyLogs();
    virtual ~CopyLogs();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Copies the sample logs from one workspace to another.";}

    virtual int version() const;
    virtual const std::string category() const;

  private:
    
    void init();
    void exec();

    /// appends new logs and overwrites existing logs. 
    void mergeReplaceExisting(const std::vector< Kernel::Property* >& inputLogs, API::Run& outputRun);
    /// appends new logs but leaves exisitng logs untouched.
    void mergeKeepExisting(const std::vector< Kernel::Property* >& inputLogs, API::Run& outputRun);
    /// appends new logs and removes all existing logs.
    void wipeExisting(const std::vector< Kernel::Property* >& inputLogs, API::Run& outputRun);
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_COPYLOGS_H_ */