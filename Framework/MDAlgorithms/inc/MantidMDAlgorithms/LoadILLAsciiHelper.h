#ifndef MANTID_MDALGORITHMS_LOADILLASCIIHELPER_H_
#define MANTID_MDALGORITHMS_LOADILLASCIIHELPER_H_

#include "MantidMDAlgorithms/DllConfig.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>

namespace Mantid {
namespace MDAlgorithms {

/** LoadILLAsciiHelper :

 This parses ILL data in Ascii format.

 For more details on data format, please see:
 <http://www.ill.eu/instruments-support/computing-for-science/data-analysis/raw-data/>

 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class ILLParser {
public:
  ILLParser(const std::string &filename);
  virtual ~ILLParser();
  void parse();
  void showHeader();
  std::string getInstrumentName();

  // Those are large vectors: returning references => no copy constructors
  // std::vector< std::vector<int> > & getSpectraList() {return spectraList;}
  // std::vector<std::map<std::string, std::string> > & getSpectraHeaderList()
  // {return spectraHeaders;}
  // No difference in running times :(

  std::vector<std::vector<int>> getSpectraList() const { return spectraList; }
  std::vector<std::map<std::string, std::string>> getSpectraHeaderList() const {
    return spectraHeaders;
  }

  template <typename T> T getValueFromHeader(const std::string &);
  template <typename T>
  T getValue(const std::string &, const std::map<std::string, std::string> &);

private:
  void parseFieldR();
  void parseFieldA();
  void parseFieldNumeric(std::map<std::string, std::string> &header,
                         int fieldWith);
  std::vector<int> parseFieldISpec(int fieldWith = intWith);

  void startParseSpectra();
  std::vector<std::string> splitLineInFixedWithFields(const std::string &s,
                                                      int fieldWidth,
                                                      int lineWitdh = lineWith);

  template <typename T> T evaluate(std::string field);

  static const int lineWith = 80;
  static const int intWith = 8;
  static const int floatWith = 16;

  std::ifstream fin;
  // Not great, but to date there's 3 containers:
  std::map<std::string, std::string> header; // file global header
  std::vector<std::map<std::string, std::string>>
      spectraHeaders; // list with every spectrum header
  std::vector<std::vector<int>>
      spectraList; // same size list but with spectra contents
};
}
// namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_LOADILLASCIIHELPER_H_ */
