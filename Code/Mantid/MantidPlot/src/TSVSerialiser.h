#ifndef MANTID_TSVSERIALISER_H_
#define MANTID_TSVSERIALISER_H_

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "qstring.h"

/** Parses the formatting used in MantidPlot project files

  @author Harry Jeffery, ISIS, RAL
  @date 23/07/2014

  Copyright &copy; 2007-2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
*/
class TSVSerialiser
{
public:

  TSVSerialiser();

  TSVSerialiser(const std::string& lines);

  void parseLines(const std::string& lines);
  std::string outputLines() const;
  void clear();

  bool hasLine(const std::string& name) const;
  bool hasSection(const std::string& name) const;

  std::vector<std::string> values(const std::string& name, const size_t i = 0) const;
  std::vector<std::string> sections(const std::string& name) const;

  std::string lineAsString(const std::string& name, const size_t i = 0) const;

  bool selectLine(const std::string& name, const size_t i = 0);
  bool selectSection(const std::string& name, const size_t i = 0);

  int         asInt(const size_t i) const;
  double      asDouble(const size_t i) const;
  float       asFloat(const size_t i) const;
  std::string asString(const size_t i) const;

  TSVSerialiser& operator>>(int& val);
  TSVSerialiser& operator>>(double& val);
  TSVSerialiser& operator>>(float& val);
  TSVSerialiser& operator>>(std::string& val);
  TSVSerialiser& operator>>(QString& val);

  TSVSerialiser& writeLine(const std::string& name);

  TSVSerialiser& operator<<(const std::string& val);
  TSVSerialiser& operator<<(const char* val);
  TSVSerialiser& operator<<(const QString& val);
  TSVSerialiser& operator<<(const double& val);
  TSVSerialiser& operator<<(const int& val);

  void writeRaw(const std::string& raw);
  void writeSection(const std::string& name, const std::string& body);
  void writeInlineSection(const std::string& name, const std::string& body);

private:
  std::map<std::string,std::vector<std::string> > m_sections;
  std::map<std::string,std::vector<std::string> > m_lines;

  std::vector<std::string> m_curValues;
  int m_curIndex;

  std::stringstream m_output;
  bool m_midLine;
};

#endif
