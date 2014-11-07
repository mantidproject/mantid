#include "MantidKernel/ChecksumHelper.h"

#include <boost/uuid/sha1.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/shared_array.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <string>

namespace Mantid
{
namespace Kernel
{


namespace ChecksumHelper
{
  /** Creates a SHA-1 checksum from a string
  * @param input The string to checksum
  * @returns a checksum string
  **/
  std::string DLLExport sha1FromString(const std::string& input)
  {
    return ChecksumHelper::processSha1(NULL, 0,input.c_str(), input.size());
  }

  /** Creates a SHA-1 checksum from a file
  * @param filepath The path to the file
  * @returns a checksum string
  **/
  std::string DLLExport sha1FromFile(const std::string& filepath)
  {
    if (filepath.size() > 0)
    {
	    boost::scoped_ptr<std::ifstream> file_ptr(
		    new std::ifstream(filepath, std::fstream::in| std::fstream::binary));
	    if (!file_ptr->is_open())
		    return "";

      // get length of file:
      file_ptr->seekg (0, file_ptr->end);
      auto length = file_ptr->tellg();
      file_ptr->seekg (0, file_ptr->beg);

	    boost::shared_array<char> temp_buf(new char[length]);
      file_ptr->read(temp_buf.get(), length);
	    file_ptr.reset();
      return ChecksumHelper::processSha1(temp_buf.get(), length);
    }
    return "";
  }

  /** Creates a git checksum from a file (these match the git hash-object command).
  * This works by reading in the file, converting all line endings into linux style endings,
  * then the following is prepended to the file contents "blob <content_length>\0",  
  * the result is then ran through a SHA-1 checksum.
  * @param filepath The path to the file
  * @returns a checksum string
  **/
  std::string DLLExport gitSha1FromFile(const std::string& filepath)
  {
    std::string retVal = "";
    if (filepath.size() > 0)
    {
	    boost::scoped_ptr<std::ifstream> file_ptr(
		    new std::ifstream(filepath, std::fstream::in));
	    if (!file_ptr->is_open())
		    return retVal;

      //get the full length (including overallocation for \r\n)
      file_ptr->seekg (0, file_ptr->end);
      auto bufferlength = file_ptr->tellg();
      file_ptr->seekg (0, file_ptr->beg);

      //data
	    boost::shared_array<char> temp_buf(new char[bufferlength]);
      file_ptr->read(temp_buf.get(), bufferlength);
      // get real length of stream
      auto length = file_ptr->gcount();
	    file_ptr.reset();

      // header
      std::stringstream ss;
      ss << "blob " << length;
      std::string header = ss.str();

      size_t headerLength = header.size() +1;
      char* headerData = new char[headerLength];
      std::copy(header.begin(),header.end(),headerData);
      headerData[headerLength -1] = '\0';

      retVal = ChecksumHelper::processSha1(temp_buf.get(), length, headerData,headerLength);
      delete[] headerData;
    }
    return retVal;
  }

  /** internal method for processing sha1 checksums.
  * @param data The data to checksum
  * @param dataLength The length of the data to checksum
  * @param header The header to checksum
  * @param headerLength The length of header to checksum
  * @returns a checksum string
  **/
  std::string DLLExport processSha1(const char* data, const size_t dataLength, const char* header, const size_t headerLength)
  {
    boost::uuids::detail::sha1 hasher;
	  boost::shared_array<unsigned int> digest;
	  
    if ((headerLength > 0) && (header != NULL))
    {
      hasher.process_bytes(header, headerLength);
    }
    if ((dataLength > 0) && (data != NULL))
    {
      hasher.process_bytes(data, dataLength);
    }
	
	  digest.reset(new unsigned int [5]);
	  char bin[20];

	  hasher.get_digest(reinterpret_cast<boost::uuids::detail::sha1::digest_type>(*digest.get()));
	  for(int i = 0; i < 5; ++i)
	  {
		  const char* tmp = reinterpret_cast<char*>(digest.get());
		  bin[i * 4    ] = tmp[i * 4 + 3];
		  bin[i * 4 + 1] = tmp[i * 4 + 2];
		  bin[i * 4 + 2] = tmp[i * 4 + 1];
		  bin[i * 4 + 3] = tmp[i * 4    ];
	  }
      
    char* hash = &bin[0];

	  char str[128] = { 0 };
	  char *ptr = str;
	  std::string ret;

	  for (int i = 0; i < 20; i++)
	  {
		  sprintf(ptr, "%02X", (unsigned char)*hash);
		  ptr += 2;
		  hash++;
	  }
	  ret = str;
    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
	  return ret;
  }
  
} // namespace ChecksumHelper
} // namespace Kernel
} // namespace Mantid