// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Logger.h"
#include "MantidCrystal/SCDCalibratePanels2.h"


namespace Mantid {
namespace Crystal {

    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::Kernel;

    /// Config logger
    namespace {
    Logger logger("SCDCalibratePanels2");
    }

    DECLARE_ALGORITHM(SCDCalibratePanels2)

    /**
     * @brief Initialization
     * 
     */
    void SCDCalibratePanels2::init() {
        // Input peakworkspace
        declareProperty(
            std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                "PeakWorkspace", 
                "",
                Kernel::Direction::Input),
            "Workspace of Indexed Peaks");

        // Lattice constant group
        auto mustBePositive = std::make_shared<BoundedValidator<double>>();
        mustBePositive->setLower(0.0);
        declareProperty("a", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter a (Leave empty to use lattice constants "
                        "in peaks workspace)");
        declareProperty("b", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter b (Leave empty to use lattice constants "
                        "in peaks workspace)");
        declareProperty("c", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter c (Leave empty to use lattice constants "
                        "in peaks workspace)");
        declareProperty("alpha", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter alpha in degrees (Leave empty to use "
                        "lattice constants in peaks workspace)");
        declareProperty("beta", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter beta in degrees (Leave empty to use "
                        "lattice constants in peaks workspace)");
        declareProperty("gamma", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter gamma in degrees (Leave empty to use "
                        "lattice constants in peaks workspace)");
        const std::string LATTICE("Lattice Constants");
        setPropertyGroup("a", LATTICE);
        setPropertyGroup("b", LATTICE);
        setPropertyGroup("c", LATTICE);
        setPropertyGroup("alpha", LATTICE);
        setPropertyGroup("beta", LATTICE);
        setPropertyGroup("gamma", LATTICE);

        // Calibration options group
        declareProperty("CalibrateT0", false, "Calibrate the T0 (initial TOF)");
        declareProperty("CalibrateL1", true, "Change the L1(source to sample) distance");
        declareProperty("CalibrateBanks", true, "Calibrate position and orientation of each bank.");
        // TODO:
        //     Once the core functionality of calibration is done, we can consider adding the
        //     following control calibration parameters.
        // declareProperty("EdgePixels", 0, "Remove peaks that are at pixels this close to edge. ");
        // declareProperty("ChangePanelSize", true, 
        //                 "Change the height and width of the "
        //                 "detectors.  Implemented only for "
        //                 "RectangularDetectors.");
        // declareProperty("CalibrateSNAPPanels", false,
        //                 "Calibrate the 3 X 3 panels of the "
        //                 "sides of SNAP.");
        const std::string PARAMETERS("Calibration Parameters");
        setPropertyGroup("CalibrateT0" ,PARAMETERS);
        setPropertyGroup("CalibrateL1" ,PARAMETERS);
        setPropertyGroup("CalibrateBanks" ,PARAMETERS);

        // Output options group
        const std::vector<std::string> detcalExts{".DetCal", ".Det_Cal"};
        declareProperty(
            std::make_unique<FileProperty>("DetCalFilename", "SCDCalibrate2.DetCal",
                                           FileProperty::OptionalSave, detcalExts),
            "Path to an ISAW-style .detcal file to save.");

        declareProperty(
            std::make_unique<FileProperty>("XmlFilename", "SCDCalibrate2.xml",
                                            FileProperty::OptionalSave, ".xml"),
            "Path to an Mantid .xml description(for LoadParameterFile) file to "
            "save.");
        // NOTE: we need to make some significant changes to the output interface considering
        //       50% of the time is spent on writing to file for the version 1.
        // Tentative options: all calibration output should be stored as a group workspace
        //                    for interactive analysis
        //  - peak positions comparison between theoretical and measured
        //  - TOF comparision between theoretical and measured
        const std::string OUTPUT("Output");
        setPropertyGroup("DetCalFilename", OUTPUT);
        setPropertyGroup("XmlFilename", OUTPUT);
    }

    /**
     * @brief validate inputs
     * 
     * @return std::map<std::string, std::string> 
     */
    std::map<std::string, std::string>
    SCDCalibratePanels2::validateInputs() {
        std::map<std::string, std::string> issues;

        return issues;
    }

    /**
     * @brief execute calibration
     * 
     */
    void SCDCalibratePanels2::exec() {
        int a = 1;
    }

    /// Core functions for calibration&optimizatoin

    /// helper functions

} // namespace Crystal
} // namespace Mantid
