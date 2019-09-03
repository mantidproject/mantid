#ifndef NEXUSGEOMETRY_MOCKOBJECTS_H
#define NEXUSGEOMETRY_MOCKOBJECTS_H
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidNexusGeometry/AbstractLogger.h"
#include <gmock/gmock.h>

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(doReport, void(const std::string &));
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
};

class MockLogger : public Mantid::NexusGeometry::AbstractLogger {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(warning, void(const std::string &));
  MOCK_METHOD1(error, void(const std::string &));
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

#endif // NEXUSGEOMETRY_MOCKOBJECTS_H
