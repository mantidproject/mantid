//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDataFileChecker.h"

namespace Mantid
{
  namespace API
  {
#ifndef WIN32
    // Magic HDF5 signature
    unsigned char const IDataFileChecker::g_hdf5_signature[8] = { '\211', 'H', 'D', 'F', '\r', '\n', '\032', '\n' };
    /// Magic HDF5 cookie that is stored in the first 4 bytes of the file.
    uint32_t const IDataFileChecker::g_hdf5_cookie = 0x0e031301;
#endif

    /// constructor
    IDataFileChecker::IDataFileChecker():API::Algorithm()
    {
    }

    /// destructor
    IDataFileChecker::~IDataFileChecker()
    {
    }

    /** returns the extension of the given file
     *  @param fileName :: name of the file.
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
