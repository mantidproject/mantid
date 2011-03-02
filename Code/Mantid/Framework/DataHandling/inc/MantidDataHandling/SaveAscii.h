#ifndef MANTID_DATAHANDLING_SAVEASCII_H_
#define MANTID_DATAHANDLING_SAVEASCII_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace DataHandling
  {
    /** @class SaveAscii SaveAscii.h DataHandling/SaveAscii.h

    Saves a workspace or selected spectra in a coma-separated ascii file. Spectra are saved in columns.
    Properties:
    <ul>
          <li>Filename - the name of the file to write to.  </li>
          <li>Workspace - the workspace name to be saved.</li>
          <li>SpectrumMin - the starting spectrum index to save (optional) </li>
          <li>SpectrumMax - the ending spectrum index to save (optional) </li>
          <li>SpectrumList - a list of comma-separated spectra indeces to save (optional) </li>
          <li>Precision - the numeric precision - the number of significant digits for the saved data (optional) </li>
    </ul>


    @author Roman Tolchenov, Tessella plc
    @date 3/07/09

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport SaveAscii : public API::Algorithm
    {
    public:
      /// Destructor
      ~SaveAscii() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveAscii"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
      ///static reference to the logger class
      static Kernel::Logger& g_log;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_SAVEASCII_H_  */
