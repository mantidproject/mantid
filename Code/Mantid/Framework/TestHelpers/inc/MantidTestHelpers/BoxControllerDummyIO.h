/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY NOT be included in any test from a package below API
 *    (e.g. Kernel, Geometry).
 *  Conversely, this file MAY NOT be modified to use anything from a package higher
 *  than API (e.g. any algorithm or concrete workspace), even if via the factory.
 *********************************************************************************/
#ifndef MANTID_TESTHELPERS_BOXCONTROLLER_DUMMUY_IO_H
#define MANTID_TESTHELPERS_BOXCONTROLLER_DUMMUY_IO_H

#include "MantidAPI/IBoxControllerIO.h"
#include "MantidAPI/BoxController.h"
#include "MantidKernel/DiskBuffer.h"


namespace MantidTestHelpers
{

  //===============================================================================================
  /** The class responsible for dummy IO operations, which mimic saving events into a direct access 
      file using generic box controller interface

      @date March 15, 2013

      Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

      File change history is stored at: <https://github.com/mantidproject/mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
    class DLLExport BoxControllerDummyIO : public Mantid::API::IBoxControllerIO
    {
        public:
            BoxControllerDummyIO(const Mantid::API::BoxController* theBC);

           ///@return true if the file to write events is opened and false otherwise
            virtual bool isOpened()const
            {
                return  (m_isOpened);
            }
            /// get the full file name of the file used for IO operations
            virtual const std::string &getFileName()const
            {
                return m_fileName;
            }
            /**Return the size of the NeXus data block used in NeXus data array*/ 
            size_t getDataChunk()const
            {
                return 1;
            }
                 
            virtual bool openFile(const std::string &fileName,const std::string &mode);
            virtual void saveBlock(const std::vector<float> & /* DataBlock */, const uint64_t /*blockPosition*/)const;
            virtual void saveBlock(const std::vector<double> & /* DataBlock */, const uint64_t /*blockPosition*/)const
            {throw Mantid::Kernel::Exception::NotImplementedError("Saving double presision events blocks is not supported at the moment");}
            virtual void loadBlock(std::vector<float> &  /* Block */, const uint64_t /*blockPosition*/,const size_t /*BlockSize*/)const;
            virtual void loadBlock(std::vector<double> &  /* Block */, const uint64_t /*blockPosition*/,const size_t /*BlockSize*/)const
            {throw Mantid::Kernel::Exception::NotImplementedError("Loading double presision events blocks is not supported at the moment");}
            virtual void flushData()const{};
            virtual void closeFile(){m_isOpened=false;}

            virtual ~BoxControllerDummyIO();
            //Auxiliary functions. Used to change default state of this object which is not fully supported. Should be replaced by some IBoxControllerIO factory
            virtual void setDataType(const size_t coordSize, const std::string &typeName);
            virtual void getDataType(size_t &coordSize, std::string &typeName)const;

            //Auxiliary functions (non-virtual, used at testing)
            int64_t getNDataColums()const
            {
                return 2;
            }
    private:
        /// full file name (with path) of the Nexis file responsible for the IO operations (as NeXus filename has very strange properties and often trunkated to 64 bytes)
        std::string m_fileName;
        // the file Handler responsible for Nexus IO operations;
        mutable std::vector<float> fileContents;
        /// shared pointer to the box controller, which is repsoponsible for this IO
        const Mantid::API::BoxController* m_bc;

        mutable Mantid::Kernel::Mutex m_fileMutex;
        /// number of bytes in the event coorinates (coord_t length). Set by  setDataType but can be defined statically with coord_t 
        unsigned int m_CoordSize;
        unsigned int m_EventSize;
        std::string m_TypeName;

       /// identifier if the file open only for reading or is  in read/write 
        bool m_ReadOnly;
        /// identified of the file state, if it is open or not.
        bool m_isOpened;



    };

}
#endif
