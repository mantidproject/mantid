// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAlgorithms/CorelliPowderCalibrationLoad.h"
#include "MantidAlgorithms/CorelliPowderCalibrationDatabase.h"
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

            // InputWorkspace
            auto wsValidator = std::make_shared<InstrumentValidator>();
            declareProperty(
                std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                    "Workspace", 
                    "", 
                    Direction::Input,
                    PropertyMode::Mandatory,
                    wsValidator),
                "CORELLI workspace to calibrate");

            // Database directory
            // NOTE:
            //  The default path, /SNS/CORELLI/shared/database, has not been
            //  setup yet, and the final decision is up to the CIS&IS of the
            //  CORELLI
            declareProperty(
                std::make_unique<FileProperty>(
                    "DatabaseDir",
                    "/SNS/CORELLI/shared/database",
                    FileProperty::Directory,
                    Direction::Input),
                "CORELLI calibration database directory");

            // CalibrationTable
            declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(
                    "CalibrationTable", 
                    "", 
                    Direction::InOut, 
                    PropertyMode::Mandatory),
                "CORELLI calibration table");

        }

        /**
         * @brief Validate algorithm inputs
         * 
         * @return std::map<std::string, std::string> 
         */
        std::map<std::string, std::string> CorelliPowderCalibrationLoad::validateInputs() {
            std::map<std::string, std::string> issues;
            ws = getProperty("Workspace");

            // 1_check: input workspace is from CORELLI
            if (ws->getInstrument()->getName() != "CORELLI") {
                issues["Workspace"] = "CORELLI only algorithm, aborting";
            }

            // 2_check: anything else?

            return issues;
        }

        /**
         * @brief Executes the algorithm.
         * 
         */
        void CorelliPowderCalibrationLoad::exec() {
            g_log.notice() << "Start loading CORELLI calibration table from database\n";

            // Parse input arguments
            ws = getProperty("Workspace");
            wsName = getPropertyValue("Workspace");
            dbdir = getPropertyValue("DatabaseDir");

            // Locate the time stamp in ws, and form the db file path

            // Load the csv file into a table

            // Set the table as the output

            g_log.notice() << "Finished loading CORELLI calibration table\n";
        }

    } // namespace Algorithms

} // namespace Mantid
