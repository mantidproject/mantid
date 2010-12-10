#ifndef MANTID_API_IDATAFILECHECKER_H
#define MANTID_API_IDATAFILECHECKER_H

#include<string>
#include "MantidAPI/Algorithm.h"
static const unsigned char hdf5_signature[] = { '\211', 'H', 'D', 'F', '\r', '\n', '\032', '\n' };
static const int bufferSize=100;
   
namespace Mantid
{
  namespace API
  {

    /** 
    Base class for data file loading algorithms.This class provides interface for 
    data file loading algorithms to quickly check the file type by opening it and 
    reading the first 100 bytes /extension of the file. It also provides an interface to 
    check how much it can load the file by reading the first few lines of the file.
    
    @author Sofia Antony, ISIS Rutherford Appleton Laboratory
    @date 17/11/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/


    class DLLExport IDataFileChecker: public API::Algorithm
    {
    public:
      ///constructor
      IDataFileChecker();
      /// virtual destructor
      virtual ~IDataFileChecker();

      /// Algorithm's name
      virtual const std::string name() const { return ""; }
      /// Algorithm's version
      virtual int version() const { return 1; }
      /// Algorithm's category for identification
      virtual const std::string category() const { return "DataHandling"; }
      
      /// quick file check by reading first 100 bytes of the file or by checking the extension
      virtual bool quickFileCheck(const std::string& filePath,int nread,unsigned char* header_buffer)=0;
      
      /// file check by looking at the structure of the data file
      virtual int fileCheck(const std::string& filePath)=0;
      /// returns the extension of the file from filename
      std::string extension(const std::string& filePath);
      /// a union used for quick file check
      union { 
        unsigned u; 
        unsigned long ul; 
        unsigned char c[bufferSize+1]; 
      } header_buffer_union;
      
    
     private:
       /// initilaisation code
       void init();
       ///execution code
       void exec();

    };

    typedef boost::shared_ptr<IDataFileChecker> IDataFileChecker_sptr;
    typedef boost::shared_ptr<const IDataFileChecker> IDataFileChecker_const_sptr;
  }
}
#endif