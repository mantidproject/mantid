//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDataFileChecker.h"

namespace Mantid
{
  namespace API
  {
    /// constructor
    IDataFileChecker::IDataFileChecker():API::Algorithm()
    {
    }
   ///destructor
    IDataFileChecker::~IDataFileChecker()
    {
    }
    ///Init method
    void IDataFileChecker::init()
    {
    }
    ///overridden method,not doing anything
    void IDataFileChecker::exec()
    {
    }
  /** retuns the extension of the given file
    * @param fileName - name of the file.
    */
    std::string IDataFileChecker::extension(const std::string& fileName)
    {
      std::size_t pos=fileName.find_last_of(".");
      if(pos==std::string::npos)
      {
        return"" ;
      }
      std::string extn=fileName.substr(pos+1,fileName.length()-pos);
      std::transform(extn.begin(),extn.end(),extn.begin(),tolower);
      return extn;
    }

  }
}
