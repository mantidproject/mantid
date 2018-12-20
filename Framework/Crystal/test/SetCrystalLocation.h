// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SETCRYSTALLOCATIONTEST_H_
#define SETCRYSTALLOCATIONTEST_H_
#include "MantidAPI/ITableWorkspace.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/ShowPeakHKLOffsets.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include <cxxtest/TestSuite.h>

using Mantid::DataHandling::LoadNexusProcessed;
using namespace Mantid::DataObjects;
using Mantid::Crystal::LoadIsawUB;
using Mantid::Crystal::ShowPeakHKLOffsets;
using Mantid::DataObjects::TableWorkspace;



class SetCrystalLocationTest : public CxxTest::TestSuite {
public:
  void test_basic() {
    LoadNexusProcessed load;
    load.initialize();
    load.setProperty("Filename", "TOPAZ_3007.peaks.nxs");
    load.setProperty("OutputWorkspace", "aaa");
    load.execute();

  }


};

#endif /* SETCRYSTALLOCATIONTEST_H_ */
