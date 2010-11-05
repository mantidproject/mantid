#ifndef MANTID_NEXUS_LOADMUONLOG_H_
#define MANTID_NEXUS_LOADMUONLOG_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{

  namespace NeXus
  {
    /** @class LoadMuonLog LoadMuonLog.h Nexus/LoadMuonLog.h

    Load ISIS Muon log data from a Nexus file. Sections of NXlog values within the
    first run will be loaded.

    The algoritm requires an input filename. If this filename is the name of a
    Nexus file the algorithm will attempt to read in all the log data (NXlog)
    within the first run section of that file.

    LoadMuonLog is an algorithm and as such inherits from the Algorithm class,
    via Nexus, and overrides the init() & exec() methods.
    LoadMuonLog is intended to be used as a child algorithm of
    other Loadxxx algorithms, rather than being used directly.

    Required Properties:
    <UL>
    <LI> Filename - The full name of and path of the input ISIS Nexus file </LI>
    <LI> Workspace - The workspace to which to append the log data </LI>
    </UL>

    @author Ronald Fowler, based on LoadLog by Anders Markvardsen, ISIS, RAL
    @date 11/08/2008

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
    class DLLExport LoadMuonLog : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadMuonLog();

      /// Destructor
      virtual ~LoadMuonLog() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadMuonLog";};
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Nexus\\Logs";}

    private:

      /// Overwrites Algorithm method.
      void init();

      /// Overwrites Algorithm method
      void exec();


      /// The name and path of an input file. This may be the filename of a
      /// raw datafile or the name of a specific log file.
      std::string m_filename;

      /// convert string to lower case
      std::string stringToLower(std::string strToConvert);

      /// check if first 19 characters of a string is data-time string according to yyyy-mm-ddThh:mm:ss
      bool isDateTimeString(const std::string& str);
    };

  } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_NEXUS_LOADMUONLOG_H_*/
