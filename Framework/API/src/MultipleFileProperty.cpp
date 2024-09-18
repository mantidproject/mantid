// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
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

#include <algorithm>
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
bool doesNotContainWildCard(const std::string &ext) { return std::string::npos == ext.find('*'); }

static const std::string SUCCESS("");

// Regular expressions for any adjacent + or , operators
const std::string INVALID = R"(\+\+|,,|\+,|,\+)";
static const boost::regex REGEX_INVALID(INVALID);

// Regular expressions that represent the allowed instances of , operators
const std::string NUM_COMMA_ALPHA(R"((?<=\d)\s*,\s*(?=\D))");
const std::string ALPHA_COMMA_ALPHA(R"((?<=\D)\s*,\s*(?=\D))");
const std::string COMMA_OPERATORS = NUM_COMMA_ALPHA + "|" + ALPHA_COMMA_ALPHA;
static const boost::regex REGEX_COMMA_OPERATORS(COMMA_OPERATORS);

// Regular expressions that represent the allowed instances of + operators
const std::string NUM_PLUS_ALPHA(R"((?<=\d)\s*\+\s*(?=\D))");
const std::string ALPHA_PLUS_ALPHA(R"((?<=\D)\s*\+\s*(?=\D))");
const std::string PLUS_OPERATORS = NUM_PLUS_ALPHA + "|" + ALPHA_PLUS_ALPHA;
static const boost::regex REGEX_PLUS_OPERATORS(PLUS_OPERATORS, boost::regex_constants::perl);

bool isASCII(const std::string &str) {
  return !std::any_of(str.cbegin(), str.cend(), [](char c) { return static_cast<unsigned char>(c) > 127; });
}

} // anonymous namespace

namespace Mantid::API {
/**
 * Alternative constructor with action
 *
 * @param name   :: The name of the property
 * @param action :: File action
 * @param exts   ::  The allowed/suggested extensions
 * @param allowEmptyTokens :: whether to allow empty tokens
 */
MultipleFileProperty::MultipleFileProperty(const std::string &name, unsigned int action,
                                           const std::vector<std::string> &exts, bool allowEmptyTokens)
    : PropertyWithValue<std::vector<std::vector<std::string>>>(
          name, std::vector<std::vector<std::string>>(),
          std::make_shared<MultiFileValidator>(exts, (action == FileProperty::Load)), Direction::Input),
      m_allowEmptyTokens(allowEmptyTokens) {
  if (action != FileProperty::Load && action != FileProperty::OptionalLoad) {
    /// raise error for unsupported actions
    throw std::runtime_error("Specified action is not supported for MultipleFileProperty");
  } else {
    m_action = action;
  }

  m_multiFileLoadingEnabled = Kernel::ConfigService::Instance().getValue<bool>("loading.multifile").value_or(false);
  std::copy_if(exts.cbegin(), exts.cend(), std::back_inserter(m_exts), doesNotContainWildCard);
}

/**
 * Default constructor with default action
 *
 * @param name :: The name of the property
 * @param exts ::  The allowed/suggested extensions
 */
MultipleFileProperty::MultipleFileProperty(const std::string &name, const std::vector<std::string> &exts)
    : MultipleFileProperty(name, FileProperty::Load, exts) {}

/**
 * Check if this property is optional
 * @returns True if the property is optinal, false otherwise
 */
bool MultipleFileProperty::isOptional() const { return (m_action == FileProperty::OptionalLoad); }

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
  // MultiFileValidator, so isOptional needs to be inspected here as well
  if (propValue.empty() && !isOptional())
    return "No file(s) specified.";

  // If multiple file loading is disabled, then set value assuming it is a
  // single file.
  if (!m_multiFileLoadingEnabled) {
    g_log.debug("MultiFile loading is not enabled, acting as standard FileProperty.");
    return setValueAsSingleFile(propValue);
  }

  try {
    // Else try and set the value, assuming it could be one or more files.
    return setValueAsMultipleFiles(propValue);
  } catch (const std::range_error &re) {
    // it was a valid multi file string but for too many files.
    return std::string(re.what());
  } catch (const std::runtime_error &re) {
    g_log.debug("MultiFile loading has failed. Trying as standard FileProperty.");

    const std::string error = setValueAsSingleFile(propValue);

    if (error.empty())
      return SUCCESS;

    // If we failed return the error message from the multiple file load attempt
    // as the single file was a guess and probably not what the user will expect
    // to see
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
std::string MultipleFileProperty::setValueAsSingleFile(const std::string &propValue) {
  // if value is unchanged use the cached version
  if ((propValue == m_oldPropValue) && (!m_oldFoundValue.empty())) {
    PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(m_oldFoundValue);
    return SUCCESS;
  }

  // Use a slave FileProperty to do the job for us.
  FileProperty slaveFileProp("Slave", "", FileProperty::Load, m_exts, Direction::Input);

  std::string error = slaveFileProp.setValue(propValue);

  if (!error.empty())
    return error;

  // Store.
  std::vector<std::vector<std::string>> foundFiles;
  try {
    toValue(slaveFileProp(), foundFiles, "", "");
    PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(foundFiles);
  } catch (std::invalid_argument &except) {
    g_log.debug() << "Could not set property " << name() << ": " << except.what();
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
std::string MultipleFileProperty::setValueAsMultipleFiles(const std::string &propValue) {
  // if value is unchanged use the cached version
  if ((propValue == m_oldPropValue) && (!m_oldFoundValue.empty())) {
    PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(m_oldFoundValue);
    return SUCCESS;
  }

  // Return error if there are any adjacent + or , operators.
  boost::smatch invalid_substring;
  if (!m_allowEmptyTokens && boost::regex_search(propValue.begin(), propValue.end(), invalid_substring, REGEX_INVALID))
    return "Unable to parse filename due to an empty token.";
  if (!isASCII(propValue))
    return "Unable to parse filename due to an unsupported non-ASCII character being found.";

  std::vector<std::vector<std::string>> fileNames;

  // Tokenise on allowed comma operators, and iterate over each token.
  boost::sregex_token_iterator end;
  boost::sregex_token_iterator commaToken(propValue.begin(), propValue.end(), REGEX_COMMA_OPERATORS, -1);

  for (; commaToken != end; ++commaToken) {
    const std::string commaTokenString = commaToken->str();

    // Tokenise on allowed plus operators.
    boost::sregex_token_iterator plusToken(commaTokenString.begin(), commaTokenString.end(), REGEX_PLUS_OPERATORS, -1);

    std::vector<std::vector<std::string>> temp;

    // Put the tokens into a vector before iterating over it this time,
    // so we can see how many we have.
    std::vector<std::string> plusTokenStrings;
    for (; plusToken != end; ++plusToken)
      plusTokenStrings.emplace_back(plusToken->str());

    m_parser.setTrimWhiteSpaces(autoTrim()); // keep trimming whitespaces in parser consistent with this property
    for (auto &plusTokenString : plusTokenStrings) {
      try {
        m_parser.parse(plusTokenString);
      } catch (const std::range_error &re) {
        g_log.error(re.what());
        throw;
      } catch (const std::runtime_error &) {
        // We should be able to safely ignore runtime_errors from parse(),
        // see below.
      }

      std::vector<std::vector<std::string>> f = m_parser.fileNames();

      // If there are no files, then we should keep this token as it was passed
      // to the property, in its untampered form. This will enable us to deal
      // with the case where a user is trying to load a single (and possibly
      // existing) file within a token, but which has unexpected zero padding,
      // or some other anomaly.
      if (VectorHelper::flattenVector(f).empty())
        f.emplace_back(1, plusTokenString);

      if (plusTokenStrings.size() > 1) {
        // See [3] in header documentation.  Basically, for reasons of
        // ambiguity, we cant add together plusTokens if they contain a range
        // of files.  So throw on any instances of this when there is more than
        // plusToken.
        if (f.size() > 1)
          return "Adding a range of files to another file(s) is not currently "
                 "supported.";

        if (temp.empty())
          temp.emplace_back(f[0]);
        else {
          for (auto &parsedFile : f[0])
            temp[0].emplace_back(parsedFile);
        }
      } else {
        temp.insert(temp.end(), f.begin(), f.end());
      }
    }

    fileNames.insert(fileNames.end(), std::make_move_iterator(temp.begin()), std::make_move_iterator(temp.end()));
  }

  std::vector<std::vector<std::string>> allUnresolvedFileNames = fileNames;
  std::vector<std::vector<std::string>> allFullFileNames;

  // First, find the default extension.  Flatten all the unresolved filenames
  // first, to make this easier.
  std::vector<std::string> flattenedAllUnresolvedFileNames = VectorHelper::flattenVector(allUnresolvedFileNames);
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
      // which this throws.
    }
  }

  // Cycle through each vector of unresolvedFileNames in allUnresolvedFileNames.
  // Remember, each vector contains files that are to be added together.
  for (const auto &unresolvedFileNames : allUnresolvedFileNames) {
    // Check for the existance of wild cards. (Instead of iterating over all the
    // filenames just join them together and search for "*" in the result.)
    if (std::string::npos != boost::algorithm::join(unresolvedFileNames, "").find("*"))
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
        // see if we have any luck.
        useDefaultExt = false;
      }

      std::string fullyResolvedFile;

      if (!useDefaultExt) {
        FileProperty slaveFileProp("Slave", "", FileProperty::Load, m_exts, Direction::Input);
        std::string error = slaveFileProp.setValue(unresolvedFileName);

        // If an error was returned then pass it along.
        if (!error.empty()) {
          throw std::runtime_error(error);
        }

        fullyResolvedFile = slaveFileProp();
      } else {
        // If a default ext has been specified/found, then use it.
        std::string errors = "";
        if (!defaultExt.empty()) {
          auto run = FileFinder::Instance().findRun(unresolvedFileName, std::vector<std::string>(1, defaultExt));
          if (run)
            fullyResolvedFile = run.result();
          else
            errors += run.errors();

        } else {
          auto run = FileFinder::Instance().findRun(unresolvedFileName, m_exts);
          if (run)
            fullyResolvedFile = run.result();
          else
            errors += run.errors();
        }
        if (fullyResolvedFile.empty()) {
          bool doThrow = false;
          if (m_allowEmptyTokens) {
            try {
              const int unresolvedInt = std::stoi(unresolvedFileName);
              if (unresolvedInt != 0) {
                doThrow = true;
              }
            } catch (std::invalid_argument &) {
              doThrow = true;
            }
          } else {
            doThrow = true;
          }
          if (doThrow) {
            auto errorMsg = "Unable to find file matching the string \"" + unresolvedFileName +
                            "\", please check the data search directories.";
            if (!errors.empty())
              errorMsg += " " + errors;

            throw std::runtime_error(errorMsg);
          } else {
            // if the fullyResolvedFile is empty, it means it failed to find the
            // file so keep the unresolvedFileName as a hint to be displayed
            // later on in the error message
            fullyResolvedFile = unresolvedFileName;
          }
        }
      }

      // Append the file name to result.
      fullFileNames.emplace_back(std::move(fullyResolvedFile));
    }
    allFullFileNames.emplace_back(std::move(fullFileNames));
  }

  // Now re-set the value using the full paths found.
  PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(allFullFileNames);

  // cache the new version of things
  m_oldPropValue = propValue;
  m_oldFoundValue = std::move(allFullFileNames);

  return SUCCESS;
}

} // namespace Mantid::API
