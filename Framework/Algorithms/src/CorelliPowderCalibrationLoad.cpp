// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CorelliPowderCalibrationLoad.h"
#include "MantidKernel/Logger.h"

namespace Mantid {

    namespace Algorithms {
        using namespace Kernel;
        using namespace API;
        using namespace DataObjects;

        /// Config logger
        namespace {
            Logger logger("CorelliPowderCalibrationLoad");
        }

        DECLARE_ALGORITHM(CorelliPowderCalibrationLoad)

        /**
         * @brief Initialization
         * 
         */
        void CorelliPowderCalibrationLoad::init() {

        }

        /**
         * @brief Validate algorithm inputs
         * 
         * @return std::map<std::string, std::string> 
         */
        std::map<std::string, std::string> CorelliPowderCalibrationLoad::validateInputs() {
            std::map<std::string, std::string> issues;

            return issues;
        }

        void CorelliPowderCalibrationLoad::exec() {
            g_log.notice() << "Start loading CORELLI calibration table from database\n";

            g_log.notice() << "Finished loading CORELLI calibration table\n";
        }

    } // namespace Algorithms

} // namespace Mantid
