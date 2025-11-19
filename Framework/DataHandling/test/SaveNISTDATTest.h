// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/Load.h"
#include "MantidDataHandling/SaveNISTDAT.h"
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <filesystem>

using namespace Mantid::API;
using namespace Mantid::DataHandling;

class SaveNISTDATTest : public CxxTest::TestSuite {
public:
  void test_writer() {
    std::string outputFile = "SaveNISTDAT_Output.dat";

    Load loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "saveNISTDAT_data.nxs"));
    loader.setPropertyValue("OutputWorkspace", "SaveNISTDAT_Input");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    SaveNISTDAT writer;
    writer.initialize();
    writer.setPropertyValue("InputWorkspace", "SaveNISTDAT_Input");
    writer.setPropertyValue("Filename", outputFile);
    outputFile = writer.getPropertyValue("Filename");
    TS_ASSERT_THROWS_NOTHING(writer.execute());

    TS_ASSERT(std::filesystem::exists(outputFile));

    std::ifstream testFile(outputFile.c_str(), std::ios::in);
    TS_ASSERT(testFile);

    std::string fileLine;
    std::getline(testFile, fileLine);
    TS_ASSERT_EQUALS(fileLine, "Data columns Qx - Qy - I(Qx,Qy) - err(I)\r");
    std::getline(testFile, fileLine);
    TS_ASSERT_EQUALS(fileLine, "ASCII data\r");
    std::getline(testFile, fileLine);
    TS_ASSERT((fileLine == "-0.0105  -0.0735  6.13876e+08  6.1697e+07\r") ||
              (fileLine == "-0.0105  -0.0735  6.13876e+008  6.1697e+007\r"))

    // remove file created by this algorithm, closing it first as Windows gets
    // tetchy about this
    testFile.close();
    std::filesystem::remove(outputFile);
  }
};
