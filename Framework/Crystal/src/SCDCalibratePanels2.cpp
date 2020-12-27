// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Logger.h"
#include "MantidCrystal/SCDCalibratePanels2.h"

#include <boost/container/flat_set.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace Mantid {
namespace Crystal {

    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::Geometry;
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
        PeaksWorkspace_sptr peaksWs = getProperty("PeakWorkspace");
    }

    /// ------------------------------------------- ///
    /// Core functions for Calibration&Optimizatoin ///
    /// ------------------------------------------- ///

    /// ---------------- ///
    /// helper functions ///
    /// ---------------- ///

    /**
     * Saves the new instrument to an xml file that can be used with the
     * LoadParameterFile Algorithm. If the filename is empty, nothing gets done.
     *
     * @param FileName     The filename to save this information to
     *
     * @param AllBankNames The names of the banks in each group whose values are
     *                     to be saved to the file
     *
     * @param instrument   The instrument with the new values for the banks in
     *                     Groups
    */
    void SCDCalibratePanels2::saveXmlFile(
        const std::string &FileName,
        const boost::container::flat_set<std::string> &AllBankNames,
        const Instrument &instrument) {
        //
        g_log.notice() << "Generating xml tree" << "\n";

        // use Boost.PropertyTree to handle XML generation
        boost::property_tree::ptree tree;
        tree.add("parameter-file.<xmlattr>.instrument",
                 instrument.getName());
        tree.add("parameter-file.<xmlattr>.valid-from",
                 instrument.getValidFromDate().toISO8601String());
        
        // add element node for each detector as component-link
        for (auto bankName : AllBankNames) {
            // Prepare data for node
            if (instrument.getName().compare("CORELLI") == 0)
                bankName.append("/sixteenpack");
            std::shared_ptr<const IComponent> bank = instrument.getComponentByName(bankName);
            Quat relRot = bank->getRelativeRot();
            std::vector<double> relRotAngles = relRot.getEulerAngles("XYZ");
            V3D pos1 = bank->getRelativePos();
            // TODO: no handling of scaling for now, will add back later
            double scalex = 1.0;
            double scaley = 1.0;

            //add nodes for each component
            boost::property_tree::ptree cmplink = 
                tree.add("parameter-file.component-link.<xmlattr>.name", bankName);
            boost::property_tree::ptree pararotx = 
                cmplink.add("parameter.<xmlattr>.name", "rotx");
            pararotx.add("value.<xmlattr>.val", relRotAngles[0]);
            boost::property_tree::ptree pararoty = 
                cmplink.add("parameter.<xmlattr>.name", "roty");
            pararoty.add("value.<xmlattr>.val", relRotAngles[1]);
            boost::property_tree::ptree pararotz = 
                cmplink.add("parameter.<xmlattr>.name", "rotz");
            pararotz.add("value.<xmlattr>.val", relRotAngles[2]);
            boost::property_tree::ptree parax = 
                cmplink.add("parameter.<xmlattr>.name", "x");
            parax.add("value.<xmlattr>.val", pos1.X());
            boost::property_tree::ptree paray = 
                cmplink.add("parameter.<xmlattr>.name", "y");
            paray.add("value.<xmlattr>.val", pos1.Y());
            boost::property_tree::ptree paraz = 
                cmplink.add("parameter.<xmlattr>.name", "z");
            paraz.add("value.<xmlattr>.val", pos1.Z());
            // TODO: default to 1 for now
            boost::property_tree::ptree parascalex = 
                cmplink.add("parameter.<xmlattr>.name", "scalex");
            parascalex.add("value.<xmlattr>.val", scalex);
            boost::property_tree::ptree parascaley = 
                cmplink.add("parameter.<xmlattr>.name", "scaley");
            parascaley.add("value.<xmlattr>.val", scaley);
        }

        // add the source as the final component-link
        // -- get positional data from source
        IComponent_const_sptr source = instrument.getSource();
        V3D sourceRelPos = source->getRelativePos();
        // -- add date to node
        boost::property_tree::ptree cmplink_src = 
            tree.add("parameter-file.component-link.<xmlattr>.name", source->getName());
        boost::property_tree::ptree parax_src =
            cmplink_src.add("parameter.<xmlattr>.name", "x");
        parax_src.add("value.<xmlattr>.val", sourceRelPos.X());
        boost::property_tree::ptree paray_src =
            cmplink_src.add("parameter.<xmlattr>.name", "y");
        paray_src.add("value.<xmlattr>.val", sourceRelPos.Y());
        boost::property_tree::ptree paraz_src =
            cmplink_src.add("parameter.<xmlattr>.name", "z");
        paraz_src.add("value.<xmlattr>.val", sourceRelPos.Z());

        // write the xml tree to disk
        g_log.notice() << "\tSaving parameter file as " << FileName << "\n";
        boost::property_tree::write_xml(
            FileName, tree, std::locale(),
            boost::property_tree::xml_writer_settings<std::string>(' ', 2));
    }

    /**
     * Really this is the operator SaveIsawDetCal but only the results of the given
     * banks are saved.  L1 and T0 are also saved.
     *
     * @param instrument   -The instrument with the correct panel geometries
     *                      and initial path length
     * @param AllBankName  -the set of the NewInstrument names of the banks(panels)
     * @param T0           -The time offset from the DetCal file
     * @param filename     -The name of the DetCal file to save the results to
     */
    void SCDCalibratePanels2::saveIsawDetCal(
        std::shared_ptr<Instrument> &instrument,
        boost::container::flat_set<std::string> &AllBankName, double T0,
        const std::string &filename) {
        g_log.notice() << "Saving DetCal file in " << filename << "\n";

        // create a workspace to pass to SaveIsawDetCal
        const size_t number_spectra = instrument->getNumberDetectors();
        Workspace2D_sptr wksp =
            std::dynamic_pointer_cast<Workspace2D>(
                WorkspaceFactory::Instance().create("Workspace2D", number_spectra, 2,
                                                    1));
        wksp->setInstrument(instrument);
        wksp->rebuildSpectraMapping(true /* include monitors */);

        // convert the bank names into a vector
        std::vector<std::string> banknames(AllBankName.begin(), AllBankName.end());

        // call SaveIsawDetCal
        API::IAlgorithm_sptr alg = createChildAlgorithm("SaveIsawDetCal");
        alg->setProperty("InputWorkspace", wksp);
        alg->setProperty("Filename", filename);
        alg->setProperty("TimeOffset", T0);
        alg->setProperty("BankNames", banknames);
        alg->executeAsChildAlg();
    }

} // namespace Crystal
} // namespace Mantid
