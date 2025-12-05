// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FileProperty.h"

#include "MantidAPI/FileFinder.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DirectoryValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Strings.h"

#include <filesystem>

#include <iterator>
#include <memory>

namespace Mantid::API {

using Mantid::Kernel::ConfigService;
using Mantid::Kernel::DirectoryValidator;
using Mantid::Kernel::FileValidator;
using Mantid::Kernel::IValidator_sptr;

namespace {
/**
 * Create the appropriate validator based on the parameters
 * @param action The type of property that is being defined, @see FileAction
 * @param exts A list of extensions, only use for File-type actions and are
 *             passed to the validator
 */
IValidator_sptr createValidator(unsigned int action, const std::vector<std::string> &exts) {
  if (action == FileProperty::Directory || action == FileProperty::OptionalDirectory) {
    return std::make_shared<DirectoryValidator>(action == FileProperty::Directory);
  } else {
    return std::make_shared<FileValidator>(exts, (action == FileProperty::Load));
  }
}

/**
 * If the given extension doesn't exist in the list then add it
 * @param extension A string listing the extension
 * @param extensions The existing collection
 */
void addExtension(const std::string &extension, std::vector<std::string> &extensions) {
  if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end())
    return;
  else
    extensions.emplace_back(extension);
}

/**
 * Get the path to the user's home directory (associated with ~) if it is set
 * as an environment variable, and cache it
 * @return The user's home path
 */
const std::string &getHomePath() {
  static std::string homePath;
  static bool initialised(false);

  if (initialised) {
    return homePath;
  }
  initialised = true;

  char *home = std::getenv("HOME"); // Usually set on Windows and UNIX
  if (home) {
    homePath = std::string(home);
    return homePath;
  }

  char *userProfile = std::getenv("USERPROFILE"); // Not usually set on UNIX
  // Return even if it's an empty string, as we can do no better
  homePath = userProfile ? std::string(userProfile) : "";
  return homePath;
}

/** Expand user variables in file path.
 *  On Windows and UNIX, ~ is replaced by the user's home directory, if found.
 *  If the path contains no user variables, or expansion fails, the path is
 *  returned unchanged, for errors to be dealt with by the calling function.
 *  Note: this function does not support the "~user/blah" format for a named
 *  user's home directory - if this is encountered, the filepath is returned
 *  unchanged.
 *  @param filepath The path to expand
 *  @return The expanded path
 */
std::string expandUser(const std::string &filepath) {
  auto start = filepath.begin();
  auto end = filepath.end();

  // Filepath empty or contains no user variables
  if (start == end || *start != '~')
    return filepath;

  // Position of the first slash after the variable
  auto nextSlash = find_if(start, end, [](const char &c) { return c == '/' || c == '\\'; });

  // ~user/blah format - no support for this as yet
  if (std::distance(start, nextSlash) != 1)
    return filepath;

  return getHomePath() + std::string(nextSlash, end);
}

/**
 * Create a given directory if it does not already exist.
 * @param path :: The path to the directory, which can include file stem
 * @returns A string indicating a problem if one occurred
 */
std::string createDirectory(const std::string &path) {
  std::filesystem::path stempath(path);
  // If the path doesn't end with a separator, assume it includes a filename and get the parent directory
  if (!path.empty() && path.back() != std::filesystem::path::preferred_separator) {
    stempath = stempath.parent_path();
  }

  if (!stempath.empty()) {
    if (!std::filesystem::exists(stempath)) {
      try {
        std::filesystem::create_directories(stempath);
      } catch (const std::exception &e) {
        std::stringstream msg;
        msg << "Failed to create directory \"" << stempath.string() << "\": " << e.what();
        return msg.str();
      }
    }
  } else {
    return "Invalid directory.";
  }
  return ""; // everything went fine
}
} // Anonymous namespace

//-----------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------
/**
 * Constructor
 * @param name The name of the property
 * @param defaultValue A default value for the property
 * @param action Inndicate whether this should be a load/save
 * property
 * @param exts The allowed extensions. The front entry in the vector
 * will be the default extension
 * @param direction An optional direction (default=Input)
 */
FileProperty::FileProperty(const std::string &name, const std::string &defaultValue, unsigned int action,
                           const std::vector<std::string> &exts, unsigned int direction)
    : PropertyWithValue<std::string>(name, defaultValue, createValidator(action, exts), direction), m_action(action),
      m_defaultExt((!exts.empty()) ? exts.front() : ""), m_runFileProp(isLoadProperty() && extsMatchRunFiles()),
      m_oldLoadPropValue(""), m_oldLoadFoundFile("") {}

/**
 * Constructor
 * @param name ::          The name of the property
 * @param default_value :: A default value for the property
 * @param ext ::           The allowed extension
 * @param action ::        An enum indicating whether this should be a load/save
 * property
 * @param direction ::     An optional direction (default=Input)
 */
FileProperty::FileProperty(const std::string &name, const std::string &default_value, unsigned int action,
                           const std::string &ext, unsigned int direction)
    : FileProperty(name, default_value, action, std::vector<std::string>(1, ext), direction) {}

/**
 * Constructor
 * @param name ::          The name of the property
 * @param default_value :: A default value for the property
 * @param exts ::          The braced-list of allowed extensions
 * @param action ::        An enum indicating whether this should be a load/save
 * property
 * @param direction ::     An optional direction (default=Input)
 */
FileProperty::FileProperty(const std::string &name, const std::string &default_value, unsigned int action,
                           std::initializer_list<std::string> exts, unsigned int direction)
    : FileProperty(name, default_value, action, std::vector<std::string>(exts), direction) {}

/**
 * Check if this is a load property
 * @returns True if the property is a Load property and false otherwise
 */
bool FileProperty::isLoadProperty() const { return m_action == Load || m_action == OptionalLoad; }

/**
 * Check if this is a Save property
 * @returns True if the property is a Save property and false otherwise
 */
bool FileProperty::isSaveProperty() const { return m_action == Save || m_action == OptionalSave; }

/**
 * Check if this is a directory selection property
 * @returns True if the property is a Directory property
 */
bool FileProperty::isDirectoryProperty() const { return m_action == Directory || m_action == OptionalDirectory; }

/**
 * Check if this property is optional
 * @returns True if the property is optinal, false otherwise
 */
bool FileProperty::isOptional() const {
  return (m_action == OptionalLoad || m_action == OptionalSave || m_action == OptionalDirectory);
}

/**
 * Set the value of the property
 * @param propValue :: The value here is treated as relating to a filename
 * @return A string indicating the outcome of the attempt to set the property.
 * An empty string indicates success.
 */
std::string FileProperty::setValue(const std::string &propValue) {
  std::string strippedValue = Kernel::Strings::strip(propValue);

  // Empty value is allowed if optional
  if (strippedValue.empty()) {
    PropertyWithValue<std::string>::setValue("");
    return isEmptyValueValid();
  }

  // Expand user variables, if there are any
  strippedValue = expandUser(strippedValue);

  // If this looks like an absolute path then don't do any searching but make
  // sure the
  // directory exists for a Save property
  if (std::filesystem::path(strippedValue).is_absolute()) {
    if (isSaveProperty()) {
      std::string error = createDirectory(strippedValue);
      if (!error.empty())
        return error;
    }

    return PropertyWithValue<std::string>::setValue(strippedValue);
  }

  std::string errorMsg;
  // For relative paths, differentiate between load and save types
  if (isLoadProperty()) {
    errorMsg = setLoadProperty(strippedValue);
  } else {
    errorMsg = setSaveProperty(strippedValue);
  }
  return errorMsg;
}

/**
 * Checks whether the current value is considered valid. Use the validator
 * unless the
 * value is an empty string. In this case it is only valid if the property is
 * not optional
 * @returns an empty string if the property is valid, otherwise contains an
 * error message
 */
std::string FileProperty::isValid() const {
  const std::string &value = (*this)();
  if (value.empty()) {
    return isEmptyValueValid();
  } else {
    return PropertyWithValue<std::string>::isValid();
  }
}

/**
 * @returns a string depending on whether an empty value is valid
 */
std::string FileProperty::isEmptyValueValid() const {
  if (isOptional()) {
    return "";
  } else {
    return "No file specified.";
  }
}

/**
 * Do the allowed values match the facility preference extensions for run files
 * @returns True if the extensions match those in the facility's preference list
 * for
 * run file extensions, false otherwise
 */
bool FileProperty::extsMatchRunFiles() {
  bool match(false);
  try {
    Kernel::FacilityInfo facilityInfo = Kernel::ConfigService::Instance().getFacility();
    const std::vector<std::string> facilityExts = facilityInfo.extensions();
    const std::vector<std::string> allowedExts = this->allowedValues();
    match = std::any_of(allowedExts.cbegin(), allowedExts.cend(), [&facilityExts](const auto &ext) {
      return std::find(facilityExts.cbegin(), facilityExts.cend(), ext) != facilityExts.cend();
    });

  } catch (Mantid::Kernel::Exception::NotFoundError &) {
  } // facility could not be found, do nothing this will return the default
  // match of false

  return match;
}

/**
 * Handles the filename if this is a load property
 * @param propValue :: The filename to treat as a filepath to be loaded
 * @returns A string contain the result of the operation, empty if successful.
 */
std::string FileProperty::setLoadProperty(const std::string &propValue) {
  // determine the initial version of foundFile
  std::string foundFile;
  if ((propValue == m_oldLoadPropValue) && (!m_oldLoadFoundFile.empty())) {
    foundFile = m_oldLoadFoundFile;
  }

  // cache the new version of propValue
  m_oldLoadPropValue = propValue;

  // if foundFile is not empty then it is the cached file
  if (foundFile.empty()) {
    if (m_runFileProp) // runfiles go through FileFinder::findRun
    {
      std::vector<std::string> allowedExts(allowedValues());
      std::vector<std::string> exts;
      if (!m_defaultExt.empty()) {
        addExtension(m_defaultExt, exts);

        std::string lower = Mantid::Kernel::Strings::toLower(m_defaultExt);
        addExtension(lower, exts);

        std::string upper = Mantid::Kernel::Strings::toUpper(m_defaultExt);
        addExtension(upper, exts);
      }
      for (auto &ext : allowedExts) {
        std::string lower(ext);
        std::string upper(ext);
        std::transform(ext.begin(), ext.end(), lower.begin(), tolower);
        std::transform(ext.begin(), ext.end(), upper.begin(), toupper);
        addExtension(ext, exts);
        addExtension(lower, exts);
        addExtension(upper, exts);
      }
      foundFile = FileFinder::Instance().findRun(propValue, exts).result();
    } else // non-runfiles go through FileFinder::getFullPath
    {
      foundFile = FileFinder::Instance().getFullPath(propValue);
    }
  }

  // cache the new version of foundFile
  m_oldLoadFoundFile = foundFile;

  if (foundFile.empty()) {
    return PropertyWithValue<std::string>::setValue(propValue);
  } else {
    return PropertyWithValue<std::string>::setValue(foundFile);
  }
}

/**
 * Handles the filename if this is a save property
 * @param propValue :: The filename to treat as a filepath to be saved
 * @returns A string contain the result of the operation, empty if successful.
 */
std::string FileProperty::setSaveProperty(const std::string &propValue) {
  if (propValue.empty()) {
    if (m_action == OptionalSave) {
      return PropertyWithValue<std::string>::setValue("");
    } else
      return "Empty filename not allowed.";
  }
  std::string errorMsg;
  // We have a relative save path so just prepend the path that is in the
  // 'defaultsave.directory'
  // Note that this catches exceptions and returns an empty string in that case
  std::string save_path = ConfigService::Instance().getString("defaultsave.directory");
  std::filesystem::path save_dir;
  if (save_path.empty()) {
    save_dir = std::filesystem::path(propValue).parent_path();
    // If we only have a stem filename, parent_path() will make save_dir empty
    if (save_dir.empty()) {
      save_dir = std::filesystem::current_path();
    }
  } else {
    save_dir = std::filesystem::path(save_path);
  }
  errorMsg = createDirectory(save_dir.string());
  if (errorMsg.empty()) {
    std::string fullpath = (save_dir / propValue).string();
    errorMsg = PropertyWithValue<std::string>::setValue(fullpath);
  }
  return errorMsg;
}
} // namespace Mantid::API
