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

  std::string DLLExport sha1FromString(const std::string& input)
  {
    return processSha1(NULL, 0,input.c_str(), input.size());
  }

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
      return processSha1(temp_buf.get(), length);
    }
    return "";
  }

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
      int bufferlength = file_ptr->tellg();
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

      retVal = processSha1(temp_buf.get(), length, headerData,headerLength);
      delete[] headerData;
    }
    return retVal;
  }

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
      
	  return sha1ToString(bin);
  }

  std::string DLLExport sha1ToString(const char *hash)
  {
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