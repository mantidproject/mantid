#include "MantidKernel/FileValidator.h"

namespace Mantid
{
namespace Kernel
{

  /** If m_fullTest=true if checks that the files exists, otherwise just that the extension is good
   * 
   *  @param value file name
   *  @returns An error message to display to users or an empty string on no error
   */
DLLExport std::string FileValidator::checkValidity(const std::string &value) const
{
  //check the file has a good extension
  if( !m_regex_exts.empty() )
  {
    //Find extension of value
    std::string ext = value.substr(value.rfind(".") + 1);
	  //Use a functor to test each allowed extension in turn
    RegExMatcher matcher(ext); 
    std::vector<std::string>::const_iterator itr = 
      std::find_if(m_regex_exts.begin(), m_regex_exts.end(), matcher);
    if( itr == m_regex_exts.end() )
    {//the extension doesn't match, list the possible extensions using a
      //different form if it is a list of one
      if (m_extensions.size() == 1) 
      {
        return "The file must have extension " + *(m_extensions.begin());
      }
      std::string error("The file must have one of these extensions: ");
      error = error + *( m_extensions.begin() );
      for ( itr = m_extensions.begin() + 1; itr < m_extensions.end(); itr++ )
      {
        error = error + ", " + *itr;
      }
      return error;
    }
  }//end if !m_regex_exts.empty()

  //If the file is required to exist check it is there
  if ( m_fullTest && ( value.empty() || !Poco::File(value).exists() ) )
  {
    return "File \"" + value + "\" not found";
  }
  return "";
}


} // namespace Kernel
} // namespace Mantid
