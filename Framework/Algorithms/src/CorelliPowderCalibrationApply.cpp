// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CorelliPowderCalibrationApply.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
    namespace Algorithms {

        using namespace Kernel;
        using namespace API;
        namespace {
        Logger logger("CorelliPowderCalibrationApply");
        }

        DECLARE_ALGORITHM(CorelliPowderCalibrationApply)

        /**
         * @brief Initialization
         * 
         */
        void CorelliPowderCalibrationApply::init() {

            // InputWorkspace
            // [Input, Required, MatrixWorkspace or EventsWorkspace]
            // workspace to which the calibration should be applied
            auto wsValidator = std::make_shared<InstrumentValidator>();
            declareProperty(
                std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                    "Workspace",
                    "",
                    Direction::InOut,
                    PropertyMode::Mandatory,
                    wsValidator),
                "CORELLI workspace to calibrate");

            // CalibrationTable
            // [Input, Mandatory, TableWorkspace]
            // workspace resulting from uploading
            declareProperty(
                std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                    "CalibrationTable",
                    "",
                    Direction::Input,
                    PropertyMode::Mandatory),
                "TableWorkspace containing calibration table");
        }

        /**
         * @brief Validate algorithm inputs
         * 
         * @return std::map<std::string, std::string> 
         */
        std::map<std::string, std::string>
        CorelliPowderCalibrationApply::validateInputs() {
            std::map<std::string, std::string> issues;
            ws = getProperty("Workspace");
            
            // 1_check: input workspace is from CORELLI
            if (ws->getInstrument()->getName() != "CORELLI") {
                issues["Workspace"] = "CORELLI only algorithm, aborting";
            }

            // 2_check: headers of calibration table
            calTable = getProperty("CalibrationTable");
            auto refCalTableHeader = CorelliCalibration::calibrationTableColumnNames;
            std::vector<std::string> colnames = calTable->getColumnNames();
            if (colnames.size() != refCalTableHeader.size()) {
                issues["CalibrationTable"] = "Headers of input calibration table does not match required";
            }
            for (size_t i=0; i<colnames.size(); i++){
                if (colnames[i] != refCalTableHeader[i]){
                    issues["CalibrationTable"] = "Header mismatch at " + std::to_string(i);
                    break;
                }
            }

            return issues;
        }
    
        /**
         * @brief Executes the algorithm.
         * 
         */
        void CorelliPowderCalibrationApply::exec(){
            g_log.notice() << "Start applying CORELLI calibration\n";

            // Parse input arguments
            ws = getProperty("Workspace");
            auto wsName = ws->getName();

            calTable = getProperty("CalibrationTable");
            auto componentNames = calTable->getColumn(0);
            auto x_poss = calTable->getColumn(1);
            auto y_poss = calTable->getColumn(2);
            auto z_poss = calTable->getColumn(3);
            auto rotxs = calTable->getColumn(4);
            auto rotys = calTable->getColumn(5);
            auto rotzs = calTable->getColumn(6);
            auto rotangs = calTable->getColumn(7);  //unit: degrees

            // Translate each component in the instrument
            // [source, sample, bank1,.. bank92]
            // dev reference:
            // https://github.com/mantidproject/mantid/blob/f0fad29ee0496901b8d453c738ea948644c7c6a6/Framework/Reflectometry/src/SpecularReflectionPositionCorrect2.cpp#L241
            // Question: single thread or parallel
            //           for parallel, any suggestions
            g_log.notice() << "Translating each component using given Calibration table";
            // Question: createChildAlgorithm or AlgorithmFactory::Instance().create?
            // https://github.com/mantidproject/mantid/blob/eaa3bd10b5a8dc847de16dabecd95314f73f6dd2/Framework/DataHandling/src/MoveInstrumentComponent.cpp
            auto moveAlg = createChildAlgorithm("MoveInstrumentComponent");
            moveAlg -> initialize();
            moveAlg -> setProperty("Workspace", wsName);
            auto componentName = calTable->getColumn(0);
            for (size_t row_num=0; row_num < calTable->rowCount(); row_num++) {
                moveAlg->setProperty("ComponentName", componentNames->cell<std::string>(row_num));
                moveAlg->setProperty("X", x_poss->cell<double>(row_num));
                moveAlg->setProperty("Y", y_poss->cell<double>(row_num));
                moveAlg->setProperty("Z", z_poss->cell<double>(row_num));
                // [IMPORTANT] the position data from calibration table are ABSOLUTE values w.r.t. the sample
                moveAlg->setProperty("RelativePosition", false);
                moveAlg->execute();
            }

            // Rotate each component in the instrument
            g_log.notice() << "Rotating each component using given Calibration table";
            // https://github.com/mantidproject/mantid/blob/eaa3bd10b5a8dc847de16dabecd95314f73f6dd2/Framework/DataHandling/src/RotateInstrumentComponent.cpp
            auto rotateAlg = createChildAlgorithm("RotateInstrumentComponent");
            rotateAlg -> initialize();
            rotateAlg->setProperty("Workspace", wsName);
            for (size_t row_num=0; row_num < calTable->rowCount(); row_num++) {
                if (abs(rotangs->cell<double>(row_num)) < 1e-8){
                    continue;
                }
                rotateAlg->setProperty("ComponentName", componentNames->cell<std::string>(row_num));
                rotateAlg->setProperty("X", rotxs->cell<double>(row_num));
                rotateAlg->setProperty("Y", rotys->cell<double>(row_num));
                rotateAlg->setProperty("Z", rotzs->cell<double>(row_num));
                rotateAlg->setProperty("Angle", rotangs->cell<double>(row_num));
                // [IMPORTANT] The rotation required here has to be the ABSOLUTE rotation angle
                rotateAlg->setProperty("RelativeRotation", false);
                rotateAlg->execute();
            }

            // Config output
            setProperty("Workspace", ws);

            g_log.notice() << "Finished applying CORELLI calibration\n";
        }

    } // namespace Algorithm

} // namespace Mantid
