// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Logger.h"

#include "MantidCrystal/SCDCalibratePanels2.h"


namespace Mantid {
namespace Crystal {

    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    /// Config logger
    namespace {
    Logger logger("SCDCalibratePanels2");
    }

    DECLARE_ALGORITHM(SCDCalibratePanels2)

    void SCDCalibratePanels2::init() {
        int a = 1;
    }

    std::map<std::string, std::string>
    SCDCalibratePanels2::validateInputs() {
        std::map<std::string, std::string> issues;

        return issues;
    }

    void SCDCalibratePanels2::exec() {
        int a = 1;
    }

} // namespace Crystal
} // namespace Mantid
