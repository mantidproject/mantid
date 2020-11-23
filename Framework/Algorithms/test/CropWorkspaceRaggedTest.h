// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <limits>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class CropWorkspaceRaggedTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CropWorkspaceRaggedTest *createSuite() { return new CropWorkspaceRaggedTest(); }
  static void destroySuite(CropWorkspaceRaggedTest *suite) { delete suite; }

  std::string createInputWorkspace() {
    std::string name = "toCrop";
    if (!AnalysisDataService::Instance().doesExist("toCrop")) {
      // Set up a small workspace for testing
      m_numberOfYPoints = 15;
      m_numberOfSpectra = 5;
      Workspace_sptr space =
          WorkspaceFactory::Instance().create("Workspace2D", m_numberOfSpectra, m_numberOfYPoints+1, m_numberOfYPoints);
      Workspace2D_sptr space2D = std::dynamic_pointer_cast<Workspace2D>(space);
      std::vector<double> ydata(m_numberOfYPoints*m_numberOfSpectra), errors(m_numberOfYPoints*m_numberOfSpectra);
      for (int j = 0; j < m_numberOfSpectra; ++j) {
        for (int k = 0; k < m_numberOfYPoints+1; ++k) {
          ydata[k+j*m_numberOfYPoints] = k+1;
          errors[k+j*m_numberOfYPoints] = sqrt(double(k+1));
          space2D->dataX(j)[k] = k;
        }
        auto beginY = std::next(ydata.begin(), j * m_numberOfYPoints);
        space2D->dataY(j) = std::vector<double>(beginY, std::next(beginY, m_numberOfYPoints));
        auto beginE = std::next(errors.begin(), j * numberOfYPoints);
        space2D->dataE(j) = std::vector<double>(beginE, std::next(beginE, m_numberOfYPoints));
      }
      InstrumentCreationHelper::addFullInstrumentToWorkspace(*space2D, false,
                                                             false, "");
      // Register the workspace in the data service
      AnalysisDataService::Instance().add(name, space);
    }
    return name;
  }

  void testName() { TS_ASSERT_EQUALS(crop.name(), "CropWorkspaceRagged"); }

  void testVersion() { TS_ASSERT_EQUALS(crop.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(crop.initialize());
    TS_ASSERT(crop.isInitialized());
  }

  void testInvalidInputs() {
    std::string inputName = createInputWorkspace();
    if (!crop.isInitialized())
      crop.initialize();

    TS_ASSERT_THROWS(crop.execute(), const std::runtime_error &);
    TS_ASSERT(!crop.isExecuted());
    TS_ASSERT_THROWS_NOTHING(
        crop.setPropertyValue("InputWorkspace", inputName));
    TS_ASSERT_THROWS_NOTHING(
        crop.setPropertyValue("OutputWorkspace", "nothing"));
    TS_ASSERT_THROWS_NOTHING(crop.setPropertyValue("XMin", "2"));
    TS_ASSERT_THROWS_NOTHING(crop.setPropertyValue("XMax", "1"));
    TS_ASSERT_THROWS_NOTHING(crop.execute());
    TS_ASSERT(crop.isExecuted());
  }

 void testSingleValues() {
    std::string inputName = createInputWorkspace();
    if (!crop.isInitialized())
      crop.initialize();
    TS_ASSERT_THROWS_NOTHING(
        crop.setPropertyValue("InputWorkspace", inputName));
    std::string outputWS("cropped");
    TS_ASSERT_THROWS_NOTHING(
        crop.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(crop.setPropertyValue("XMin", "2.0"));
    TS_ASSERT_THROWS_NOTHING(crop.setPropertyValue("XMax", "10"));

    TS_ASSERT_THROWS_NOTHING(crop.execute());
    TS_ASSERT(crop.isExecuted());
    if (!crop.isExecuted())
      return;
    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), m_numberOfSpectra);

    for (int i = 0; i < m_numberOfSpectra; ++i) {
        TS_ASSERT_EQUALS(output->readX(i)[0], 2.0);
        TS_ASSERT_EQUALS(output->readY(i)[0], 3.0);
        TS_ASSERT_EQUALS(output->readE(i)[0], sqrt(3.0));

	TS_ASSERT_EQUALS(output->readX(i).back(), 10.0);
        TS_ASSERT_EQUALS(output->readY(i).back(), 11.0);
        TS_ASSERT_EQUALS(output->readE(i).back(), sqrt(11.0));
      }
  }

 void testStartList() {
    std::string inputName = createInputWorkspace();
    std::vector<int> startValues;
    for(int j=0; j<m_numberOfSpectra; j++){
       startValues.push_back(j+1);
    }
    if (!crop.isInitialized())
      crop.initialize();
    TS_ASSERT_THROWS_NOTHING(
        crop.setPropertyValue("InputWorkspace", inputName));
    std::string outputWS("cropped");
    TS_ASSERT_THROWS_NOTHING(
        crop.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(crop.setPropertyValue("XMin", startValues));
    TS_ASSERT_THROWS_NOTHING(crop.setPropertyValue("XMax", "10"));
    TS_ASSERT_THROWS_NOTHING(crop.execute());
    TS_ASSERT(crop.isExecuted());
    if (!crop.isExecuted())
      return;
    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), m_numberOfSpectra);
        TS_ASSERT_EQUALS(output->readX(0)[0], 1.0);
        TS_ASSERT_EQUALS(output->readY(0)[0], 2.0);
        TS_ASSERT_EQUALS(output->readE(0)[0], sqrt(2.0));

	TS_ASSERT_EQUALS(output->readX(0).back(), 10.0);
        TS_ASSERT_EQUALS(output->readY(0).back(), 11.0);
        TS_ASSERT_EQUALS(output->readE(0).back(), sqrt(11.0));
 
    for (int i = 1; i < m_numberOfSpectra; ++i) {
        TS_ASSERT_EQUALS(output->readX(i)[0], double(i+1));
        TS_ASSERT_EQUALS(output->readY(i)[0], double(i+1));
        TS_ASSERT_EQUALS(output->readE(i)[0], sqrt(double(i+1));
	TS_ASSERT_EQUALS(output->readX(i).back(), 10.0);
        TS_ASSERT_EQUALS(output->readY(i).back(), 11.0);
        TS_ASSERT_EQUALS(output->readE(i).back(), sqrt(11.0));
      }
  }



private:
  CropWorkspace crop;
  int m_numberOfSpectral;
  int m_numberOfYPoints;
};

