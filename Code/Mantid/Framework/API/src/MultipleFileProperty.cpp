
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/MultipleFileProperty.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MultiFileValidator.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/System.h"

#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <ctype.h>
#include <functional>
#include <numeric>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace // anonymous
    {
/// static logger
Mantid::Kernel::Logger g_log("MultipleFileProperty");

/**
 * Unary predicate for use with copy_if.  Checks for the existance of
 * a "*" wild card in the file extension string passed to it.
 */
bool doesNotContainWildCard(const std::string &ext) {
  if (std::string::npos != ext.find("*"))
    return false;
  return true;
}
} // anonymous namespace

namespace Mantid {
namespace API {
/**
 * Constructor
 *
 * @param name :: The name of the property
 * @param exts ::  The allowed/suggested extensions
 */
MultipleFileProperty::MultipleFileProperty(const std::string &name,
                                           const std::vector<std::string> &exts)
    : PropertyWithValue<std::vector<std::vector<std::string>>>(
          name, std::vector<std::vector<std::string>>(),
          boost::make_shared<MultiFileValidator>(exts), Direction::Input),
      m_multiFileLoadingEnabled(), m_exts(), m_parser(), m_defaultExt("") {
  std::string allowMultiFileLoading =
      Kernel::ConfigService::Instance().getString("loading.multifile");

  if (boost::iequals(allowMultiFileLoading, "On"))
    m_multiFileLoadingEnabled = true;
  else
    m_multiFileLoadingEnabled = false;

  for (auto ext = exts.begin(); ext != exts.end(); ++ext)
    if (doesNotContainWildCard(*ext))
      m_exts.push_back(*ext);
}

/**
 * Destructor
 */
MultipleFileProperty::~MultipleFileProperty() {}

/**
 * Convert the given propValue into a comma and plus separated list of full
 *filenames, and pass to the parent's
 * setValue method to store as a vector of vector of strings.
 *
 * READ HEADER FILE DOCUMENTATION FOR A MORE DETAILED OVERVIEW.
 *
 * @param propValue :: A string of the allowed format, indicating the user's
 *choice of files.
 * @return A string indicating the outcome of the attempt to set the property.
 *An empty string indicates success.
 */
std::string MultipleFileProperty::setValue(const std::string &propValue) {
  // No empty value is allowed.
  if (propValue.empty())
    return "No file(s) specified.";

  // If multiple file loading is disabled, then set value assuming it is a
  // single file.
  if (!m_multiFileLoadingEnabled) {
    g_log.debug(
        "MultiFile loading is not enabled, acting as standard FileProperty.");
    return setValueAsSingleFile(propValue);
  }

  try {
    // Else try and set the value, assuming it could be one or more files.
    return setValueAsMultipleFiles(propValue);
  } catch (const std::range_error &re) {
    // it was a valid multi file string but for too many files.
    return std::string(re.what());
  } catch (const std::runtime_error &re) {
    g_log.debug(
        "MultiFile loading has failed. Trying as standard FileProperty.");

    const std::string error = setValueAsSingleFile(propValue);

    if (error.empty())
      return "";

    // If we failed return the error message from the multiple file load attempt
    // as the single file was a guess
    // and probably not what the user will expect to see
    return re.what();
  }
}

std::string MultipleFileProperty::value() const {
  if (!m_multiFileLoadingEnabled)
    return toString(m_value, "", "");

  return toString(m_value);
}

/**
 * Get the value the property was initialised with -its default value
 * @return The default value
 */
std::string MultipleFileProperty::getDefault() const {
  if (!m_multiFileLoadingEnabled)
    return toString(m_initialValue, "", "");

  return toString(m_initialValue);
}

/**
 * A convenience function for the cases where we dont use the MultiFileProperty
 *to
 * *add* workspaces - only to list them.  It "flattens" the given vector of
 *vectors
 * into a single vector which is much easier to traverse.  For example:
 *
 * ((1), (2), (30), (31), (32), (100), (102)) becomes (1, 2, 30, 31, 32, 100,
 *102)
 *
 * Used on a vector of vectors that *has* added filenames, the following
 *behaviour is observed:
 *
 * ((1), (2), (30, 31, 32), (100), (102)) becomes (1, 2, 30, 31, 32, 100, 102)
 *
 * @param fileNames :: a vector of vectors, containing all the file names.
 * @return a single vector containing all the file names.
 */
std::vector<std::string> MultipleFileProperty::flattenFileNames(
    const std::vector<std::vector<std::string>> &fileNames) {
  std::vector<std::string> flattenedFileNames;

  std::vector<std::vector<std::string>>::const_iterator it = fileNames.begin();

  for (; it != fileNames.end(); ++it) {
    flattenedFileNames.insert(flattenedFileNames.end(), it->begin(), it->end());
  }

  return flattenedFileNames;
}

/**
 * Called by setValue in the case where a user has disabled multiple file
 *loading.
 *
 * @param propValue :: A string of the allowed format, indicating the user's
 *choice of files.
 * @return A string indicating the outcome of the attempt to set the property.
 *An empty string indicates success.
 */
std::string
MultipleFileProperty::setValueAsSingleFile(const std::string &propValue) {
  // Use a slave FileProperty to do the job for us.
  FileProperty slaveFileProp("Slave", "", FileProperty::Load, m_exts,
                             Direction::Input);

  std::string error = slaveFileProp.setValue(propValue);

  if (!error.empty())
    return error;

  // Store.
  try {
    std::vector<std::vector<std::string>> result;
    toValue(slaveFileProp(), result, "", "");
    PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(result);
  } catch (std::invalid_argument &except) {
    g_log.debug() << "Could not set property " << name() << ": "
                  << except.what();
    return except.what();
  }
  return "";
}

/**
 * Called by setValue in the case where multiple file loading is enabled.
 *
 * NOTE: If multifile loading is enabled, then users make the concession that
 *they cannot use "," or "+" in
 *       directory names; they are used as operators only.
 *
 * @param propValue :: A string of the allowed format, indicating the user's
 *choice of files.
 * @return A string indicating the outcome of the attempt to set the property.
 *An empty string indicates success.
 */
std::string
MultipleFileProperty::setValueAsMultipleFiles(const std::string &propValue) {
  // Return error if there are any adjacent + or , operators.
  const std::string INVALID = "\\+\\+|,,|\\+,|,\\+";
  boost::smatch invalid_substring;
  if (boost::regex_search(propValue.begin(), propValue.end(), invalid_substring,
                          boost::regex(INVALID)))
    return "Unable to parse filename due to an empty token.";

  // Regular expressions that represent the allowed instances of + or ,
  // operators.
  const std::string NUM_COMMA_ALPHA = "(?<=\\d)\\s*,\\s*(?=\\D)";
  const std::string ALPHA_COMMA_ALPHA = "(?<=\\D)\\s*,\\s*(?=\\D)";
  const std::string NUM_PLUS_ALPHA = "(?<=\\d)\\s*\\+\\s*(?=\\D)";
  const std::string ALPHA_PLUS_ALPHA = "(?<=\\D)\\s*\\+\\s*(?=\\D)";
  const std::string COMMA_OPERATORS = NUM_COMMA_ALPHA + "|" + ALPHA_COMMA_ALPHA;
  const std::string PLUS_OPERATORS = NUM_PLUS_ALPHA + "|" + ALPHA_PLUS_ALPHA;

  std::stringstream errorMsg;
  std::vector<std::vector<std::string>> fileNames;

  // Tokenise on allowed comma operators, and iterate over each token.
  boost::sregex_token_iterator end;
  boost::sregex_token_iterator commaToken(propValue.begin(), propValue.end(),
                                          boost::regex(COMMA_OPERATORS), -1);

  for (; commaToken != end; ++commaToken) {
    const std::string commaTokenString = commaToken->str();

    // Tokenise on allowed plus operators.
    boost::sregex_token_iterator plusToken(
        commaTokenString.begin(), commaTokenString.end(),
        boost::regex(PLUS_OPERATORS, boost::regex_constants::perl), -1);

    std::vector<std::vector<std::string>> temp;

    // Put the tokens into a vector before iterating over it this time,
    // so we can see how many we have.
    std::vector<std::string> plusTokenStrings;
    for (; plusToken != end; ++plusToken)
      plusTokenStrings.push_back(plusToken->str());

    for (auto plusTokenString = plusTokenStrings.begin();
         plusTokenString != plusTokenStrings.end(); ++plusTokenString) {
      try {
        m_parser.parse(*plusTokenString);
      } catch (const std::range_error &re) {
        g_log.error(re.what());
        throw;
      } catch (const std::runtime_error &re) {
        errorMsg << "Unable to parse run(s): \"" << re.what();
      }

      std::vector<std::vector<std::string>> f = m_parser.fileNames();

      // If there are no files, then we should keep this token as it was passed
      // to the property,
      // in its untampered form. This will enable us to deal with the case where
      // a user is trying to
      // load a single (and possibly existing) file within a token, but which
      // has unexpected zero
      // padding, or some other anomaly.
      if (flattenFileNames(f).size() == 0)
        f.push_back(std::vector<std::string>(1, *plusTokenString));

      if (plusTokenStrings.size() > 1) {
        // See [3] in header documentation.  Basically, for reasons of
        // ambiguity, we cant add
        // together plusTokens if they contain a range of files.  So throw on
        // any instances of this
        // when there is more than plusToken.
        if (f.size() > 1)
          return "Adding a range of files to another file(s) is not currently "
                 "supported.";

        if (temp.empty())
          temp.push_back(f[0]);
        else {
          for (auto parsedFile = f[0].begin(); parsedFile != f[0].end();
               ++parsedFile)
            temp[0].push_back(*parsedFile);
        }
      } else {
        temp.insert(temp.end(), f.begin(), f.end());
      }
    }

    fileNames.insert(fileNames.end(), temp.begin(), temp.end());
  }

  std::vector<std::vector<std::string>> allUnresolvedFileNames = fileNames;
  std::vector<std::vector<std::string>> allFullFileNames;

  // First, find the default extension.  Flatten all the unresolved filenames
  // first, to make this easier.
  std::vector<std::string> flattenedAllUnresolvedFileNames =
      flattenFileNames(allUnresolvedFileNames);
  std::string defaultExt = "";
  auto unresolvedFileName = flattenedAllUnresolvedFileNames.begin();
  for (; unresolvedFileName != flattenedAllUnresolvedFileNames.end();
       ++unresolvedFileName) {
    try {
      // Check for an extension.
      Poco::Path path(*unresolvedFileName);
      if (!path.getExtension().empty()) {
        defaultExt = "." + path.getExtension();
        break;
      }

    } catch (Poco::Exception &) {
      // Safe to ignore?  Need a better understanding of the circumstances under
      // which
      // this throws.
    }
  }

  // Cycle through each vector of unresolvedFileNames in allUnresolvedFileNames.
  // Remember, each vector contains files that are to be added together.
  auto unresolvedFileNames = allUnresolvedFileNames.begin();
  for (; unresolvedFileNames != allUnresolvedFileNames.end();
       ++unresolvedFileNames) {
    // Check for the existance of wild cards. (Instead of iterating over all the
    // filenames just join them together
    // and search for "*" in the result.)
    if (std::string::npos !=
        boost::algorithm::join(*unresolvedFileNames, "").find("*"))
      return "Searching for files by wildcards is not currently supported.";

    std::vector<std::string> fullFileNames;

    for (auto unresolvedFileName = unresolvedFileNames->begin();
         unresolvedFileName != unresolvedFileNames->end();
         ++unresolvedFileName) {
      bool useDefaultExt;

      try {
        // Check for an extension.
        Poco::Path path(*unresolvedFileName);

        if (path.getExtension().empty())
          useDefaultExt = true;
        else
          useDefaultExt = false;
      } catch (Poco::Exception &) {
        // Just shove the problematic filename straight into FileProperty and
        // see
        // if we have any luck.
        useDefaultExt = false;
      }

      std::string fullyResolvedFile = "";

      if (!useDefaultExt) {
        FileProperty slaveFileProp("Slave", "", FileProperty::Load, m_exts,
                                   Direction::Input);
        std::string error = slaveFileProp.setValue(*unresolvedFileName);

        // If an error was returned then pass it along.
        if (!error.empty()) {
          throw std::runtime_error(error);
        }

        fullyResolvedFile = slaveFileProp();
      } else {
        // If a default ext has been specified/found, then use it.
        if (!defaultExt.empty()) {
          fullyResolvedFile = FileFinder::Instance().findRun(
              *unresolvedFileName, std::vector<std::string>(1, defaultExt));
        } else {
          fullyResolvedFile =
              FileFinder::Instance().findRun(*unresolvedFileName, m_exts);
        }
        if (fullyResolvedFile.empty())
          throw std::runtime_error(
              "Unable to find file matching the string \"" +
              *unresolvedFileName +
              "\", even after appending suggested file extensions.");
      }

      // Append the file name to result.
      fullFileNames.push_back(fullyResolvedFile);
    }

    allFullFileNames.push_back(fullFileNames);
  }

  // Now re-set the value using the full paths found.
  return PropertyWithValue<std::vector<std::vector<std::string>>>::setValue(
      toString(allFullFileNames));
}

} // namespace Mantid
} // namespace API
