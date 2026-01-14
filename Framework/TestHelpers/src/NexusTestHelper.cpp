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
#include <filesystem>
#include <memory>

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#include "MantidNexus/NexusFile.h"
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
    if (std::filesystem::exists(filename))
      std::filesystem::remove(filename);
  }
}

//----------------------------------------------------------------------------------------------
/** Creates a NXS file with an entry, for use in a test
 * @param barefilename :: simple filename (no path) to save to.
 * */
void NexusTestHelper::createFile(const std::string &barefilename) {
  filename = (Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory") + barefilename);
  if (std::filesystem::exists(filename))
    std::filesystem::remove(filename);
  file = std::make_unique<Mantid::Nexus::File>(filename, NXaccess::CREATE5);
  file->makeGroup("test_entry", "NXentry", true);
}

//----------------------------------------------------------------------------------------------
/** Close the newly created file and re-open it for reading back. */
void NexusTestHelper::reopenFile() {
  if (!file)
    throw std::runtime_error("NexusTestHelper: you must call createFile() before reopenFile().");
  file->close();
  file = std::make_unique<Mantid::Nexus::File>(filename, NXaccess::READ);
  file->openGroup("test_entry", "NXentry");
}
