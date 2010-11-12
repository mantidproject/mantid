#ifndef DIRECTORYVALIDATORTEST_H_
#define DIRECTORYVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/DirectoryValidator.h"
#include "Poco/File.h"

using namespace Mantid::Kernel;

class DirectoryValidatorTest: public CxxTest::TestSuite
{
public:

  void testFailsOnNonexistentDirectory()
  {
    DirectoryValidator v(true);
    std::string NoDir("/home/MyJunkyFolderThatDoesntExist");
    TS_ASSERT_EQUALS( v.isValid(NoDir),
          "Directory \"" + NoDir + "\" not found" );
  }

  void testFailsOnAFile()
  {
    DirectoryValidator v(true);
    std::string ThisIsAFile("directoryvalidatortestfile.txt");
    Poco::File txt_file(ThisIsAFile);
    txt_file.createFile();
    TS_ASSERT_EQUALS( v.isValid(ThisIsAFile),
          "Directory \"" + ThisIsAFile + "\" specified is actually a file" );
    txt_file.remove();
  }

  void testPassesOnNonexistentDirectoryIfYouSaySoForSomeReason()
  {
    DirectoryValidator v(false);
    std::string NoDir("./MyJunkyFolderThatDoesntExist");
    TS_ASSERT_EQUALS( v.isValid(NoDir), "");
  }

  void testPassesOnExistingDirectory()
  {
    std::string TestDir("./MyTestFolder");
    Poco::File dir(TestDir);
    dir.createDirectory();
    DirectoryValidator v(true);
    TS_ASSERT_EQUALS( v.isValid(TestDir), "");
    dir.remove(); //clean up your folder
  }




};

#endif /*DIRECTORYVALIDATORTEST_H_*/
