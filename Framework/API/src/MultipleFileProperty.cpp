
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MultiFileValidator.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyHelper.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VectorHelper.h"

#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <cctype>
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
  return std::string::npos == ext.find('*');
}

static const std::string SUCCESS("");

// Regular expressions for any adjacent + or , operators
const std::string INVALID = "\\+\\+|,,|\\+,|,\\+";
static const boost::regex REGEX_INVALID(INVALID);

// Regular expressions that represent the allowed instances of , operators
const std::string NUM_COMMA_ALPHA("(?<=\\d)\\s*,\\s*(?=\\D)");
const std::string ALPHA_COMMA_ALPHA("(?<=\\D)\\s*,\\s*(?=\\D)");
const std::string COMMA_OPERATORS = NUM_COMMA_ALPHA + "|" + ALPHA_COMMA_ALPHA;
static const boost::regex REGEX_COMMA_OPERATORS(COMMA_OPERATORS);

// Regular expressions that represent the allowed instances of + operators
const std::string NUM_PLUS_ALPHA("(?<=\\d)\\s*\\+\\s*(?=\\D)");
const std::string ALPHA_PLUS_ALPHA("(?<=\\D)\\s*\\+\\s*(?=\\D)");
const std::string PLUS_OPERATORS = NUM_PLUS_ALPHA + "|" + ALPHA_PLUS_ALPHA;
static const boost::regex REGEX_PLUS_OPERATORS(PLUS_OPERATORS,
                                               boost::regex_constants::perl);

} // anonymous namespace

namespace Mantid {
namespace API {
/**
 * Alternative constructor with action and prefix
 *
 * @param name   :: The name of the property
 * @param action :: File action
 * @param exts   :: The allowed/suggested extensions
 * @param prefix :: The prefix to prepend to the property value when set
 */
MultipleFileProperty::MultipleFileProperty(const std::string &name,
                                           unsigned int action,
                                           const std::vector<std::string> &exts,
                                           const std::string &prefix)
    : PropertyWithValue<std::vector<std::vector<std::string>>>(
          name, std::vector<std::vector<std::string>>(),
          boost::make_shared<MultiFileValidator>(
              exts, (action == FileProperty::Load)),
          Direction::Input) {
  if (action != FileProperty::Load && action != FileProperty::OptionalLoad) {
    /// raise error for unsupported actions
    throw std::runtime_error(
        "Specified action is not supported for MultipleFileProperty");
  } else {
    m_action = action;
  }
  std::string allowMultiFileLoading =
      Kernel::ConfigService::Instance().getString("loading.multifile");

  m_multiFileLoadingEnabled = boost::iequals(allowMultiFileLoading, "On");

  // Return error if there are any adjacent + or , operators in the prefix
  boost::smatch invalid_substring;
  if (boost::regex_search(prefix.begin(), prefix.end(), invalid_substring,
                          REGEX_INVALID))
    throw std::runtime_error(
        "Specified prefix '" + prefix +
        "' is invalid: contains adjacent '+' or ',' operators.");
  m_prefix = prefix;

  for (const auto &ext : exts)
    if (doesNotContainWildCard(ext))
      m_exts.push_back(ext);
}

/**
 * Alternative constructor with action and prefix but no extensions
 *
 * @param name   :: The name of the property
 * @param action :: File action
 * @param prefix :: The prefix to prepend to the property value when set
 */
MultipleFileProperty::MultipleFileProperty(const std::string &name,
                                           unsigned int action,
                                           const std::string &prefix = "")
    : MultipleFileProperty(name, action, std::vector<std::string>(), prefix) {}

/**
 * Alternative constructor with prefix, default action and no extensions
 *
 * @param name   :: The name of the property
 * @param prefix :: The prefix to prepend to the property value when set
 */
MultipleFileProperty::MultipleFileProperty(const std::string &name,
                                           const std::string &prefix = "")
    : MultipleFileProperty(name, FileProperty::Load, std::vector<std::string>(),
                           prefix) {}

/**
 * Default constructor with default action
 *
 * @param name   :: The name of the property
 * @param exts   :: The allowed/suggested extensions
 * @param prefix :: The prefix to prepend to the property value when set
 */
MultipleFileProperty::MultipleFileProperty(const std::string &name,
                                           const std::vector<std::string> &exts,
                                           const std::string &prefix)
    : MultipleFileProperty(name, FileProperty::Load, exts, prefix) {}

/**
 * Check if this property is optional
 * @returns True if the property is optinal, false otherwise
 */
bool MultipleFileProperty::isOptional() const {
  return (m_action == FileProperty::OptionalLoad);
}

/**
 * @returns Empty string if empty value is valid, error message otherwise
 */
std::string MultipleFileProperty::isEmptyValueValid() const {
  if (isOptional()) {
    return SUCCESS;
  } else {
    return "No file specified.";
  }
}

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
  // No empty value is allowed, unless optional.
  // This is yet aditional check that is beyond the underlying
  // MultiFileValidator,
  // so isOptional needs to be inspected here as well
  if (propValue.empty() && !isOptional())
    return "No file(s) specified.";

  // If multiple file loading is disabled, then set value assuming it is a
  // single file.
  if (!m_multiFileLoadingEnabled) {
    g_log.debug(
        "MultiFile loading is not enabled, acting as standard FileProperty.");
    return setValueAsSingleFile(m_prefix + propValue);
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

    const std::string error = setValueAsSingleFile(m_prefix + propValue);

    if (error.empty())
      return SUCCESS;

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
  // if value is unchanged use the cached version
  if ((propValue == m_oldPropValue) && (!m_oldFoundValue.empty())) {
    PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(
        m_oldFoundValue);
    return SUCCESS;
  }

  // Use a slave FileProperty to do the job for us.
  FileProperty slaveFileProp("Slave", "", FileProperty::Load, m_exts,
                             Direction::Input);

  std::string error = slaveFileProp.setValue(propValue);

  if (!error.empty())
    return error;

  // Store.
  std::vector<std::vector<std::string>> foundFiles;
  try {
    toValue(slaveFileProp(), foundFiles, "", "");
    PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(
        foundFiles);
  } catch (std::invalid_argument &except) {
    g_log.debug() << "Could not set property " << name() << ": "
                  << except.what();
    return except.what();
  }

  // cache the new version of things
  m_oldPropValue = propValue;
  m_oldFoundValue = std::move(foundFiles);

  return SUCCESS;
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
  // if value is unchanged use the cached version
  if ((propValue == m_oldPropValue) && (!m_oldFoundValue.empty())) {
    PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(
        m_oldFoundValue);
    return SUCCESS;
  }

  std::string errorMsg;
  try {
    m_parser.parse(m_prefix + propValue);
  } catch (const std::range_error &re) {
    g_log.error(re.what());
    throw;
  } catch (const std::runtime_error &re) {
    errorMsg = re.what();
  }
  std::vector<std::vector<std::string>> allUnresolvedFileNames =
      m_parser.fileNames();
  std::vector<std::vector<std::string>> allFullFileNames;

  // First, find the default extension.  Flatten all the unresolved filenames
  // first, to make this easier.
  std::vector<std::string> flattenedAllUnresolvedFileNames =
      VectorHelper::flattenVector(allUnresolvedFileNames);
  std::string defaultExt;
  for (const auto &unresolvedFileName : flattenedAllUnresolvedFileNames) {
    try {
      // Check for an extension.
      Poco::Path path(unresolvedFileName);
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
  for (const auto &unresolvedFileNames : allUnresolvedFileNames) {
    // Check for the existance of wild cards. (Instead of iterating over all the
    // filenames just join them together
    // and search for "*" in the result.)
    if (std::string::npos !=
        boost::algorithm::join(unresolvedFileNames, "").find("*"))
      return "Searching for files by wildcards is not currently supported.";

    std::vector<std::string> fullFileNames;

    for (const auto &unresolvedFileName : unresolvedFileNames) {
      bool useDefaultExt;

      try {
        // Check for an extension.
        Poco::Path path(unresolvedFileName);

        useDefaultExt = path.getExtension().empty();
      } catch (Poco::Exception &) {
        // Just shove the problematic filename straight into FileProperty and
        // see
        // if we have any luck.
        useDefaultExt = false;
      }

      std::string fullyResolvedFile;

      if (!useDefaultExt) {
        FileProperty slaveFileProp("Slave", "", FileProperty::Load, m_exts,
                                   Direction::Input);
        std::string error = slaveFileProp.setValue(unresolvedFileName);

        // If an error was returned then pass it along.
        if (!error.empty()) {
          throw std::runtime_error(error);
        }

        fullyResolvedFile = slaveFileProp();
      } else {
        // If a default ext has been specified/found, then use it.
        if (!defaultExt.empty()) {
          fullyResolvedFile = FileFinder::Instance().findRun(
              unresolvedFileName, std::vector<std::string>(1, defaultExt));
        } else {
          fullyResolvedFile =
              FileFinder::Instance().findRun(unresolvedFileName, m_exts);
        }
        if (fullyResolvedFile.empty())
          throw std::runtime_error(
              "Unable to find file matching the string \"" +
              unresolvedFileName +
              "\", even after appending suggested file extensions.");
      }

      // Append the file name to result.
      fullFileNames.push_back(std::move(fullyResolvedFile));
    }
    allFullFileNames.push_back(std::move(fullFileNames));
  }

  if (!allFullFileNames.empty()) {
    // Now re-set the value using the full paths found.
    PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(
        allFullFileNames);
  }

  if (errorMsg.empty()) {
    // cache the new version of things
    m_oldPropValue = propValue;
    m_oldFoundValue = std::move(allFullFileNames);
    return SUCCESS;
  } else
    return errorMsg;
}

} // namespace API
} // namespace Mantid
