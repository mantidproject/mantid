#ifndef MANTID_DATAHANDLING_SaveILLCosmosAscii_H_
#define MANTID_DATAHANDLING_SaveILLCosmosAscii_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
  namespace DataHandling
  {
    /** @class SaveILLCosmosAscii SaveILLCosmosAscii.h DataHandling/SaveILLCosmosAscii.h

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


    @author Keith Brown, ISIS, Placement student from the University of Derby
    @date 10/10/13

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

    File change history is stored at: <https://github.com/mantidproject/mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport SaveILLCosmosAscii : public API::Algorithm
    {
    public:
      /// Default constructor
      SaveILLCosmosAscii();
      /// Destructor
      ~SaveILLCosmosAscii() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveILLCosmosAscii"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Text"; }

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
      /// returns true if the value is NaN
      bool checkIfNan(const double& value) const;
      /// returns true if the value if + or - infinity
      bool checkIfInfinite(const double& value) const;
      /// print the appropriate value to file
      void outputval (double val, std::ofstream & file, bool leadingSep = true);
      ///static reference to the logger class
      static Kernel::Logger& g_log;

      /// Map the separator options to their string equivalents
      std::map<std::string,std::string> m_separatorIndex;

      int m_nBins;
      char m_sep;
      bool m_writeDX;
      bool m_isHistogram;
      API::MatrixWorkspace_const_sptr m_ws;
    };
  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_SaveILLCosmosAscii_H_  */
