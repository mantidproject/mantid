#ifndef MANTID_DATAHANDLING_SaveAscii2_H_
#define MANTID_DATAHANDLING_SaveAscii2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace DataHandling {
/** @class SaveAscii2 SaveAscii2.h DataHandling/SaveAscii2.h

Saves a workspace or selected spectra in a coma-separated ascii file. Spectra
are saved in columns.

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
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveAscii"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 2D workspace to a ascii file.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadAscii", "SaveANSTOAscii", "SaveCSV", "SaveDiffFittingAscii",
            "SaveMFT", "SaveOpenGenieAscii", "SaveGSS", "SaveFocusedXYE"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  /// Writes a spectrum to the file using a workspace index
  void writeSpectrum(const int &wsIndex, std::ofstream &file);
  std::vector<std::string> stringListToVector(std::string &inputString);
  void populateQMetaData();
  void populateSpectrumNumberMetaData();
  void populateAngleMetaData();
  void populateAllMetaData();
  bool
  findElementInUnorderedStringVector(const std::vector<std::string> &vector,
                                     const std::string &toFind);

  /// Map the separator options to their string equivalents
  std::map<std::string, std::string> m_separatorIndex;

  int m_nBins;
  std::string m_sep;
  bool m_writeDX;
  bool m_writeID;
  bool m_isCommonBins;
  API::MatrixWorkspace_const_sptr m_ws;
  std::vector<std::string> m_metaData;
  std::map<std::string, std::vector<std::string>> m_metaDataMap;
};
} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SaveAscii2_H_  */
