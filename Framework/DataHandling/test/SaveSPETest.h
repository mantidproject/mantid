// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SAVESPETEST_H_
#define SAVESPETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/SaveSPE.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/File.h>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <numeric>

using namespace Mantid;
using namespace API;
using namespace DataHandling;
using Mantid::Geometry::Instrument;
using Mantid::Kernel::UnitFactory;

static const double MASK_FLAG =
    -1e30; // values here need to match what is in the SaveSPE.h file
static const double MASK_ERROR = 0.0;

static const int NHIST = 3;
static const int THEMASKED = 1;
static const int DEFAU_Y = 2;

class SaveSPETest : public CxxTest::TestSuite {
public:
  static SaveSPETest *createSuite() { return new SaveSPETest(); }
  static void destroySuite(SaveSPETest *suite) { delete suite; }

  SaveSPETest() { // the functioning of SaveSPE is affected by a function call
                  // in the FrameworkManager's constructor, creating the
                  // algorithm in this way ensures that function is executed
    saver = FrameworkManager::Instance().createAlgorithm("SaveSPE");
  }

  void testName() { TS_ASSERT_EQUALS(saver->name(), "SaveSPE"); }

  void testVersion() { TS_ASSERT_EQUALS(saver->version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(saver->initialize());
    TS_ASSERT(saver->isInitialized());
    TS_ASSERT_EQUALS(static_cast<int>(saver->getProperties().size()), 2);
  }

  void testExec() {
    // Create a small test workspace
    std::string WSName = "saveSPETest_input";
    MatrixWorkspace_const_sptr input = makeWorkspace(WSName);

    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("InputWorkspace", WSName));
    std::string outputFile("testSPE.spe");
    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("Filename", outputFile));
    outputFile = saver->getPropertyValue("Filename"); // get abs path

    TS_ASSERT_THROWS_NOTHING(saver->execute());
    TS_ASSERT(saver->isExecuted());

    TS_ASSERT(Poco::File(outputFile).exists());
    std::ifstream file(outputFile.c_str());

    std::string tmp;
    double tmp2;

    getline(file, tmp);
    TS_ASSERT_EQUALS(tmp, "       " + boost::lexical_cast<std::string>(NHIST) +
                              "      10")
    getline(file, tmp);
    TS_ASSERT_EQUALS(tmp, "### Phi Grid")
    file >> tmp2;
    TS_ASSERT_EQUALS(tmp2, 0.5)
    getline(file, tmp);
    getline(file, tmp);
    TS_ASSERT_EQUALS(tmp, "### Energy Grid")
    file >> tmp2;
    TS_ASSERT_EQUALS(tmp2, 1)
    getline(file, tmp);
    file >> tmp2;
    TS_ASSERT_EQUALS(tmp2, 9)
    getline(file, tmp);

    for (int i = 0; i < NHIST;
         ++i) { // if the spectrum number (1+index number) is that of the masked
                // spectrum look for the mask flag, otherwise value in the
                // workspace
      double value = i != THEMASKED ? DEFAU_Y : MASK_FLAG;

      getline(file, tmp);
      TS_ASSERT_EQUALS(tmp, "### S(Phi,w)")
      file >> tmp2;
      TS_ASSERT_EQUALS(tmp2, value)
      getline(file, tmp);
      file >> tmp2;
      TS_ASSERT_EQUALS(tmp2, value)
      getline(file, tmp);

      double error = i != THEMASKED ? M_SQRT2 : MASK_ERROR;
      getline(file, tmp);
      TS_ASSERT_EQUALS(tmp, "### Errors")
      file >> tmp2;
      TS_ASSERT_DELTA(tmp2, error, 0.001)
      getline(file, tmp);
      file >> tmp2;
      TS_ASSERT_DELTA(tmp2, error, 0.001)
      getline(file, tmp);
    }

    TS_ASSERT(file.good())
    // That should be the end of the file
    getline(file, tmp);
    TS_ASSERT(file.fail())

    file.close();

    AnalysisDataService::Instance().remove(WSName);
    Poco::File(outputFile).remove();
  }

  void testThatOutputIsValidFromWorkspaceWithNumericAxis() {
    // Create a small test workspace
    std::string WSName = "saveSPETestB_input";
    MatrixWorkspace_sptr input = makeWorkspaceWithNumericAxis(WSName);

    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("InputWorkspace", WSName));
    std::string outputFile("testSPE_Axis.spe");
    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("Filename", outputFile));
    outputFile = saver->getPropertyValue("Filename"); // get abs path
    saver->setRethrows(true);
    saver->execute();
    TS_ASSERT(saver->isExecuted());

    TS_ASSERT(Poco::File(outputFile).exists());
    if (Poco::File(outputFile).exists()) {
      Poco::File(outputFile).remove();
    }
  }

private:
  MatrixWorkspace_sptr makeWorkspace(const std::string &input) {
    // all the Y values in this new workspace are set to DEFAU_Y, which
    // currently = 2
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(NHIST, 10, 1.0);
    return setUpWorkspace(input, inputWS);
  }

  MatrixWorkspace_sptr makeWorkspaceWithNumericAxis(const std::string &input) {
    // all the Y values in this new workspace are set to DEFAU_Y, which
    // currently = 2
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(NHIST, 10, 1.0);
    inputWS = setUpWorkspace(input, inputWS);
    API::Axis *axisOne = inputWS->getAxis(1);
    auto newAxisOne = std::make_unique<NumericAxis>(axisOne->length());
    for (std::size_t i = 0; i < axisOne->length(); ++i) {
      newAxisOne->setValue(i, axisOne->operator()((i)));
    }
    // Set the units
    inputWS->replaceAxis(1, std::move(newAxisOne));
    inputWS->getAxis(1)->unit() = UnitFactory::Instance().create("Energy");
    inputWS->setYUnit("MyCaption");
    return inputWS;
  }

  MatrixWorkspace_sptr setUpWorkspace(const std::string &input,
                                      MatrixWorkspace_sptr inputWS) {
    inputWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("DeltaE");
    AnalysisDataService::Instance().add(input, inputWS);

    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "INES_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", input);
    loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    loader.execute();

    // mask the detector
    inputWS->mutableDetectorInfo().setMasked(THEMASKED, true);

    // required to get it passed the algorthms validator
    inputWS->setDistribution(true);

    return inputWS;
  }

private:
  IAlgorithm *saver;
};

#endif /*SAVESPETEST_H_*/
