// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAlgorithms/CorelliPowderCalibrationLoad.h"
#include "MantidAlgorithms/CorelliPowderCalibrationDatabase.h"
#include "MantidKernel/Logger.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>

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
                    FileProperty::Directory),
                "CORELLI calibration database directory");

            // CalibrationTable
            declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(
                    "OutputWorkspace", 
                    "", 
                    Direction::Output),
                "Name of the output CORELLI calibration table");

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
         * @brief 
         * 
         * @param ws: input workspace
         * @return std::string : deduced filename with YYYYMMDD format
         */
        std::string CorelliPowderCalibrationLoad::deduce_database_name(API::MatrixWorkspace_sptr ws){
            std::string timeStamp{""};
            if (ws->run().hasProperty("start_time")){
                timeStamp = ws->run().getProperty("start_time")->value();
            }else {
                timeStamp = ws->run().getProperty("run_start")->value();
            }
            // convert date to YYYYMMDD
            std::string timeStampStr = CorelliPowderCalibrationDatabase::convertTimeStamp(timeStamp);

            // dedeuced file name
            std::string dbFileName = "corelli_instrument_" + timeStampStr + ".csv";

            return dbFileName;
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
            std::string dbdir = getProperty("DatabaseDir");

            // Prepare output table
            const std::string calTableName = getPropertyValue("OutputWorkspace");

            // Locate the time stamp in ws, and form the db file path
            // NOTE:
            //  Using inline functions from other class leads to a not
            //  defined warning.
            std::string dbFileName = deduce_database_name(ws);
            boost::filesystem::path dir(dbdir);
            boost::filesystem::path file(dbFileName);
            boost::filesystem::path fullPath = dir / file;
            std::string dbFullPath = fullPath.string();

            // Load the csv file into a table
            g_log.notice() << "Loading database:\n";
            g_log.notice() << "\t" << dbFullPath << "\n";
            auto alg = createChildAlgorithm("LoadAscii");
            alg -> initialize();
            alg -> setProperty("Filename", dbFullPath);
            alg -> setProperty("Separator", "CSV");
            alg -> setProperty("CommentIndicator", "#");
            alg -> setPropertyValue("OutputWorkspace", calTableName);
            alg -> execute();

            Workspace_sptr _outws = alg->getProperty("OutputWorkspace");
            TableWorkspace_sptr calTable = std::dynamic_pointer_cast<TableWorkspace>(_outws);

            // get the table and set it as the output
            setProperty("OutputWorkspace", calTable);
            g_log.notice() << "Finished loading CORELLI calibration table\n";
        }

    } // namespace Algorithms

} // namespace Mantid
