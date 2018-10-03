// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include "MantidKernel/Logger.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <sstream>

namespace {
Mantid::Kernel::Logger g_log("TSVSerialiser");
}

using namespace MantidQt::API;

TSVSerialiser::TSVSerialiser() : m_curIndex(0), m_midLine(false) {}

TSVSerialiser::TSVSerialiser(const std::string &lines)
    : m_curIndex(0), m_midLine(false) {
  parseLines(lines);
}

void TSVSerialiser::parseLines(const std::string &lines) {
  std::vector<std::string> lineVec;
  boost::split(lineVec, lines, boost::is_any_of("\n"));

  // Clear out any old data.
  m_lines.clear();
  m_sections.clear();

  boost::regex valueLineRegex("\\s*([a-zA-Z0-9]+)\\b.*");
  boost::regex closedSectionRegex("\\s*<([a-zA-Z0-9]+)>(.*)</\\1>");
  boost::regex openSectionRegex("\\s*<([a-zA-Z0-9]+)( [0-9]+)?>(.*)");

  for (auto lineIt = lineVec.begin(); lineIt != lineVec.end(); ++lineIt) {
    const std::string line = *lineIt;

    if (line.length() == 0)
      continue;

    // Stores matched sections of a regex
    boost::smatch matches;

    // Check if this is a value line
    if (boost::regex_match(line, matches, valueLineRegex)) {
      std::string name = matches[1].str();
      m_lines[name].push_back(line);
    }
    // Look for lines which open and close a section in one line:
    // <section>data</section>
    else if (boost::regex_match(line, matches, closedSectionRegex)) {
      std::string name = matches[1].str();
      std::string contents = matches[2].str();

      m_sections[name].push_back(contents);
    }
    // Check if this is the start of a multiline section, if so, consume the
    // whole section.
    else if (boost::regex_match(line, matches, openSectionRegex)) {
      std::stringstream sectionSS;

      std::string name = matches[1].str();
      std::string firstLine = matches[2].str();
      std::string num;
      if (matches.size() == 4) {
        num = matches[2].str();
        firstLine = matches[3].str();
      }

      // firstLine exists because of a legacy edgecase: the <folder> section
      // keeps values on the same line as
      // the opening tag, so we have to be able to read that.
      if (firstLine.length() > 0)
        sectionSS << firstLine << "\n";

      boost::regex openRegex("\\s*<" + name + num + ">.*");
      boost::regex closeRegex("\\s*</" + name + ">");

      // Lets iterate over the contents of the section
      auto secIt = lineIt + 1;

      // Search for opening and closing tags, counting depth and building the
      // section string.
      for (int depth = 1; depth > 0 && secIt != lineVec.end(); ++secIt) {
        std::string secLine = *secIt;
        // Are we going down?
        if (boost::regex_match(secLine, openRegex))
          depth++;
        else if (boost::regex_match(secLine, closeRegex))
          depth--;

        if (depth > 0)
          sectionSS << secLine << "\n";
      }

      // We've now advanced beyond the end of the section so go back one
      secIt--;

      std::string sectionStr = sectionSS.str();

      // We drop the last character because it's a spare newline
      if (sectionStr.size() > 0)
        sectionStr.resize(sectionStr.size() - 1);

      m_sections[name + num].push_back(sectionStr);

      // Skip parsing to the end of the section
      lineIt = secIt;
    } else {
      // If we've made it here then we don't know what kind of line this is.
      g_log.warning()
          << "Unable to identify line in TSVSerialiser::parseLines(): '" << line
          << "'\n";
    }
  }
}

bool TSVSerialiser::hasLine(const std::string &name) const {
  return (m_lines.find(name) != m_lines.end());
}

bool TSVSerialiser::hasSection(const std::string &name) const {
  return (m_sections.find(name) != m_sections.end());
}

std::vector<std::string> TSVSerialiser::values(const std::string &name,
                                               size_t i) const {
  // Select correct line with lineAsString, parse it, then return values
  std::vector<std::string> ret;

  std::string line = lineAsString(name, i);
  boost::split(ret, line, boost::is_any_of("\t"));

  return ret;
}

std::vector<std::string>
TSVSerialiser::sections(const std::string &name) const {
  if (!hasSection(name))
    return std::vector<std::string>();

  return m_sections.at(name);
}

std::string TSVSerialiser::lineAsString(const std::string &name,
                                        const size_t i) const {
  if (!hasLine(name))
    return "";

  auto lines = m_lines.at(name);

  return lines[i];
}

QString TSVSerialiser::lineAsQString(const std::string &name,
                                     const size_t i) const {
  return QString::fromStdString(lineAsString(name, i));
}

bool TSVSerialiser::selectLine(const std::string &name, const size_t i) {
  if (!hasLine(name))
    return false;

  if (i >= m_lines[name].size())
    return false;

  m_curValues = values(name, i);
  m_curIndex = 1; // 1 because we want to start on the values, not the name
  return true;
}

bool TSVSerialiser::selectSection(const std::string &name, const size_t i) {
  if (!hasSection(name))
    return false;

  if (i >= m_sections[name].size())
    return false;

  m_curValues.clear();
  m_curValues.push_back(name);
  m_curValues.push_back(m_sections[name][i]);
  m_curIndex = 1; // 1 because we want to start on the values, not the name
  return true;
}

int TSVSerialiser::asInt(const size_t i) const {
  if (i >= m_curValues.size())
    return 0;

  std::string valStr = m_curValues.at(i);

  std::stringstream valSS(valStr);
  int ret;
  valSS >> ret;

  return ret;
}

size_t TSVSerialiser::asSize_t(const size_t i) const {
  if (i >= m_curValues.size())
    return 0;

  std::string valStr = m_curValues.at(i);

  std::stringstream valSS(valStr);
  size_t ret;
  valSS >> ret;

  return ret;
}

double TSVSerialiser::asDouble(const size_t i) const {
  if (i >= m_curValues.size())
    return 0.00;

  std::string valStr = m_curValues.at(i);

  std::stringstream valSS(valStr);
  double ret;
  valSS >> ret;

  return ret;
}

float TSVSerialiser::asFloat(const size_t i) const {
  if (i >= m_curValues.size())
    return 0.00;

  std::string valStr = m_curValues.at(i);

  std::stringstream valSS(valStr);
  float ret;
  valSS >> ret;

  return ret;
}

bool TSVSerialiser::asBool(const size_t i) const {
  if (i >= m_curValues.size())
    return false;

  std::string valStr = m_curValues.at(i);

  std::stringstream valSS(valStr);
  bool ret;
  valSS >> ret;

  return ret;
}

QRect TSVSerialiser::asQRect(const size_t i) {
  if (i + 3 >= m_curValues.size())
    return QRect();

  int x0 = asInt(m_curIndex);
  int y0 = asInt(++m_curIndex);
  int x1 = asInt(++m_curIndex);
  int y1 = asInt(++m_curIndex);
  ++m_curIndex;

  QPoint point0(x0, y0);
  QPoint point1(x1, y1);
  QRect rect(point0, point1);

  return rect;
}

QColor TSVSerialiser::asQColor(const size_t i) {
  if (i + 3 >= m_curValues.size())
    return QColor();

  int r = asInt(m_curIndex);
  int g = asInt(++m_curIndex);
  int b = asInt(++m_curIndex);
  int a = asInt(++m_curIndex);
  ++m_curIndex;

  QColor color(r, g, b, a);
  return color;
}

QPoint TSVSerialiser::asQPoint(const size_t i) {
  if (i + 1 >= m_curValues.size())
    return QPoint();

  int x = asInt(m_curIndex);
  int y = asInt(++m_curIndex);
  ++m_curIndex;

  QPoint point(x, y);
  return point;
}

QPointF TSVSerialiser::asQPointF(const size_t i) {
  if (i + 1 >= m_curValues.size())
    return QPointF();

  double x = asDouble(m_curIndex);
  double y = asDouble(++m_curIndex);
  ++m_curIndex;

  QPointF point(x, y);
  return point;
}

std::string TSVSerialiser::asString(const size_t i) const {
  if (i >= m_curValues.size())
    return "";

  return m_curValues.at(i);
}

QString TSVSerialiser::asQString(const size_t i) const {
  if (i >= m_curValues.size())
    return "";

  return QString::fromStdString(m_curValues.at(i));
}

void TSVSerialiser::storeDouble(const double val) { m_output << "\t" << val; }

void TSVSerialiser::storeInt(const int val) { m_output << "\t" << val; }
void TSVSerialiser::storeString(const std::string val) {
  m_output << "\t" << val;
}

void TSVSerialiser::storeBool(const bool val) { m_output << "\t" << val; }

double TSVSerialiser::readDouble() { return asDouble(m_curIndex++); }

int TSVSerialiser::readInt() { return asInt(m_curIndex++); }

std::string TSVSerialiser::readString() { return asString(m_curIndex++); }

bool TSVSerialiser::readBool() { return asBool(m_curIndex++); }

TSVSerialiser &TSVSerialiser::operator>>(int &val) {
  val = asInt(m_curIndex++);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator>>(size_t &val) {
  val = asSize_t(m_curIndex++);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator>>(double &val) {
  val = asDouble(m_curIndex++);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator>>(float &val) {
  val = asFloat(m_curIndex++);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator>>(std::string &val) {
  val = asString(m_curIndex++);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator>>(QString &val) {
  val = QString::fromUtf8(asString(m_curIndex++).c_str());
  return *this;
}

TSVSerialiser &TSVSerialiser::operator>>(bool &val) {
  val = asBool(m_curIndex++);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator>>(QRect &val) {
  val = asQRect(m_curIndex);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator>>(QColor &val) {
  val = asQColor(m_curIndex);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator>>(QPoint &val) {
  val = asQPoint(m_curIndex);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator>>(QPointF &val) {
  val = asQPointF(m_curIndex);
  return *this;
}

TSVSerialiser &TSVSerialiser::writeLine(const std::string &name) {
  // If we're not on a new line, make one
  if (m_midLine) {
    m_output << "\n";
  }

  m_output << name;

  m_midLine = true;
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const std::string &val) {
  storeString(val);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const char *val) {
  m_output << "\t" << std::string(val);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const QString &val) {
  const std::string str = val.toUtf8().constData();
  m_output << "\t" << str;
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const double &val) {
  storeDouble(val);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const int &val) {
  storeInt(val);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const size_t &val) {
  m_output << "\t" << val;
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const bool &val) {
  storeBool(val);
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const QRect &val) {
  auto point0 = val.topLeft();
  auto point1 = val.bottomRight();
  m_output << "\t" << point0.x() << "\t" << point0.y() << "\t" << point1.x()
           << "\t" << point1.y();
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const QColor &val) {

  m_output << "\t" << val.red() << "\t" << val.green() << "\t" << val.blue()
           << "\t" << val.alpha();
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const QPoint &val) {

  m_output << "\t" << val.x() << "\t" << val.y();
  return *this;
}

TSVSerialiser &TSVSerialiser::operator<<(const QPointF &val) {

  m_output << "\t" << val.x() << "\t" << val.y();
  return *this;
}

void TSVSerialiser::writeRaw(const std::string &raw) {
  if (m_midLine) {
    m_output << "\n";
    m_midLine = false;
  }

  m_output << raw;

  // If raw didn't end in a newline, make a note of it.
  m_midLine = (raw.length() > 0 && raw[raw.length() - 1] != '\n');
}

void TSVSerialiser::writeSection(const std::string &name,
                                 const std::string &body) {
  // If we're not on a new line, make one
  if (m_midLine) {
    m_output << "\n";
    m_midLine = false;
  }

  m_output << "<" << name << ">"
           << "\n";
  m_output << body;

  // If body isn't blank and didn't end with a new line, add one.
  if (body.length() > 0 && body[body.length() - 1] != '\n')
    m_output << "\n";

  m_output << "</" << name << ">"
           << "\n";
}

void TSVSerialiser::writeInlineSection(const std::string &name,
                                       const std::string &body) {
  // If we're not on a new line, make one
  if (m_midLine) {
    m_output << "\n";
    m_midLine = false;
  }

  m_output << "<" << name << ">";
  m_output << body;
  m_output << "</" << name << ">"
           << "\n";
}

std::string TSVSerialiser::outputLines() const {
  std::string output = m_output.str();
  if (m_midLine)
    output += "\n";

  return output;
}

void TSVSerialiser::clear() {
  m_sections.clear();
  m_lines.clear();
  m_curValues.clear();
  m_curIndex = 0;
  m_output.clear();
  m_midLine = false;
}
