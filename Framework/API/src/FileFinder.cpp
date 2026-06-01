// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IArchiveSearch.h"
#include "MantidAPI/ISISInstrumentDataCache.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/MultiFileNameParser.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"

#include <boost/lexical_cast.hpp>
#include <json/value.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <regex>

namespace {
Mantid::Kernel::Logger g_log("FileFinder");

bool containsWildCard(const std::string &ext) { return std::string::npos != ext.find('*'); }

std::string toUpper(const std::string &src) {
  std::string out(src);
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}

std::string toLower(const std::string &src) {
  std::string out(src);
  std::transform(out.begin(), out.end(), out.begin(), ::tolower);
  return out;
}

// Commas that act as separators between tokens: any character followed by a
// comma followed by a non-digit. Digit→digit commas (e.g. "15196,15197") are
// left for the MultiFileNameParsing::Parser to handle as run-number lists.
// Mirrors MultipleFileProperty's REGEX_COMMA_OPERATORS without the redundant
// left-side lookbehind (which std::regex does not support).
const std::regex COMMA_OPERATORS(R"(\s*,\s*(?=\D))");

bool isASCII(const std::string &str) {
  return !std::any_of(str.cbegin(), str.cend(), [](char c) { return static_cast<unsigned char>(c) > 127; });
}

/// Split an input string on comma operators (those where the comma separates
/// distinct tokens rather than appearing inside a digit-list like "15196,15197").
std::vector<std::string> splitOnCommaOperators(const std::string &input) {
  std::vector<std::string> tokens;
  std::sregex_token_iterator end;
  std::sregex_token_iterator it(input.begin(), input.end(), COMMA_OPERATORS, -1);
  for (; it != end; ++it)
    tokens.emplace_back(it->str());
  return tokens;
}

/// Expand a single comma-token into one or more file hints by feeding it
/// through the multi-file-name parser.
///
/// A range/list token like "CNCS10-15" or "CNCS10-15.nxs.h5" expands to one
/// entry per run. A single-file token that already carries an extension is
/// returned literally (the parser would otherwise apply instrument-prefix
/// zero-padding that may not match the user's on-disk filename).
///
/// Any exception from the parser is propagated so the caller can decide how to
/// handle a token the parser could not interpret: a too-large range surfaces a
/// std::range_error, while a malformed/unrecognised token surfaces some other
/// std::exception. The caller distinguishes a genuine literal hint (e.g. a
/// "-add" group) from a malformed run range.
std::vector<std::string> expandHint(const std::string &token, Mantid::Kernel::MultiFileNameParsing::Parser &parser) {
  if (token.empty())
    return {};

  parser.parse(token);

  std::vector<std::string> expanded;
  for (const auto &group : parser.fileNames())
    expanded.insert(expanded.end(), group.cbegin(), group.cend());

  // Parser produced nothing — fall back to the literal token.
  if (expanded.empty())
    return {token};

  // Single-file token with an explicit extension: prefer the user's exact
  // filename, as the parser may have applied zero-padding that won't match
  // the on-disk filename. Ranges/lists (size > 1) keep the parser's output
  // since expansion was the whole point of the token.
  if (expanded.size() == 1 && std::filesystem::path(token).has_extension())
    return {token};

  return expanded;
}

} // namespace

namespace Mantid::API {

// this allowed string could be made into an array of allowed, currently used
// only by the ISIS SANS group
const std::string FileFinderImpl::ALLOWED_SUFFIX = "-add";

FileFinderImpl::FileFinderImpl() {
  // Make sure plugins are loaded
  FrameworkManager::Instance().loadPlugins();

// determine from Mantid property how sensitive Mantid should be
#ifdef _WIN32
  m_globOption = Kernel::Glob::GLOB_DEFAULT;
#else
  setCaseSensitive(Kernel::ConfigService::Instance().getValue<bool>("filefinder.casesensitive").value_or(false));
#endif
}

/**
 * Option to set if file finder should be case sensitive
 * @param cs :: If true then set to case sensitive
 */
void FileFinderImpl::setCaseSensitive(const bool cs) {
  if (cs)
    m_globOption = Kernel::Glob::GLOB_DEFAULT;
  else
    m_globOption = Kernel::Glob::GLOB_CASELESS;
}

/**
 * Option to get if file finder should be case sensitive
 * @return cs :: If case sensitive return true, if not case sensitive return
 * false
 */
bool FileFinderImpl::getCaseSensitive() const { return (m_globOption == Kernel::Glob::GLOB_DEFAULT); }

/**
 * Return the full path to the file given its name
 * @param filename :: A file name (without path) including extension
 * @param ignoreDirs :: If true, directories that match are skipped unless the
 * path given is already absolute
 * @return The full path if the file exists and can be found in one of the
 * search locations
 *  or an empty string otherwise.
 */

std::filesystem::path FileFinderImpl::getFullPath(const std::string &filename, const bool ignoreDirs) const {
  return Kernel::ConfigService::Instance().getFullPath(filename, ignoreDirs, m_globOption);
}

/** Run numbers can be followed by an allowed string. Check if there is
 *  one, remove it from the name and return the string, else return empty
 *  @param userString run number that may have a suffix
 *  @return the suffix, if there was one
 */
std::string FileFinderImpl::extractAllowedSuffix(std::string &userString) const {
  if (userString.find(ALLOWED_SUFFIX) == std::string::npos) {
    // short cut processing as normally there is no suffix
    return "";
  }

  // ignore any file extension in checking if a suffix is present
  std::filesystem::path entry(userString);
  std::string noExt(entry.stem().string());
  const size_t repNumChars = ALLOWED_SUFFIX.size();
  if (noExt.find(ALLOWED_SUFFIX) == noExt.size() - repNumChars) {
    userString.replace(userString.size() - repNumChars, repNumChars, "");
    return ALLOWED_SUFFIX;
  }
  return "";
}

/**
 * Return the InstrumentInfo as determined from the hint.
 *
 * @param hintstr :: The name hint.
 * @param returnDefaultIfNotFound :: Flag to control return. May throw exception if set to false.
 * @param defaultInstrument :: The default instrument to return if not found. If empty, the system default is used.
 * @return The InstrumentInfo object.
 */
const Kernel::InstrumentInfo FileFinderImpl::getInstrument(const std::string &hintstr,
                                                           const bool returnDefaultIfNotFound,
                                                           const std::string &defaultInstrument) const {
  if ((!hintstr.empty()) && (!isdigit(hintstr[0]))) {
    // If hint contains path components, use only the filename part for instrument detection
    std::string filename = toUpper(std::filesystem::path(hintstr).filename().string());

    try {
      std::string instrName = Kernel::ConfigService::Instance().findLongestInstrumentPrefix(filename);

      // if still empty, throw not found
      if (instrName.empty()) {
        throw Kernel::Exception::NotFoundError("Instrument not found", hintstr);
      }

      return Kernel::ConfigService::Instance().getInstrument(instrName);
    } catch (Kernel::Exception::NotFoundError &e) {
      g_log.debug() << e.what() << "\n";
      if (!returnDefaultIfNotFound)
        throw;
    }
  }
  return Kernel::ConfigService::Instance().getInstrument(defaultInstrument);
}

/**
 * Extracts the instrument name and run number from a hint
 * @param hintstr :: The name hint
 * @param defaultInstrument :: The default instrument to use if the hint does not contain an instrument name. If empty,
 * the system default is used.
 * @return A pair of instrument name and run number
 */
std::pair<std::string, std::string> FileFinderImpl::toInstrumentAndNumber(const std::string &hintstr,
                                                                          const std::string &defaultInstrument) const {
  Kernel::InstrumentInfo instr = this->getInstrument(hintstr, true, defaultInstrument);
  return toInstrumentAndNumber(hintstr, instr);
}

/**
 * Extracts the instrument name and run number from a hint
 * @param hintstr :: The name hint
 * @param instr :: The instrument for the file
 * @return A pair of instrument name and run number
 */
std::pair<std::string, std::string> FileFinderImpl::toInstrumentAndNumber(const std::string &hintstr,
                                                                          const Kernel::InstrumentInfo &instr) const {
  g_log.debug() << "toInstrumentAndNumber(" << hintstr << ")\n";
  std::string runPart;

  if (hintstr.empty()) {
    throw std::invalid_argument("Malformed hint: empty hint");
  }

  if (isdigit(hintstr[0])) {
    runPart = hintstr;
  } else {
    const auto hintUpper = toUpper(hintstr);
    std::string instrPart = instr.name();
    if (!hintUpper.starts_with(instrPart)) {
      instrPart = instr.shortName();
      if (!hintUpper.starts_with(instrPart)) {
        throw std::invalid_argument("Malformed hint: does not start with instrument name or short name");
      }
    }

    // need to advance to the first digit after the instrument name to handle underscores, etc.
    size_t nChars = instrPart.length();
    while (nChars < hintstr.size() && !std::isdigit(static_cast<unsigned char>(hintstr[nChars])))
      ++nChars;
    if (nChars == hintstr.size())
      throw std::invalid_argument("Malformed hint: no run number found");
    runPart = hintstr.substr(nChars);
  }

  unsigned int irunPart(0);
  try {
    irunPart = boost::lexical_cast<unsigned int>(runPart);
  } catch (boost::bad_lexical_cast &) {
    std::ostringstream os;
    os << "Cannot convert '" << runPart << "' to run number.";
    throw std::invalid_argument(os.str());
  }
  size_t nZero = instr.zeroPadding(irunPart);
  // remove any leading zeros in case there are too many of them
  std::string::size_type i = runPart.find_first_not_of('0');
  runPart.erase(0, i);
  while (runPart.size() < nZero)
    runPart.insert(0, "0");
  if (runPart.size() > nZero && nZero != 0) {
    throw std::invalid_argument("Run number does not match instrument's zero padding");
  }

  return std::make_pair(instr.filePrefix(irunPart), runPart);
}

/**
 * Make a data file name (without extension) from a hint. The hint can be either
 * a run number or
 * a run number prefixed with an instrument name/short name. If the instrument
 * name is absent the default one is used.
 * @param hintstr :: The name hint
 * @param instrument :: The current instrument object
 * @return The file name
 * @throw NotFoundError if a required default is not set
 * @throw std::invalid_argument if the argument is malformed or run number is
 * too long
 */
std::string FileFinderImpl::makeFileName(const std::string &hintstr, const Kernel::InstrumentInfo &instrument) const {
  if (hintstr.empty())
    return "";

  Kernel::InstrumentInfo instrToUse = instrument;
  if (!isdigit(hintstr[0])) {
    try {
      std::string hintUpper = toUpper(hintstr);
      std::string shortName = toUpper(instrument.shortName());
      std::string name = toUpper(instrument.name());
      if (hintUpper.rfind(shortName, 0) != 0 && hintUpper.rfind(name, 0) != 0) {
        instrToUse = getInstrument(hintstr, false);
      }
    } catch (const std::exception &ex) {
      g_log.debug() << "Failed to resolve instrument from hint '" << hintstr << "' in makeFileName: " << ex.what();
    }
  }

  std::string filename(hintstr);
  const std::string suffix = extractAllowedSuffix(filename);
  const std::string shortName = instrToUse.shortName();
  std::string delimiter = instrToUse.delimiter();

  // see if starts with the provided instrument name
  if (filename.substr(0, shortName.size()) == shortName) {
    filename = filename.substr(shortName.size());
    if ((!delimiter.empty()) && (filename.substr(0, delimiter.size()) == delimiter))
      filename = filename.substr(delimiter.size());

    filename = shortName + filename;
  }

  auto [instrumentName, runNumber] = toInstrumentAndNumber(filename, instrToUse);

  // delimiter and suffix might be empty strings
  filename = instrumentName + delimiter + runNumber + suffix;
  return filename;
}

/**
 * Determine the extension from a filename.
 *
 * @param filename The filename to get the extension from.
 * @param exts The list of extensions to try before giving up and
 * using the default: whatever happens after the '.'.
 *
 * @return The extension. If one isn't determined it is an empty string.
 */
std::string FileFinderImpl::getExtension(const std::string &filename, const std::vector<std::string> &exts) const {
  g_log.debug() << "getExtension(" << filename << ", exts[" << exts.size() << "])\n";

  // go through the list of supplied extensions
  for (const auto &ext : exts) {
    std::string extension = toUpper(ext);
    if (extension.rfind('*') == extension.size() - 1) // there is a wildcard at play
    {
      extension.resize(extension.rfind('*'));
    }

    std::size_t found = toUpper(filename).rfind(extension);
    if (found != std::string::npos) {
      g_log.debug() << "matched extension \"" << extension << "\" based on \"" << ext << "\"\n";
      return filename.substr(found); // grab the actual extensions found
    }
  }

  g_log.debug() << "Failed to find extension. Just using last \'.\'\n";
  std::size_t pos = filename.find_last_of('.');
  if (pos != std::string::npos) {
    return filename.substr(pos);
  }

  // couldn't find an extension
  return "";
}

namespace {
/// Decide whether the archive search should be enabled for `facilityName`,
/// given the user's `datasearch.searcharchive` configuration value.
/// The setting accepts: "off"/empty (never), "all" (every facility),
/// "on" (only the default facility), or a substring-matched facility list.
/// All comparisons are case-insensitive (caller passes pre-lowered strings).
bool shouldUseArchiveForFacility(const std::string &archiveOpt, const std::string &facilityName) {
  if (archiveOpt == "all")
    return true;
  if (archiveOpt == "on")
    return facilityName == toLower(Mantid::Kernel::ConfigService::Instance().getString("default.facility"));
  return archiveOpt.find(facilityName) != std::string::npos;
}
} // namespace

std::vector<IArchiveSearch_sptr> FileFinderImpl::getArchiveSearch(const Kernel::FacilityInfo &facility) {
  const auto archiveOpt = toLower(Kernel::ConfigService::Instance().getString("datasearch.searcharchive"));
  if (archiveOpt.empty() || archiveOpt == "off" || facility.archiveSearch().empty())
    return {};
  if (!shouldUseArchiveForFacility(archiveOpt, toLower(facility.name())))
    return {};

  std::vector<IArchiveSearch_sptr> archs;
  archs.reserve(facility.archiveSearch().size());
  for (const auto &name : facility.archiveSearch()) {
    g_log.debug() << "get archive search for the facility..." << name << "\n";
    archs.emplace_back(ArchiveSearchFactory::Instance().create(name));
  }
  return archs;
}

/**
 * Find a path to a single file from a hint.
 * @param hintstr :: hint string to look for filename.
 * @param extensionsProvided :: Vector of aditional file extensions to consider. Optional.
 *                  If not provided, facility extensions used.
 * @param useOnlyExtensionsProvided :: Optional bool.
 *                  If it's true (and extensionsProvided is not empty),
 *                  search for the file using extensionsProvided only.
 *                  If it's false, use extensionsProvided AND facility extensions.
 * @return The full path to the file if found, or an empty string otherwise.
 */
const API::Result<std::filesystem::path> FileFinderImpl::findRun(const std::string &hintstr,
                                                                 const std::vector<std::string> &extensionsProvided,
                                                                 const bool useOnlyExtensionsProvided) const {

  auto const error = validateRuns(hintstr);
  if (!error.empty())
    return API::Result<std::filesystem::path>(std::filesystem::path(), error);

  std::string hintsStr = Kernel::Strings::strip(hintstr);
  auto filePath = tryResolvePathWithExtension(hintsStr);
  auto fileInfo = FileInfo{.hint = hintsStr,
                           .found = !filePath.empty(),
                           .path = filePath,
                           .instr = std::make_shared<Kernel::InstrumentInfo>(this->getInstrument(hintsStr, true))};

  std::vector<FileInfo> fileInfos{fileInfo};
  processFileInfos(fileInfos, extensionsProvided, useOnlyExtensionsProvided);
  const auto &resolvedFileInfo = fileInfos[0];

  if (resolvedFileInfo.found)
    return API::Result<std::filesystem::path>(resolvedFileInfo.path);

  g_log.debug() << "Failed to find file for hint: " << hintstr << "\n";
  if (resolvedFileInfo.error)
    g_log.debug() << "Error message: " << resolvedFileInfo.errorMsg << "\n";
  return API::Result<std::filesystem::path>(
      std::filesystem::path(), resolvedFileInfo.errorMsg.empty() ? "Not found." : resolvedFileInfo.errorMsg);
}

/**
 * Given a set of already determined extensions and new extensions,
 * create a set of all extensions.
 * If not in an extension-is-case-sensitive environment, only add the
 * lower case OR upper case version of the extension
 * @param extensionsToAdd :: a vector of extensions to add
 * @param uniqueExts :: a vector of currently included extensions
 */
void FileFinderImpl::getUniqueExtensions(const std::vector<std::string> &extensionsToAdd,
                                         std::vector<std::string> &uniqueExts) const {
  const bool isCaseSensitive = getCaseSensitive();
  for (const auto &ext : extensionsToAdd) {
    const auto normalized = isCaseSensitive ? ext : toLower(ext);
    if (std::find(uniqueExts.begin(), uniqueExts.end(), normalized) == uniqueExts.end())
      uniqueExts.emplace_back(normalized);
  }
}

/**
 * Performs validation on the search text entered into the File Finder. It will
 * return an error message if a problem is found.
 * @param searchText :: The text to validate.
 * @return An error message if something is invalid.
 */
std::string FileFinderImpl::validateRuns(const std::string &searchText) const {
  if (!isASCII(searchText))
    return "An unsupported non-ASCII character was found in the search text.";
  return "";
}

/**
 * Decide whether a token the multi-file parser could not expand is a malformed
 * run range rather than a genuine literal file hint.
 *
 * This mirrors the historical "fileSuspected" heuristic: a token containing a
 * path separator or the "-add" suffix, or one carrying a file extension, is
 * treated as a literal file and passed through unchanged. Any other token that
 * looks like a run range (i.e. splitting on '-' yields two or more parts, e.g.
 * "MUSR15189-n15193" or "1-2-3") but that the parser could not expand is
 * reported as malformed, restoring the old "Malformed range of runs" diagnostic
 * so the user can rule out a genuine file-not-found. Note this is only consulted
 * after the parser has already rejected the token, so well-formed ranges (which
 * the parser expands successfully) never reach here.
 *
 * @param token :: A single, already comma-split hint token.
 * @return true if the token should be reported as a malformed run range.
 */
bool FileFinderImpl::isMalformedRange(const std::string &token) const {
  if (token.find('/') != std::string::npos || token.find('\\') != std::string::npos ||
      token.find(ALLOWED_SUFFIX) != std::string::npos || std::filesystem::path(token).has_extension())
    return false;

  const Kernel::StringTokenizer parts(token, "-",
                                      Kernel::StringTokenizer::TOK_TRIM | Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  return parts.count() >= 2;
}

/**
 * Find a list of files from a comma- and range-separated hint string.
 *
 * The hint is split on comma operators, then each token is expanded by the
 * multi-file-name parser (which understands ranges like "123-135" or
 * "123:135:2", instrument prefixes, and zero padding). The resulting flat
 * list of file hints is resolved via findRuns(vector).
 *
 * @param hintstr :: Comma-separated hint string. Only the first token of a
 *                   range may carry an instrument name.
 * @param extensionsProvided :: Optional file extensions. If empty the
 *                              facility extensions are used.
 * @param useOnlyExtensionsProvided :: If true, search using only
 *                                     extensionsProvided; otherwise combine
 *                                     with facility extensions.
 * @return One full path per resolved file, in input order.
 * @throw std::invalid_argument if the input fails ASCII validation, if a
 *        token expands into a range too large for the parser to load, or if a
 *        token looks like a malformed run range (see isMalformedRange). Tokens
 *        the parser does not understand but that look like literal files are
 *        passed through as literal hints and surface as Exception::NotFoundError
 *        if they don't exist on disk.
 * @throw Exception::NotFoundError if any (literal or expanded) file cannot
 *        be found.
 */
std::vector<std::filesystem::path> FileFinderImpl::findRuns(const std::string &hintstr,
                                                            const std::vector<std::string> &extensionsProvided,
                                                            const bool useOnlyExtensionsProvided) const {
  auto const error = validateRuns(hintstr);
  if (!error.empty())
    throw std::invalid_argument(error);

  // Pre-split on comma operators (any-char→non-digit commas), mirroring
  // MultipleFileProperty. Digit→digit commas (e.g. "INST15196,15197") stay
  // intact so the parser can expand them as run-number lists.
  const auto tokens = splitOnCommaOperators(Kernel::Strings::strip(hintstr));

  Kernel::MultiFileNameParsing::Parser parser;
  parser.setTrimWhiteSpaces(true);

  std::vector<std::string> hints;
  for (const auto &token : tokens) {
    try {
      const auto expanded = expandHint(token, parser);
      hints.insert(hints.end(), expanded.cbegin(), expanded.cend());
    } catch (const std::range_error &re) {
      // The parser refused this range as too large. Surface as invalid_argument
      // so callers see a validation error rather than a NotFoundError.
      throw std::invalid_argument(re.what());
    } catch (const std::exception &) {
      // The parser could not interpret the token. If it looks like a malformed
      // run range, report that explicitly (so the user can rule out a genuine
      // file-not-found); otherwise pass it through as a literal file hint to be
      // resolved, or surfaced as a NotFoundError, downstream.
      if (isMalformedRange(token))
        throw std::invalid_argument("Malformed range of runs: " + token);
      hints.push_back(token);
    }
  }

  return findRuns(hints, extensionsProvided, useOnlyExtensionsProvided);
}

void FileFinderImpl::prepareFileInfo(FileInfo &fileInfo, const std::vector<std::string> &extensionsProvided,
                                     bool useOnlyExtensionsProvided) const {
  if (fileInfo.found || fileInfo.error)
    return;

  g_log.debug() << "  " << fileInfo.hint << " instrument: " << (fileInfo.instr ? fileInfo.instr->name() : "null")
                << "\n";

  const Kernel::FacilityInfo &facility = fileInfo.instr->facility();
  const std::vector<std::string> facilityExtensions = facility.extensions();

  // NB: std::filesystem::path::extension() only returns the *final* extension,
  // so a hint like "INST_123.nxs.h5" is split into filename "INST_123.nxs" and
  // extension ".h5". This matches how such double-extension files are searched
  // for on disk (the leading ".nxs" stays part of the stem).
  std::filesystem::path filePath(fileInfo.hint);
  const auto extension = filePath.extension();
  std::string filename = filePath.replace_extension().string();

  if (filePath.parent_path().empty()) {
    try {
      if (!facility.noFilePrefix()) {
        filename = makeFileName(filename, *fileInfo.instr);
      }
    } catch (std::invalid_argument &) {
      if (filename.length() >= fileInfo.hint.length()) {
        g_log.information() << "Could not form filename from standard rules '" << filename << "'\n";
      }
    }
  }

  if (filename.empty()) {
    g_log.warning() << "Unable to determine filename for hint '" << fileInfo.hint << "\n";
    fileInfo.error = true;
    fileInfo.errorMsg = "Unable to determine filename from hint.";
    return;
  }

  g_log.debug() << "filename to search for: " << filename << " with extension: " << extension << "\n";

  // Look first at the original filename then for case variations. This is important
  // on platforms where file names ARE case sensitive.
  fileInfo.filenames.insert(filename);
  if (!getCaseSensitive()) {
    fileInfo.filenames.insert(toUpper(filename));
    fileInfo.filenames.insert(toLower(filename));
  }

  // Merge the extensions & throw out duplicates
  // On Windows throw out ones that only vary in case
  fileInfo.extensionsToSearch.reserve(1 + extensionsProvided.size() + facilityExtensions.size());

  if (useOnlyExtensionsProvided) {
    getUniqueExtensions(extensionsProvided, fileInfo.extensionsToSearch);
  } else {
    // Search the hint's own extension first (highest priority), then the
    // provided and facility extensions.
    if (!extension.empty())
      fileInfo.extensionsToSearch.emplace_back(extension.string());

    getUniqueExtensions(extensionsProvided, fileInfo.extensionsToSearch);
    getUniqueExtensions(facilityExtensions, fileInfo.extensionsToSearch);
  }

  fileInfo.archs = getArchiveSearch(facility);
}

void FileFinderImpl::processFileInfos(std::vector<FileInfo> &fileInfos,
                                      const std::vector<std::string> &extensionsProvided,
                                      bool useOnlyExtensionsProvided) const {
  for (auto &fileInfo : fileInfos)
    prepareFileInfo(fileInfo, extensionsProvided, useOnlyExtensionsProvided);

  performFileSearch(fileInfos);
  performCacheSearch(fileInfos);
  performArchiveSearch(fileInfos);
}

std::vector<std::filesystem::path> FileFinderImpl::findRuns(const std::vector<std::string> &hints,
                                                            const std::vector<std::string> &extensionsProvided,
                                                            const bool useOnlyExtensionsProvided) const {
  if (hints.empty())
    return {};

  for (const auto &hint : hints) {
    auto const error = validateRuns(hint);
    if (!error.empty())
      throw std::invalid_argument(error);
  }

  std::vector<FileInfo> fileInfos;
  fileInfos.reserve(hints.size());
  std::shared_ptr<Kernel::InstrumentInfo> cachedInstr;
  for (const auto &hint : hints) {
    auto filePath = tryResolvePathWithExtension(hint);
    if (!filePath.empty()) {
      fileInfos.emplace_back(FileInfo{.hint = hint, .found = true, .path = filePath});
      continue;
    }
    cachedInstr = std::make_shared<Kernel::InstrumentInfo>(
        this->getInstrument(hint, true, cachedInstr ? cachedInstr->shortName() : std::string()));
    fileInfos.emplace_back(FileInfo{.hint = hint, .instr = cachedInstr});
  }

  processFileInfos(fileInfos, extensionsProvided, useOnlyExtensionsProvided);

  std::vector<std::filesystem::path> res;
  res.reserve(fileInfos.size());
  for (const auto &fileInfo : fileInfos) {
    if (!fileInfo.found) {
      if (fileInfo.error)
        g_log.warning() << "Error while searching for '" << fileInfo.hint << "': " << fileInfo.errorMsg << "\n";
      else
        g_log.warning() << "Failed to find file for hint '" << fileInfo.hint << "'\n";
      throw Kernel::Exception::NotFoundError("Unable to find file:", fileInfo.hint);
    }
    if (fileInfo.error) {
      g_log.debug() << "Non-fatal error during search for '" << fileInfo.hint << "': " << fileInfo.errorMsg << "\n";
    }
    res.emplace_back(fileInfo.path);
  }
  return res;
}

void FileFinderImpl::performFileSearch(std::vector<FileInfo> &fileInfos) const {
  // Before we try any globbing, make sure we exhaust all reasonable attempts at
  // constructing the possible filename.
  // Avoiding the globbing of getFullPath() for as long as possible will help
  // performance when calling findRuns()
  // with a large range of files, especially when searchPaths consists of
  // folders containing a large number of runs.

  const std::vector<std::string> &searchPaths = Kernel::ConfigService::Instance().getDataSearchDirs();

  for (auto &fileInfo : fileInfos) {
    if (fileInfo.found || fileInfo.error)
      continue;

    std::vector<std::string> extensions;
    extensions.assign(fileInfo.extensionsToSearch.begin(), fileInfo.extensionsToSearch.end());

    // Remove wild cards.
    extensions.erase(std::remove_if(extensions.begin(), extensions.end(), containsWildCard), extensions.end());

    // Use the std::error_code overloads of exists() so a single unreadable
    // search path (permission denied, dangling symlink, dead network mount)
    // doesn't abort the search for every other path.
    std::error_code ec;
    for (const auto &extension : extensions) {
      for (const auto &filename : fileInfo.filenames) {
        for (const auto &searchPath : searchPaths) {
          const auto filePath = std::filesystem::path(searchPath) / (filename + extension);
          if (std::filesystem::exists(filePath, ec)) {
            fileInfo.found = true;
            fileInfo.path = filePath;
            break;
          }
        }
        if (fileInfo.found)
          break;
      }
      if (fileInfo.found)
        break;
    }

    if (!fileInfo.found)
      for (const auto &extension : extensions) {
        for (const auto &filename : fileInfo.filenames) {
          const auto filepath = getFullPath(filename + extension);
          if (!filepath.empty() && std::filesystem::exists(filepath, ec)) {
            g_log.debug() << "path returned from getFullPath() = " << filepath << '\n';
            fileInfo.found = true;
            fileInfo.path = filepath;
            break;
          }
        }
        if (fileInfo.found)
          break;
      }
  }
}

void FileFinderImpl::performCacheSearch(std::vector<FileInfo> &fileInfos) const {
  // Search data cache
  std::filesystem::path cachePathToSearch(Kernel::ConfigService::Instance().getString("datacachesearch.directory"));
  // Only expect to find path to data cache on IDAaaS
  if (std::filesystem::exists(cachePathToSearch)) {
    for (auto &fileInfo : fileInfos) {
      if (fileInfo.found || fileInfo.error)
        continue;

      auto cacheFilePath =
          getISISInstrumentDataCachePath(cachePathToSearch, fileInfo.filenames, fileInfo.extensionsToSearch);

      if (cacheFilePath) {
        g_log.debug() << "Found file in data cache: " << cacheFilePath.result() << "\n";
        fileInfo.found = true;
        fileInfo.path = cacheFilePath.result();
      } else {
        fileInfo.errorMsg = cacheFilePath.errors();
      }
    }
  } else {
    g_log.debug() << "Data cache directory not found, proceeding with the search."
                  << "\n";
  }
}

/// If every unfound FileInfo has the same single archive and instrument, and
/// that archive supports batched multi-hint lookups, return it. Otherwise
/// return nullptr. Two unfound entries with equal InstrumentInfo always have
/// the same archive list because getArchiveSearch() is deterministic per
/// facility, so we only need to compare instruments rather than archive types.
IArchiveSearch_sptr FileFinderImpl::batchableArchive(const std::vector<FileInfo> &fileInfos) {
  const auto isUnfound = [](const auto &fi) { return !fi.found && !fi.error; };
  const auto first = std::find_if(fileInfos.cbegin(), fileInfos.cend(), isUnfound);
  if (first == fileInfos.cend() || first->archs.size() != 1)
    return nullptr;
  const auto &arch = first->archs[0];
  if (!arch || !arch->supportsMultipleHints())
    return nullptr;
  const Kernel::InstrumentInfo &refInstr = *first->instr;
  for (auto it = std::next(first); it != fileInfos.cend(); ++it) {
    if (!isUnfound(*it))
      continue;
    if (it->archs.size() != 1 || *it->instr != refInstr)
      return nullptr;
  }
  return arch;
}

void FileFinderImpl::performArchiveSearch(std::vector<FileInfo> &fileInfos) const {
  if (fileInfos.empty())
    return;

  if (const auto sharedArch = batchableArchive(fileInfos)) {
    performBatchedArchiveSearch(fileInfos, sharedArch);
    // The batched call may have failed outright, returned a mismatched number
    // of paths, or only resolved some of the hints. Fall back to a per-file
    // search for anything still unfound so a partial batch result doesn't doom
    // files that an individual lookup could locate. Entries already found (or
    // flagged with an error) are skipped by the per-file search.
    const auto stillUnfound =
        std::any_of(fileInfos.cbegin(), fileInfos.cend(), [](const auto &fi) { return !fi.found && !fi.error; });
    if (stillUnfound)
      performPerFileArchiveSearch(fileInfos);
  } else {
    performPerFileArchiveSearch(fileInfos);
  }
}

void FileFinderImpl::performBatchedArchiveSearch(std::vector<FileInfo> &fileInfos,
                                                 const IArchiveSearch_sptr &sharedArch) const {
  g_log.debug() << "performArchiveSearch: batching unfound hints through a single archive call\n";

  // One hint per unfound file (archive search is case-insensitive so the
  // first filename in the set is sufficient).
  std::vector<std::string> hints;
  for (const auto &fileInfo : fileInfos) {
    if (fileInfo.found || fileInfo.error)
      continue;
    hints.push_back(*fileInfo.filenames.cbegin());
  }

  const auto archivePaths = sharedArch->getArchivePaths(hints);
  if (!archivePaths) {
    g_log.error() << "Archive search failed: " << archivePaths.errors() << "\n";
    return;
  }
  if (archivePaths.result().size() != hints.size()) {
    g_log.error() << "Archive search returned a different number of paths than hints. Expected " << hints.size()
                  << " but got " << archivePaths.result().size() << ".\n";
    return;
  }

  // Walk the unfound entries in the same order the hints were collected and
  // assign each archive result back.
  const auto &paths = archivePaths.result();
  size_t index = 0;
  for (auto &fileInfo : fileInfos) {
    if (fileInfo.found || fileInfo.error)
      continue;
    const auto &archivePath = paths[index++];
    try {
      if (std::filesystem::exists(archivePath)) {
        fileInfo.found = true;
        fileInfo.path = archivePath;
      }
    } catch (std::exception &e) {
      g_log.error() << "Cannot open file " << archivePath << ": " << e.what() << '\n';
      fileInfo.error = true;
      fileInfo.errorMsg = "Cannot open file from archive: " + std::string(e.what());
    }
  }
}

void FileFinderImpl::performPerFileArchiveSearch(std::vector<FileInfo> &fileInfos) const {
  // Cache the directory and extension of the last archive hit so consecutive
  // runs in a range can be resolved with a local existence check rather than
  // another network call.
  std::filesystem::path lastFoundDir;
  std::string lastFoundExt;

  const auto tryLastFoundShortcut = [&](FileInfo &fileInfo) {
    if (lastFoundDir.empty() || lastFoundExt.empty())
      return false;
    try {
      for (const auto &filename : fileInfo.filenames) {
        const auto candidate = lastFoundDir / (filename + lastFoundExt);
        if (std::filesystem::exists(candidate)) {
          fileInfo.found = true;
          fileInfo.path = candidate;
          return true;
        }
      }
    } catch (...) {
      lastFoundDir.clear();
      lastFoundExt.clear();
    }
    return false;
  };

  for (auto &fileInfo : fileInfos) {
    if (fileInfo.found || fileInfo.error)
      continue;

    if (tryLastFoundShortcut(fileInfo)) {
      // Shortcut hit: lastFoundDir/Ext already point at this fileInfo's parent.
      continue;
    }

    if (!fileInfo.archs.empty()) {
      g_log.debug() << "Search the archives for file: " << fileInfo.hint << "\n";
      const auto archivePath = getArchivePath(fileInfo.archs, fileInfo.filenames, fileInfo.extensionsToSearch);
      if (archivePath) {
        try {
          if (std::filesystem::exists(archivePath.result())) {
            fileInfo.found = true;
            fileInfo.path = archivePath.result();
          }
        } catch (std::exception &e) {
          g_log.error() << "Cannot open file " << archivePath << ": " << e.what() << '\n';
          fileInfo.error = true;
          fileInfo.errorMsg = "Cannot open file from archive: " + std::string(e.what());
        }
      }
    }

    if (fileInfo.found) {
      lastFoundDir = fileInfo.path.parent_path();
      lastFoundExt = fileInfo.path.extension().string();
    }
  }
}

const API::Result<std::filesystem::path>
FileFinderImpl::getISISInstrumentDataCachePath(const std::filesystem::path &cacheDir,
                                               const std::set<std::string> &hintstrs,
                                               const std::vector<std::string> &exts) const {
  std::string errors;
  auto dataCache = API::ISISInstrumentDataCache(cacheDir.string());

  for (const auto &hint : hintstrs) {
    std::filesystem::path parentDir;

    try {
      parentDir = dataCache.getFileParentDirectoryPath(hint);
    } catch (const std::invalid_argument &e) {
      errors += "Data cache: " + std::string(e.what());
      return API::Result<std::filesystem::path>("", errors);
    } catch (const Json::Exception &e) {
      errors += "Data cache: Failed parsing to JSON: " + std::string(e.what()) +
                "Error likely due to accessing instrument index file while it was being updated on IDAaaS.";
      return API::Result<std::filesystem::path>("", errors);
    }

    if (!std::filesystem::exists(parentDir)) {
      errors += "Data cache: Directory not found: " + parentDir.string();
      return API::Result<std::filesystem::path>("", errors);
    }

    for (const auto &ext : exts) {
      const auto filePath = parentDir / (hint + ext);
      try { // Catches error for permission denied
        if (std::filesystem::exists(filePath)) {
          return API::Result<std::filesystem::path>(filePath);
        }
      } catch (const std::filesystem::filesystem_error &e) {
        errors += "Data cache: " + std::string(e.what());
        return API::Result<std::filesystem::path>("", errors);
      }
    }
    errors += "Data cache: " + hint + " not found in " + parentDir.string();
  }
  return API::Result<std::filesystem::path>("", errors);
}

/**
 * Return the path to the file found in archive
 * @param archs :: A list of archives to search
 * @param hintstrs :: A list of hints (without extensions) to pass to the
 * archive
 * @param exts :: A list of extensions to check for in turn against each file
 * @return The full path if the file exists and can be found in one of the
 * search locations
 *  or an empty string otherwise.
 */
const API::Result<std::filesystem::path> FileFinderImpl::getArchivePath(const std::vector<IArchiveSearch_sptr> &archs,
                                                                        const std::set<std::string> &hintstrs,
                                                                        const std::vector<std::string> &exts) const {
  g_log.debug() << "getArchivePath([IArchiveSearch_sptr], [ ";
  for (const auto &iter : hintstrs)
    g_log.debug() << iter << " ";
  g_log.debug() << "], [ ";
  for (const auto &iter : exts)
    g_log.debug() << iter << " ";
  g_log.debug() << "])\n";

  std::string errors;
  for (const auto &arch : archs) {
    try {
      g_log.debug() << "Getting archive path for requested files\n";
      auto path = arch->getArchivePath(hintstrs, exts);
      if (path)
        return path;
      else
        errors += path.errors();
    } catch (...) {
    }
  }
  return API::Result<std::filesystem::path>("", errors);
}

std::filesystem::path FileFinderImpl::tryResolvePathWithExtension(const std::string &hint) const {
  if (!std::filesystem::path(hint).has_extension())
    return {};
  // getFullPath already verifies existence and returns empty on miss.
  auto path = getFullPath(hint);
  if (!path.empty())
    g_log.debug() << "found path = " << path << '\n';
  return path;
}

} // namespace Mantid::API
