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
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiFileValidator.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyHelper.h"
#include "MantidKernel/VectorHelper.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <numeric>
#include <regex>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("MultipleFileProperty");

bool doesNotContainWildCard(const std::string &ext) { return std::string::npos == ext.find('*'); }

static const std::string SUCCESS("");

// Regular expressions for any adjacent + or , operators
static const std::regex REGEX_INVALID(R"(\+\+|,,|\+,|,\+)");

// Comma/plus operators that act as token separators: any char on the left,
// non-digit on the right (after optional whitespace). The digit→digit case
// is left for the run-number list parser to handle. The original Boost
// patterns used left-side lookbehinds that std::regex does not support, but
// the alternation digit-or-non-digit on the left is equivalent to "any
// preceding char" and so can be dropped.
static const std::regex REGEX_COMMA_OPERATORS(R"(\s*,\s*(?=\D))");
static const std::regex REGEX_PLUS_OPERATORS(R"(\s*\+\s*(?=\D))");

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

  // Use a temporary single FileProperty to do the job for us using this name
  FileProperty singleFileProperty(this->name(), "", FileProperty::Load, m_exts, Direction::Input);

  std::string error = singleFileProperty.setValue(propValue);

  if (!error.empty())
    return error;

  // Store.
  std::vector<std::vector<std::string>> foundFiles;
  try {
    toValue(singleFileProperty(), foundFiles, "", "");
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
  // Empty input (for optional properties — required ones are rejected upstream
  // in setValue) means "no files selected". Short-circuit so we don't generate
  // a spurious empty hint that would later fail file resolution. Boost's
  // sregex_token_iterator used to silently yield zero tokens here; std::regex
  // yields one empty token, hence the explicit guard.
  if (propValue.empty()) {
    PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(std::vector<std::vector<std::string>>{});
    m_oldPropValue = propValue;
    m_oldFoundValue.clear();
    return SUCCESS;
  }

  // if value is unchanged use the cached version
  if ((propValue == m_oldPropValue) && (!m_oldFoundValue.empty())) {
    PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(m_oldFoundValue);
    return SUCCESS;
  }

  // Return error if there are any adjacent + or , operators.
  std::smatch invalid_substring;
  if (!m_allowEmptyTokens && std::regex_search(propValue, invalid_substring, REGEX_INVALID))
    return "Unable to parse filename due to an empty token.";
  if (!isASCII(propValue))
    return "Unable to parse filename due to an unsupported non-ASCII character being found.";

  std::vector<std::vector<std::string>> fileNames;

  // Tokenise on allowed comma operators, and iterate over each token.
  std::sregex_token_iterator end;
  std::sregex_token_iterator commaToken(propValue.begin(), propValue.end(), REGEX_COMMA_OPERATORS, -1);

  for (; commaToken != end; ++commaToken) {
    const std::string commaTokenString = commaToken->str();

    // Tokenise on allowed plus operators.
    std::sregex_token_iterator plusToken(commaTokenString.begin(), commaTokenString.end(), REGEX_PLUS_OPERATORS, -1);

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
      std::filesystem::path path(unresolvedFileName);
      if (path.has_extension()) {
        defaultExt = path.extension().string();
        break;
      }

    } catch (const std::exception &) {
      // Safe to ignore?  Need a better understanding of the circumstances under
      // which this throws.
    }
  }

  // Cycle through each vector of unresolvedFileNames in allUnresolvedFileNames.
  // Remember, each vector contains files that are to be added together.
  for (const auto &unresolvedFileNames : allUnresolvedFileNames) {
    const auto hasWildCard = [](const std::string &name) { return name.find('*') != std::string::npos; };
    if (std::any_of(unresolvedFileNames.cbegin(), unresolvedFileNames.cend(), hasWildCard))
      return "Searching for files by wildcards is not currently supported.";

    // Separate files into two groups: those with explicit extensions and those
    // that need default extension resolution. resolvedFiles is sized up-front
    // and indexed by the original position so the input order survives the
    // batched resolution path below.
    std::vector<std::string> filesToResolveWithExtension;
    std::vector<size_t> resolutionIndices;
    std::vector<std::string> resolvedFiles(unresolvedFileNames.size());

    for (size_t i = 0; i < unresolvedFileNames.size(); ++i) {
      const auto &unresolvedFileName = unresolvedFileNames[i];
      bool useDefaultExt;

      try {
        // Check for an extension.
        std::filesystem::path path(unresolvedFileName);

        useDefaultExt = !path.has_extension();
      } catch (const std::exception &) {
        // Just shove the problematic filename straight into FileProperty and
        // see if we have any luck.
        useDefaultExt = false;
      }

      if (!useDefaultExt) {
        FileProperty slaveFileProp("Slave", "", FileProperty::Load, m_exts, Direction::Input);
        std::string error = slaveFileProp.setValue(unresolvedFileName);

        // If an error was returned then pass it along.
        if (!error.empty()) {
          throw std::runtime_error(error);
        }

        resolvedFiles[i] = slaveFileProp();
      } else {
        // Collect files that need extension resolution for batch processing
        filesToResolveWithExtension.emplace_back(unresolvedFileName);
        resolutionIndices.emplace_back(i);
      }
    }

    // Batch resolve files with extension using findRuns for better performance.
    // When the batch succeeds we get a single call into the archive search,
    // which lets back-ends like ONCat resolve all runs in one network round
    // trip. If the batch throws (any one file is missing) we fall back to
    // per-file findRun so the error names the actually missing file rather
    // than whichever hint findRuns happened to report first.
    if (!filesToResolveWithExtension.empty()) {
      const auto extsToUse = !defaultExt.empty() ? std::vector<std::string>(1, defaultExt) : m_exts;
      bool batchSucceeded = false;
      try {
        auto resolvedPaths = FileFinder::Instance().findRuns(filesToResolveWithExtension, extsToUse);
        for (size_t i = 0; i < resolvedPaths.size(); ++i) {
          resolvedFiles[resolutionIndices[i]] = resolvedPaths[i].string();
        }
        batchSucceeded = true;
      } catch (const Exception::NotFoundError &) {
        // Fall through to per-file resolution below.
      }

      if (!batchSucceeded) {
        for (size_t i = 0; i < filesToResolveWithExtension.size(); ++i) {
          const auto &unresolvedFileName = filesToResolveWithExtension[i];
          auto run = FileFinder::Instance().findRun(unresolvedFileName, extsToUse);
          if (run) {
            resolvedFiles[resolutionIndices[i]] = run.result().string();
            continue;
          }

          bool doThrow = !m_allowEmptyTokens;
          if (m_allowEmptyTokens) {
            try {
              if (std::stoi(unresolvedFileName) != 0)
                doThrow = true;
            } catch (std::invalid_argument &) {
              doThrow = true;
            }
          }
          if (doThrow)
            throw Exception::NotFoundError("Unable to find file:", unresolvedFileName);

          // Empty token allowed: keep the hint as the resolved value so it
          // surfaces in any downstream error message.
          resolvedFiles[resolutionIndices[i]] = unresolvedFileName;
        }
      }
    }

    allFullFileNames.emplace_back(std::move(resolvedFiles));
  }

  PropertyWithValue<std::vector<std::vector<std::string>>>::operator=(allFullFileNames);
  m_oldPropValue = propValue;
  m_oldFoundValue = std::move(allFullFileNames);
  return SUCCESS;
}

} // namespace Mantid::API
