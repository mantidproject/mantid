// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/NexusEntryProvider.h"

using Mantid::DataHandling::NexusEntryProvider;

class NexusEntryProviderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusEntryProviderTest *createSuite() { return new NexusEntryProviderTest(); }
  static void destroySuite(NexusEntryProviderTest *suite) { delete suite; }
};
