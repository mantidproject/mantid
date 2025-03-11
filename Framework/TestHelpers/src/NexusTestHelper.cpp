// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This file MAY NOT be modified to use anything from a package other than
 *Kernel.
 *********************************************************************************/
#include "MantidFrameworkTestHelpers/NexusTestHelper.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/File.h>
#include <memory>

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#include "MantidNexus/NeXusFile.hpp"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

//----------------------------------------------------------------------------------------------
/** structor.
 * */
NexusTestHelper::NexusTestHelper(bool deleteFile) : file(nullptr), deleteFile(deleteFile) {}

//----------------------------------------------------------------------------------------------
/** Destructor.
 * Close the NXS file and delete it.
 */
NexusTestHelper::~NexusTestHelper() {
  if (!file)
    return;
  file->close();
  if (deleteFile) {
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }
}

//----------------------------------------------------------------------------------------------
/** Creates a NXS file with an entry, for use in a test
 * @param barefilename :: simple filename (no path) to save to.
 * */
void NexusTestHelper::createFile(const std::string &barefilename) {
  filename = (Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory") + barefilename);
  if (Poco::File(filename).exists())
    Poco::File(filename).remove();
  file = std::make_unique<::NeXus::File>(filename, NXACC_CREATE5);
  file->makeGroup("test_entry", "NXentry", true);
}

//----------------------------------------------------------------------------------------------
/** Close the newly created file and re-open it for reading back. */
void NexusTestHelper::reopenFile() {
  if (!file)
    throw std::runtime_error("NexusTestHelper: you must call createFile() before reopenFile().");
  file->close();
  file = std::make_unique<::NeXus::File>(filename, NXACC_READ);
  file->openGroup("test_entry", "NXentry");
}
