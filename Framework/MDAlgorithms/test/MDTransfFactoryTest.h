// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"

#include "MantidMDAlgorithms/MDTransfModQ.h"
#include "MantidMDAlgorithms/MDTransfNoQ.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;

//
class MDTransfFactoryTest : public CxxTest::TestSuite {
public:
  static MDTransfFactoryTest *createSuite() { return new MDTransfFactoryTest(); }
  static void destroySuite(MDTransfFactoryTest *suite) { delete suite; }

  void testInit() {
    std::vector<std::string> keys;

    TS_ASSERT_THROWS_NOTHING(keys = MDTransfFactory::Instance().getKeys());
    // we already have three transformation defined. It can be only more in a
    // future;
    TS_ASSERT(keys.size() > 2);
  }

  void testWrongAlgThrows() {
    TS_ASSERT_THROWS(MDTransfFactory::Instance().create("Non_existing_ChildAlgorithm"),
                     const Kernel::Exception::NotFoundError &);
  }

  void testGetAlg() {
    MDTransf_sptr transf;

    TS_ASSERT_THROWS_NOTHING(transf = MDTransfFactory::Instance().create("CopyToMD"));
    TS_ASSERT(dynamic_cast<MDTransfNoQ *>(transf.get()));

    TS_ASSERT_THROWS_NOTHING(transf = MDTransfFactory::Instance().create("|Q|"));
    TS_ASSERT(dynamic_cast<MDTransfModQ *>(transf.get()));

    TS_ASSERT_THROWS_NOTHING(transf = MDTransfFactory::Instance().create("Q3D"));
    TS_ASSERT(dynamic_cast<MDTransfQ3D *>(transf.get()));
  }

  //
  MDTransfFactoryTest() { API::FrameworkManager::Instance(); }
};
