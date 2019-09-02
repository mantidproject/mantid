// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NOTEBOOKWRITERTEST_H_
#define MANTID_NOTEBOOKWRITERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/NotebookBuilder.h"
#include "MantidTestHelpers/FakeObjects.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class NotebookWriterTest : public CxxTest::TestSuite {
public:
  void test_writeNotebook() {
    auto notebook = std::make_unique<NotebookWriter>();

    std::string notebookText = notebook->writeNotebook();

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, notebookText, boost::is_any_of("\n"));

    // Test if the name metadata line is present in the output notebook text
    TS_ASSERT_EQUALS(notebookLines[2], "      \"name\" : \"Mantid Notebook\"")
  }

  void test_markdownCell() {
    auto notebook = std::make_unique<NotebookWriter>();
    std::string test_data = "Test markdown cell";
    std::string notebookText = notebook->markdownCell(test_data);

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, notebookText, boost::is_any_of("\n"));

    // Test if the markdown cell is present in the output notebook text
    TS_ASSERT_EQUALS(notebookLines[1], "   \"cell_type\" : \"markdown\",")
    // Test if the test_data string is present in the output notebook text
    TS_ASSERT_EQUALS(notebookLines[3], "   \"source\" : \"" + test_data + "\"")
  }

  void test_codeCell() {
    auto notebook = std::make_unique<NotebookWriter>();
    std::string test_data = "print 'Test code cell'";
    std::string notebookText = notebook->codeCell(test_data);

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, notebookText, boost::is_any_of("\n"));

    // Test if the code cell is present in the output notebook text
    TS_ASSERT_EQUALS(notebookLines[1], "   \"cell_type\" : \"code\",")
    // Test if the test_data string is present in the output notebook text
    TS_ASSERT_EQUALS(notebookLines[3], "   \"input\" : \"" + test_data + "\",")
  }
};

#endif // MANTID_NOTEBOOKWRITERTEST_H_
