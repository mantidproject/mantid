#include "TSVSerialiser.h"

#include "MantidKernel/Logger.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <sstream>

namespace
{
  Mantid::Kernel::Logger g_log("TSVSerialiser");
}

TSVSerialiser::TSVSerialiser() : m_curIndex(0)
{
}

TSVSerialiser::TSVSerialiser(std::string lines) : m_curIndex(0)
{
  parseLines(lines);
}

void TSVSerialiser::parseLines(std::string lines)
{
  std::vector<std::string> lineVec;
  boost::split(lineVec, lines, boost::is_any_of("\n"));

  //Clear out any old data.
  m_lines.clear();
  m_sections.clear();

  boost::regex valueLineRegex("([a-zA-Z0-9]+)\\b.*");
  boost::regex closedSectionRegex("<([a-zA-Z0-9]+)>(.*)</\\1>");
  boost::regex openSectionRegex("<([a-zA-Z0-9]+)>(.*)");

  for(auto lineIt = lineVec.begin(); lineIt != lineVec.end(); ++lineIt)
  {
    std::string line = *lineIt;

    if(line.length() == 0)
      continue;

    //Stores matched sections of a regex
    boost::smatch matches;

    //Check if this is a value line
    if(boost::regex_match(line, matches, valueLineRegex))
    {
      std::string name = matches[1].str();

      m_lines[name].push_back(line);

      g_log.information() << "found value line with name '" << name << "'" << std::endl;
      continue;
    }

    //Look for lines which open and close a section in one line: <section>data</section>
    if(boost::regex_match(line, matches, closedSectionRegex))
    {
      std::string name = matches[1].str();
      std::string contents = matches[2].str();

      m_sections[name].push_back(contents);

      g_log.information() << "found closed section '" << name << "' with contents='" << contents << "'" << std::endl;
      continue;
    }

    //Check if this is the start of a multiline section, if so, consume the whole section.
    if(boost::regex_match(line, matches, openSectionRegex))
    {
      std::stringstream sectionSS;

      std::string name = matches[1].str();
      std::string firstLine = matches[2].str();

      //firstLine exists because of a legacy edgecase: the <folder> section keeps values on the same line as
      //the opening tag, so we have to be able to read that.
      if(firstLine.length() > 0)
        sectionSS << firstLine << "\n";

      std::stringstream openSS;
      openSS << "<" << name << ">.*";
      boost::regex openRegex(openSS.str());

      std::stringstream closeSS;
      closeSS << "</" << name << ">";
      boost::regex closeRegex(closeSS.str());

      //Lets iterate over the contents of the section
      auto secIt = lineIt + 1;

      //Search for opening and closing tags, counting depth and building the section string.
      for(int depth = 1; depth > 0 && secIt != lineVec.end(); ++secIt)
      {
        std::string secLine = *secIt;
        //Are we going down?
        if(boost::regex_match(secLine, openRegex))
        {
          depth++;
        } else if(boost::regex_match(secLine, closeRegex))
        {
          depth--;
        }

        if(depth > 0)
          sectionSS << secLine << "\n";
      }

      //We've now advanced beyond the end of the section so go back one
      secIt--;

      std::string sectionStr = sectionSS.str();

      //We drop the last character because it's a spare newline
      if(sectionStr.size() > 0)
        sectionStr.resize(sectionStr.size() - 1);

      m_sections[name].push_back(sectionStr);

      g_log.information() << "read <" << name << ">:\n---------------------------\n" << sectionSS.str() << "----------------------------" << std::endl;

      //Skip parsing to the end of the section
      lineIt = secIt;
      continue;
    }

    //If we've made it here then we don't know what kind of line this is.
    g_log.error() << "Unable to identify line in TSVSerialiser::parseLines(): '" << line << "'" << std::endl;
  }
}

bool TSVSerialiser::hasLine(const std::string& name) const
{
  return ( m_lines.find(name) != m_lines.end() );
}

bool TSVSerialiser::hasSection(const std::string& name) const
{
  return ( m_sections.find(name) != m_sections.end() );
}

std::vector<std::string> TSVSerialiser::values(const std::string& name, size_t i) const
{
  //Select correct line with lineAsString, parse it, then return values
  std::vector<std::string> ret;

  std::string line = lineAsString(name, i);
  boost::split(ret, line, boost::is_any_of("\t"));

  return ret;
}

std::vector<std::string> TSVSerialiser::sections(const std::string& name) const
{
  if(!hasSection(name))
    return std::vector<std::string>();

  return m_sections.at(name);
}

std::string TSVSerialiser::lineAsString(const std::string& name, const size_t i) const
{
  if(!hasLine(name))
    return "";

  auto lines = m_lines.at(name);

  return lines[i];
}

bool TSVSerialiser::selectLine(const std::string& name, const size_t i)
{
  if(!hasLine(name))
    return false;

  if(i >= m_lines[name].size())
    return false;

  m_curValues = values(name, i);
  m_curIndex = 1; //1 because we want to start on the values, not the name
  return true;
}

int TSVSerialiser::asInt(const size_t i) const
{
  if(i >= m_curValues.size())
    return 0;

  std::string valStr = m_curValues.at(i);

  std::stringstream valSS(valStr);
  int ret;
  valSS >> ret;

  return ret;
}

double TSVSerialiser::asDouble(const size_t i) const
{
  if(i >= m_curValues.size())
    return 0.00;

  std::string valStr = m_curValues.at(i);

  std::stringstream valSS(valStr);
  double ret;
  valSS >> ret;

  return ret;
}

std::string TSVSerialiser::asString(const size_t i) const
{
  if(i >= m_curValues.size())
    return "";

  return m_curValues.at(i);
}

TSVSerialiser& TSVSerialiser::operator>>(int& val)
{
  val = asInt(m_curIndex++);
  return *this;
}

TSVSerialiser& TSVSerialiser::operator>>(double& val)
{
  val = asDouble(m_curIndex++);
  return *this;
}

TSVSerialiser& TSVSerialiser::operator>>(std::string& val)
{
  val = asString(m_curIndex++);
  return *this;
}
