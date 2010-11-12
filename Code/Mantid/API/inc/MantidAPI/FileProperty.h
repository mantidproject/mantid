#ifndef MANTID_API_FILEPROPERTY_H_
#define MANTID_API_FILEPROPERTY_H_

//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
#include "MantidKernel/PropertyWithValue.h"

namespace Mantid
{
namespace API
{
  /**
     A specialized class for dealing with file properties. Mantid allows multiple
     search paths to be defined so that each of these is used when attempting to
     load a file with a relative path. 
     
     When attempting to load a file this class handles searching the specified paths and, 
     if found, the <code>value()</code> method returns the full path to the file. For 
     saving, Mantid's default save directory is used when a relative path is encountered.

     This class can also be used to browse for a Directory, by specify the appropriate
       FileAction type parameter in the constructor.

   */
class DLLExport FileProperty : public Kernel::PropertyWithValue<std::string>
{
public:
  /// An enumeration for load/save types. This is passed on to the FileValidator as a constructor parameter.
  enum FileAction
  {
    // Note that the order here ensures that the correct boolean gets passed to the FileValidator.
    Save = 0,              ///< to specify a file to write to, the file may or may not exist
    OptionalSave = 1,      ///< to specify a file to write to but an empty string is allowed here which will be passed to the algorithm
    Load = 2,              ///< to specify a file to open for reading, the file must exist
    OptionalLoad = 3,      ///< to specify a file to read but the file doesn't have to exist
    Directory = 4,         ///< to specify a directory that must exist
    OptionalDirectory = 5  ///< to specify a directory that does not have to exist
  };

  ///Constructor
  FileProperty(const std::string & name, const std::string& default_value, unsigned int action,
	       const std::vector<std::string> & exts = std::vector<std::string>(), 
	       unsigned int direction = Kernel::Direction::Input);
  FileProperty(const std::string & name, const std::string& default_value, unsigned int action,
	       const std::string & ext, unsigned int direction = Kernel::Direction::Input);

  /// 'Virtual copy constructor
  Kernel::Property* clone() { return new FileProperty(*this); }

  /// Check if this is a load type property.
  bool isLoadProperty() const;
  /// Check if this is a save type property.
  bool isSaveProperty() const;
  /// Check if this is a directory type property.
  bool isDirectoryProperty() const;
  /// Check if this property is optional
  bool isOptional() const;
  ///Overridden setValue method
  virtual std::string setValue(const std::string & propValue);
  /// Returns the main file extension that's used 
  std::string getDefaultExt() const {return m_defaultExt;}

 private:
  void setUp(const std::string & defExt);
  /// Do the allowed values match the facility preference extensions for run files
  bool extsMatchRunFiles();
  /// Handles the filename if this is a save property
  std::string setLoadProperty(const std::string & propValue);
  /// Handles the filename if this is a save property
  std::string setSaveProperty(const std::string & propValue);
  /// Check that a given directory exists
  std::string createDirectory(const std::string & path) const;
  /// Check file extension to see if a lower- or upper-cased version will also match if the first does not exist
  std::string convertExtension(const std::string & filepath) const;
  /// The action type of this property, i.e. load/save
  unsigned int m_action;
  ///The default file extension associated with the type of file this property will handle
  std::string m_defaultExt;
  /// Is this property for run files?
  bool m_runFileProp;
};

}
}

#endif //MANTID_API_FILEPROPERTY_H_
