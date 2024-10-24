// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY NOT be included in any test from a package below API
 *    (e.g. Kernel, Geometry).
 *  Conversely, this file MAY NOT be modified to use anything from a package
 *higher
 *  than API (e.g. any algorithm or concrete workspace), even if via the
 *factory.
 *********************************************************************************/
#pragma once

#include "MantidAPI/BoxController.h"
#include "MantidAPI/IBoxControllerIO.h"
#include "MantidKernel/DiskBuffer.h"
#include <mutex>

namespace MantidTestHelpers {

//===============================================================================================
/** The class responsible for dummy IO operations, which mimic saving events
   into a direct access
    file using generic box controller interface

    @date March 15, 2013
*/
class DLLExport BoxControllerDummyIO : public Mantid::API::IBoxControllerIO {
public:
  BoxControllerDummyIO(const Mantid::API::BoxController *bc);
  ///@return true if the file to write events is opened and false otherwise
  bool isOpened() const override { return (m_isOpened); }
  /// get the full file name of the file used for IO operations
  const std::string &getFileName() const override { return m_fileName; }
  void copyFileTo(const std::string &) override {}
  /**Return the size of the NeXus data block used in NeXus data array*/
  size_t getDataChunk() const override { return 1; }

  bool openFile(const std::string &fileName, const std::string &mode) override;
  void saveBlock(const std::vector<float> & /* DataBlock */, const uint64_t /*blockPosition*/) const override;
  void saveBlock(const std::vector<double> & /* DataBlock */, const uint64_t /*blockPosition*/) const override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "Saving double presision events blocks is not supported at the moment");
  }
  void loadBlock(std::vector<float> & /* Block */, const uint64_t /*blockPosition*/,
                 const size_t /*BlockSize*/) const override;
  void loadBlock(std::vector<double> & /* Block */, const uint64_t /*blockPosition*/,
                 const size_t /*BlockSize*/) const override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "Loading double presision events blocks is not supported at the "
        "moment");
  }
  void flushData() const override{};
  void closeFile() override { m_isOpened = false; }

  ~BoxControllerDummyIO() override;
  // Auxiliary functions. Used to change default state of this object which is
  // not fully supported. Should be replaced by some IBoxControllerIO factory
  void setDataType(const size_t blockSize, const std::string &typeName) override;
  void getDataType(size_t &CoordSize, std::string &typeName) const override;

  // Auxiliary functions (non-virtual, used at testing)
  int64_t getNDataColums() const { return 2; }

private:
  /// full file name (with path) of the Nexis file responsible for the IO
  /// operations (as NeXus filename has very strange properties and often
  /// trunkated to 64 bytes)
  std::string m_fileName;
  // the file Handler responsible for Nexus IO operations;
  mutable std::vector<float> fileContents;
  /// shared pointer to the box controller, which is repsoponsible for this IO
  const Mantid::API::BoxController *m_bc;

  mutable std::mutex m_fileMutex;
  /// number of bytes in the event coorinates (coord_t length). Set by
  /// setDataType but can be defined statically with coord_t
  unsigned int m_CoordSize;
  unsigned int m_EventSize;
  std::string m_TypeName;

  /// identifier if the file open only for reading or is  in read/write
  bool m_ReadOnly;
  /// identified of the file state, if it is open or not.
  bool m_isOpened;
};
} // namespace MantidTestHelpers
