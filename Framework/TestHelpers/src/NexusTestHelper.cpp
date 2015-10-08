/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This file MAY NOT be modified to use anything from a package other than
 *Kernel.
 *********************************************************************************/
#include "MantidKernel/ConfigService.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include <Poco/File.h>

//----------------------------------------------------------------------------------------------
/** Constructor.
   * */
NexusTestHelper::NexusTestHelper(bool deleteFile)
    : file(NULL), deleteFile(deleteFile) {}

//----------------------------------------------------------------------------------------------
/** Destructor.
   * Close the NXS file and delete it.
   */
NexusTestHelper::~NexusTestHelper() {
  if (!file)
    return;
  file->close();
  delete file;
  if (deleteFile) {
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }
}

//----------------------------------------------------------------------------------------------
/** Creates a NXS file with an entry, for use in a test
   * @param barefilename :: simple filename (no path) to save to.
   * */
void NexusTestHelper::createFile(std::string barefilename) {
  filename = (Mantid::Kernel::ConfigService::Instance().getString(
                  "defaultsave.directory") +
              barefilename);
  if (Poco::File(filename).exists())
    Poco::File(filename).remove();
  file = new ::NeXus::File(filename, NXACC_CREATE5);
  file->makeGroup("test_entry", "NXentry", 1);
}

//----------------------------------------------------------------------------------------------
/** Close the newly created file and re-open it for reading back. */
void NexusTestHelper::reopenFile() {
  if (!file)
    throw std::runtime_error(
        "NexusTestHelper: you must call createFile() before reopenFile().");
  file->close();
  delete file;
  file = new ::NeXus::File(filename, NXACC_READ);
  file->openGroup("test_entry", "NXentry");
}
