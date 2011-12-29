#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/System.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/Property.h"
#include <Poco/Path.h>
#include "MantidAPI/FileFinder.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace API
{

  /** Constructor
   *
   * @param name ::          The name of the property
   * @param exts ::          The allowed/suggested extensions
   * @param optional ::      If ture, the property is optional
   */
  MultipleFileProperty::MultipleFileProperty(const std::string & name,
      const std::vector<std::string> & exts, bool optional)
    : ArrayProperty<std::string>(name),
      m_optional(optional)
  {
    m_exts.insert( exts.begin(), exts.end());
  }


    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MultipleFileProperty::~MultipleFileProperty()
  {
  }

  /** Set the value, the list of files, comma-separated
   *
   * @param propValue :: comma-separated string of filenames
   * @return A string indicating the outcome of the attempt to set the property. An empty string indicates success.
   */
  std::string MultipleFileProperty::setValue(const std::string & propValue)
  {
    // Separate the files
    std::vector<std::string> filenames;
    toValue(propValue, filenames);

    // Empty value is allowed if optional
    if( filenames.empty())
    {
      if (m_optional)
        return "";
      else
        return "No file specified.";
    }

    std::string outValue;
    for (size_t i=0; i<filenames.size(); i++)
    {
      std::string filename = filenames[i];

      // Adjust the filename if the path is not absolute
      if( !Poco::Path(propValue).isAbsolute() )
      {
        filename = FileFinder::Instance().getFullPath(filename);
      }
      // Re-build a comma-sep string
      if (i > 0) outValue += ",";
      outValue += filename;
    }

    // Now re-set the strings using the full paths found
    return ArrayProperty<std::string>::setValue(outValue);
  }

  /**
   * Set a property value via a DataItem
   * @param data :: A shared pointer to a data item
   * @return "" if the assignment was successful or a user level description of the problem
  */
  std::string MultipleFileProperty::setValue(const boost::shared_ptr<Kernel::DataItem> data )
  {
    // Implemented this method for documentation reasons. Just calls base class method.
    return ArrayProperty<std::string>::setValue(data);
  }



} // namespace Mantid
} // namespace API

