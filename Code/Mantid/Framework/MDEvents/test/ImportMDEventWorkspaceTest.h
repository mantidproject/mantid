#ifndef MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <fstream>
#include <iomanip>

#include "MantidMDEvents/ImportMDEventWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;


class ImportMDEventWorkspaceTest : public CxxTest::TestSuite
{
private:

/*
This builder type provides a convenient way to create and change the contents of a virtual file of the type expected 
by the ImportMDEventWorkspace algorithm.

This type is particularly useful when generating corrupt file contents, as it allows individual apects of the file contents to be modified independently.
*/
  class FileContentsBuilder
  {
  private:
    std::string m_DimensionBlock;
    std::string m_MDEventsBlock;
    std::string m_DimensionEntries;
    std::string m_MDEventEntries;
  public:
    FileContentsBuilder() : 
        m_DimensionBlock(ImportMDEventWorkspace::DimensionBlockFlag()), 
          m_MDEventsBlock(ImportMDEventWorkspace::MDEventBlockFlag()),
          m_DimensionEntries("a, A, U, 10\nb, B, U, 11")
        {
        }

        void setDimensionBlock(const std::string& value)
        {
          m_DimensionBlock = value;
        }

        void setMDEventBlock(const std::string& value)
        {
          m_MDEventsBlock = value;
        }

        void setDimensionEntries(const std::string& value)
        {
          m_DimensionEntries = value;
        }

        std::string create() const
        {
          const std::string newline = "\n";
          return m_DimensionBlock + newline + m_DimensionEntries + newline + m_MDEventsBlock + newline;
        }
  };


/**
Helper type. Creates a test file, and also manages the resource to ensure that the file is closed and removed, no matter what the outcome of the test.

Uses a 
*/
class MDFileObject
{
public:

  /// Create a simple input file.
  MDFileObject(const FileContentsBuilder& builder = FileContentsBuilder(), std::string filename="test_import_md_event_workspace_file.txt") : m_filename(filename)
  {
    m_file.open (filename.c_str());
    // Invoke the builder to create the contents of the file.
    m_file << builder.create();
    m_file.close();
  }

  std::string getFileName() const
  {
    return m_filename;
  }

  /// Free up resources.
  ~MDFileObject()
  {
    m_file.close();
    if( remove( m_filename.c_str() ) != 0 )
      throw std::runtime_error("cannot remove " + m_filename);
  }

private:
  std::string m_filename;
  std::ofstream m_file;
  // Following methods keeps us from being able to put objects of this type on the heap.
  void *operator new(size_t);
  void *operator new[](size_t);
};

/**
Helper method runs tests that should throw and invalid argument when the algorithm is executed.
*/
void do_check_throws_invalid_alg_upon_execution(const MDFileObject& infile)
{
  ImportMDEventWorkspace alg;
  alg.initialize();
  alg.setRethrows(true);
  alg.setPropertyValue("Filename", infile.getFileName());
  alg.setPropertyValue("OutputWorkspace", "test_out");
  TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
}

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImportMDEventWorkspaceTest *createSuite() { return new ImportMDEventWorkspaceTest(); }
  static void destroySuite( ImportMDEventWorkspaceTest *suite ) { delete suite; }


  void test_Init()
  {
    ImportMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_missing_dimension_block_throws()
  {
    // Setup the corrupt file
    FileContentsBuilder fileContents;
    fileContents.setDimensionBlock("");
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }

  void test_missing_mdevents_block_throws()
  {
    // Setup the corrupt file 
    FileContentsBuilder fileContents;
    fileContents.setMDEventBlock("");
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }

  void test_mdevent_block_declared_before_dimension_block_throws()
  {
    // Setup the corrupt file. Notice that the DimensionBlockFlag and the MDEventBlockFlag arguments have been swapped over.
    FileContentsBuilder fileContents;
    fileContents.setDimensionBlock(ImportMDEventWorkspace::MDEventBlockFlag());
    fileContents.setMDEventBlock(ImportMDEventWorkspace::DimensionBlockFlag());
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }

  void test_dimension_block_has_corrupted_entries_throws()
  {
    // Setup the corrupt file. Notice that the DimensionBlockFlag and the MDEventBlockFlag arguments have been swapped over.
    FileContentsBuilder fileContents;
    std::string dim1 = "a, A, U, 10\n";
    std::string dim2 = "b, B, U, 11\n";
    std::string dim3 = "b, B, U\n"; // Ooops, forgot to put in the number of bins for this dimension.
    fileContents.setDimensionEntries(dim1 + dim2 + dim3);
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }


};


#endif /* MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACETEST_H_ */