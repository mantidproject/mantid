// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Glob.h"
#include "MantidKernel/Strings.h"
#include <Poco/Glob.h>

namespace Mantid::Kernel {

/**
 *    Creates a set of files that match the given pathPattern.
 *
 *    The path may be give in either Unix, Windows or VMS syntax and
 *    is automatically expanded by calling Path::expand().
 *
 *    The pattern may contain wildcard expressions even in intermediate
 *    directory names (e.g. /usr/include/<I>*</I> /<I>*</I>*.h).
 *
 *    Note that, for obvious reasons, escaping characters in a pattern
 *    with a backslash does not work in Windows-style paths.
 *
 *    Directories that for whatever reason cannot be traversed are
 *    ignored.
 *
 *    It seems that whatever bug Poco had is fixed now.
 *    So calling Poco::Glob::glob(pathPattern,files,options) inside.
 *
 *    @param pathPattern :: The search pattern as a string
 *    @param files :: The names of the files that match the pattern
 *    @param options :: Options
 */
void Glob::glob(const std::string &pathPattern, std::set<std::string> &files, int options) {
#ifdef _WIN32
  // There appears to be a bug in the glob for windows.
  // Putting case sensitive on then with reference to test
  // testFindFileCaseSensitive()
  // in FileFinderTest on Windows it is able to find "CSp78173.Raw" as it should
  // even
  // the case is wrong, but for some strange reason it then cannot find
  // IDF_for_UNiT_TESTiNG.xMl!!!!
  // Hence the reason to circumvent this by this #ifdef
  Poco::Glob::glob(pathPattern, files, Poco::Glob::GLOB_CASELESS);
#else
  Poco::Glob::glob(pathPattern, files, options);
#endif
}

std::string Glob::globToRegex(const std::string &globPattern) {
  const std::string STAR("*");
  const std::string STAR_ESCAPED("\\*");
  const std::string QUESTION("?");
  const std::string QUESTION_ESCAPED("\\?");

  const std::string REGEX_MATCH_GLOB(".+");
  const std::string REGEX_MATCH_CHAR(".");
  const std::string REGEX_START_STR("^");
  const std::string REGEX_END_STR("$");

  // this is a variant of Strings::replace and Strings::replaceAll
  std::string output = globPattern;

  // replace stars
  std::string::size_type pos = 0;
  while ((pos = output.find(STAR, pos)) != std::string::npos) {
    if ((pos > 0) && (pos + 1 != std::string::npos)) {
      if (output.substr(pos - 1, STAR_ESCAPED.length()) == STAR_ESCAPED) {
        pos += 1;
        continue;
      }
    }
    output.erase(pos, STAR.length());
    output.insert(pos, REGEX_MATCH_GLOB);
    pos += REGEX_MATCH_GLOB.length();
  }

  // now with question marks
  pos = 0;
  while ((pos = output.find(QUESTION, pos)) != std::string::npos) {
    if ((pos > 0) && (pos + 1 != std::string::npos)) {
      if (output.substr(pos - 1, QUESTION_ESCAPED.length()) == QUESTION_ESCAPED) {
        pos += 1;
        continue;
      }
    }
    output.erase(pos, QUESTION.length());
    output.insert(pos, REGEX_MATCH_CHAR);
    pos += REGEX_MATCH_CHAR.length();
  }

  // now [! and don't worry about escaping
  pos = 0;
  while ((pos = output.find("[!", pos)) != std::string::npos) {
    output.erase(pos, std::string("[!").length());
    output.insert(pos, "![");
    pos += std::string("![").length();
  }

  // replaced string with anchors
  return REGEX_START_STR + output + REGEX_END_STR;
  // return output;
}

} // namespace Mantid::Kernel
