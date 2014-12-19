#ifndef MANTID_DATAHANDLING_SaveAscii2_H_
#define MANTID_DATAHANDLING_SaveAscii2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace DataHandling {
/** @class SaveAscii2 SaveAscii2.h DataHandling/SaveAscii2.h

Saves a workspace or selected spectra in a coma-separated ascii file. Spectra
are saved in columns.
Properties:
<ul>
<li>Filename - the name of the file to write to.  </li>
<li>Workspace - the workspace name to be saved.</li>
<li>SpectrumMin - the starting spectrum index to save (optional) </li>
<li>SpectrumMax - the ending spectrum index to save (optional) </li>
<li>SpectrumList - a list of comma-separated spectra indeces to save (optional)
</li>
<li>Precision - the numeric precision - the number of significant digits for the
saved data (optional) </li>
</ul>


@author Keith Brown, ISIS, Placement student from the University of Derby
@date 10/10/13

Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SaveAscii2 : public API::Algorithm {
public:
  /// Default constructor
  SaveAscii2();
  /// Destructor
  ~SaveAscii2() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SaveAscii"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Saves a 2D workspace to a ascii file.";
  }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 2; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Text"; }

private:
  /// Overwrites Algorithm method.
  void init();
  /// Overwrites Algorithm method
  void exec();
  /**writes a spectra to the file using a workspace ID
  @param spectraIndex :: an integer relating to a workspace ID
  @param file :: the file writer object
  */
  void writeSpectra(const int &spectraIndex, std::ofstream &file);
  /**writes a spectra to the file using an iterator
  @param spectraItr :: a set<int> iterator pointing to a set of workspace IDs to
  be saved
  @param file :: the file writer object
  */
  void writeSpectra(const std::set<int>::const_iterator &spectraItr,
                    std::ofstream &file);

  /// Map the separator options to their string equivalents
  std::map<std::string, std::string> m_separatorIndex;

  int m_nBins;
  std::string m_sep;
  bool m_writeDX;
  bool m_writeID;
  bool m_isHistogram;
  bool m_isCommonBins;
  API::MatrixWorkspace_const_sptr m_ws;
};
} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SaveAscii2_H_  */
