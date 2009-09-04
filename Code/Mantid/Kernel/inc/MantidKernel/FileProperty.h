#ifndef MANTID_KERNEL_FILEPROPERTY_H_
#define MANTID_KERNEL_FILEPROPERTY_H_

//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/FileValidator.h"

namespace Mantid
{
namespace Kernel
{
  /**
     A specialized class for dealing with file properties. Mantid allows multiple
     search paths to be defined so that each of these is used when attempting to
     load a file with a relative path. 
     
     When attempting to load a file this class handles searching the specified paths and, 
     if found, the <code>value()</code>  a method returns the full path to the file. For 
     saving, Mantid's default save directory is used when a relative path is encountered.
   */
class FileProperty : public PropertyWithValue<std::string>
{
public:
  /// An enumeration for load/save types. This is passed on to the FileValidator as a constructor parameter.
  enum
  {
    // Note that changing this order will break things!
    Save = 0,
    Load = 1
  };

public:
  ///Constructor
  FileProperty(const std::string & name, const std::string& default_value, unsigned int action,
	       const std::vector<std::string> & exts = std::vector<std::string>(), 
	       unsigned int direction = Direction::Input);

  ///Overridden setValue method
  virtual std::string setValue(const std::string & filename);

private:
  /// The action type of this property, i.e. load/save
  unsigned int m_action;
};

}
}

#endif //MANTID_KERNEL_FILEPROPERTY_H_
