#ifndef MANTID_KERNEL_FILEPROPERTY_H_
#define MANTID_KERNEL_FILEPROPERTY_H_

//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
#include "MantidKernel/PropertyWithValue.h"

namespace Mantid
{
namespace Kernel
{
  /**
     A specialized class for dealing with file properties. Mantid allows multiple
     search paths to be defined so that each of these is used when attempting to
     load a file with a relative path. 
     
     When attempting to load a file this class handles searching the specified paths and, 
     if found, the <code>value()</code> method returns the full path to the file. For 
     saving, Mantid's default save directory is used when a relative path is encountered.
   */
class DLLExport FileProperty : public PropertyWithValue<std::string>
{
public:
  /// An enumeration for load/save types. This is passed on to the FileValidator as a constructor parameter.
  enum FileAction
  {
    // Note that the order here ensures that the correct boolean gets passed to the FileValidator.
    Save = 0,              ///< to specify a file to write to, the file may or may not exist
    OptionalSave = 1,      ///< to specify a file to write to but an empty string is allowed here which will be passed to the algorithm
    Load = 2,              ///< to specify a file to open for reading, the file must exist
    OptionalLoad = 3        ///< to specify a file to read but the file doesn't have to exist
  };

  ///Constructor
  FileProperty(const std::string & name, const std::string& default_value, unsigned int action,
      const std::vector<std::string> & exts = std::vector<std::string>(), 
      unsigned int direction = Direction::Input);
  FileProperty(const std::string & name, const std::string& default_value, unsigned int action,
      const std::string & ext, unsigned int direction = Direction::Input);

  /// 'Virtual copy constructor
  Property* clone() { return new FileProperty(*this); }

  /// Check if this is a load type property.
  bool isLoadProperty() const;

  ///Overridden setValue method
  virtual std::string setValue(const std::string & filename);

private:
  /// Check that a given directory exists
  std::string checkDirectory(const std::string & filepath) const;
  /// Check file extension to see if a lower- or upper-cased version will also match if the first does not exist
  std::string convertExtension(const std::string & filepath) const;
  /// The action type of this property, i.e. load/save
  unsigned int m_action;
};

}
}

#endif //MANTID_KERNEL_FILEPROPERTY_H_
