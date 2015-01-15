#ifndef MANTID_DATAHANDLING_REMOVELOGS_H_
#define MANTID_DATAHANDLING_REMOVELOGS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Run.h"

namespace Mantid {

namespace DataHandling {
/** @class RemoveLogs RemoveLogs.h DataHandling/RemoveLogs.h

    Load ISIS log file(s). Assumes that a log file originates from a
    PC (not VMS) environment, i.e. the log files to be loaded are assumed
    to have the extension .txt. Its filename is assumed to starts with the raw
   data
    file identifier followed by the character '_', and a log file is assumed to
   have a
    format of two columns, where the first column consists of data-time strings
   of the
    ISO 8601 form and the second column consists of either numbers or strings
   that may
    contain spaces.

    The algoritm requires an input filename. If this filename is the name of a
    raw datafile the algorithm will attempt to read in all the log files
   associated
    with that log file. Otherwise it will assume the filename specified is the
    filename of a specific log file.

    RemoveLogs is an algorithm and as such inherits from the Algorithm class,
    via DataHandlingCommand, and overrides the init() & exec() methods.
    RemoveLogs is intended to be used as a child algorithm of
    other Loadxxx algorithms, rather than being used directly.

    Required Properties:
    <UL>
    <LI> Filename - The filename (including its full or relative path) of either
   an ISIS log file
    or an ISIS raw file. If a raw file is specified all log files associated
   with that raw file
    are loaded into the specified workspace. The file extension must either be
   .raw or .s when
    specifying a raw file, and at least 10 characters long. </LI>
    <LI> Workspace - The workspace to which the log data is appended </LI>
    </UL>

    @author Vickie Lynch, SNS
    @date 26/04/2012

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport RemoveLogs : public API::Algorithm {
public:
  /// Default constructor
  RemoveLogs();

  /// Destructor
  ~RemoveLogs() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "RemoveLogs"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Remove logs from a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Logs"; }

private:
  /// Overwrites Algorithm method.
  void init();

  /// Overwrites Algorithm method
  void exec();
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_REMOVELOGS_H_*/
