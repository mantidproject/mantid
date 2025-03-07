// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/PropertyWithValue.h"

namespace Mantid {
namespace API {

#ifdef _WIN32
#pragma warning(push)
// Disable 'multiple assignment operators specified' warning for this class
//  - it's not an accident that we have more than one
#pragma warning(disable : 4522)
#endif

/**
   A specialized class for dealing with file properties. Mantid allows multiple
   search paths to be defined so that each of these is used when attempting to
   load a file with a relative path.

   When attempting to load a file this class handles searching the specified
   paths and,
   if found, the <code>value()</code> method returns the full path to the file.
   For
   saving, Mantid's default save directory is used when a relative path is
   encountered.

   This class can also be used to browse for a Directory, by specify the
   appropriate
     FileAction type parameter in the constructor.

 */
class MANTID_API_DLL FileProperty : public Kernel::PropertyWithValue<std::string> {
public:
  /// An enumeration for load/save types. This is passed on to the FileValidator
  /// as a constructor parameter.
  enum FileAction {
    // Note that the order here ensures that the correct boolean gets passed to
    // the FileValidator.
    Save = 0,         ///< to specify a file to write to, the file may or may not exist
    OptionalSave = 1, ///< to specify a file to write to but an empty string is
    /// allowed here which will be passed to the algorithm
    Load = 2,             ///< to specify a file to open for reading, the file must exist
    OptionalLoad = 3,     ///< to specify a file to read but the file doesn't have to exist
    Directory = 4,        ///< to specify a directory that must exist
    OptionalDirectory = 5 ///< to specify a directory that does not have to exist
  };

  /// Constructor taking a list of extensions as a vector
  FileProperty(const std::string &name, const std::string &defaultValue, unsigned int action,
               const std::vector<std::string> &exts = std::vector<std::string>(),
               unsigned int direction = Kernel::Direction::Input);
  /// Constructor taking a single extension as a string
  FileProperty(const std::string &name, const std::string &default_value, unsigned int action, const std::string &ext,
               unsigned int direction = Kernel::Direction::Input);
  /// Constructor taking a list of extensions as an initializer_list
  FileProperty(const std::string &name, const std::string &default_value, unsigned int action,
               std::initializer_list<std::string> exts, unsigned int direction = Kernel::Direction::Input);

  FileProperty(const FileProperty &) = default;
  FileProperty &operator=(const FileProperty &) = default;

  /// 'Virtual copy constructor
  FileProperty *clone() const override { return new FileProperty(*this); }

  /// Check if this is a load type property.
  bool isLoadProperty() const;
  /// Check if this is a save type property.
  bool isSaveProperty() const;
  /// Check if this is a directory type property.
  bool isDirectoryProperty() const;
  /// Check if this property is optional
  bool isOptional() const;
  /// Overridden setValue method
  std::string setValue(const std::string &propValue) override;
  /// Returns an empty string if the property is valid, otherwise contains an
  /// error message
  std::string isValid() const override;

  /// Returns the main file extension that's used
  const std::string &getDefaultExt() const { return m_defaultExt; }

  // Unhide the PropertyWithValue assignment operator
  using Kernel::PropertyWithValue<std::string>::operator=;

private:
  /// Returns a string depending on whether an empty value is valid
  std::string isEmptyValueValid() const;
  /// Do the allowed values match the facility preference extensions for run
  /// files
  bool extsMatchRunFiles();
  /// Handles the filename if this is a save property
  std::string setLoadProperty(const std::string &propValue);
  /// Handles the filename if this is a save property
  std::string setSaveProperty(const std::string &propValue);
  /// The action type of this property, i.e. load/save
  unsigned int m_action;
  /// The default file extension associated with the type of file this property
  /// will handle
  std::string m_defaultExt;
  /// Is this property for run files?
  bool m_runFileProp;
  /// Last value of propValue used in FileProperty::setLoadProperty
  std::string m_oldLoadPropValue;
  /// Last value of foundFile used in FileProperty::setLoadProperty
  std::string m_oldLoadFoundFile;
};

#ifdef _WIN32
#pragma warning(pop) // Re-enable the warning about multiple assignment operators
#endif
} // namespace API
} // namespace Mantid
