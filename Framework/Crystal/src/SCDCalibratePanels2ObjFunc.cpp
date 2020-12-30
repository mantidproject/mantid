// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidCrystal/SCDCalibratePanels2ObjFunc.h"

namespace Mantid {
namespace Crystal {

    using namespace Mantid::API;
    using namespace Mantid::CurveFitting;
    using namespace Mantid::DataObjects;
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    namespace {
        // static logger
        Logger g_log("SCDCalibratePanels2ObjFunc");
    }

    DECLARE_FUNCTION(SCDCalibratePanels2ObjFunc)

    // --------------------------
    // ----- Core functions -----
    // --------------------------
    SCDCalibratePanels2ObjFunc::SCDCalibratePanels2ObjFunc() {
        // parameters
        declareParameter("dx", 0.0, "relative shift along X");
        declareParameter("dy", 0.0, "relative shift along Y");
        declareParameter("dz", 0.0, "relative shift along Z");
        declareParameter("drotx", 0.0, "relative rotation around X");
        declareParameter("droty", 0.0, "relative rotation around Y");
        declareParameter("drotz", 0.0, "relative rotation around Z");
        declareParameter("dT0", 0.0, "delta of TOF");
        // attributes
        declareAttribute("Workspace", Attribute(""));
        declareAttribute("ComponentName", Attribute(""));
    }

    /**
     * @brief Evalute the objective function with given feature vector X
     * 
     * @param out     :: Q_calculated
     * @param xValues :: feature vector [shiftx3, rotx3, T0]
     * @param order   :: dimensionality of feature vector
     */
    void SCDCalibratePanels2ObjFunc::function1D(
        double *out,
        const double *xValues,
        const size_t order) const {
        // Get the feature vector component (numeric type)
        //-- delta in translation
        const double dx = getParameter("dx");
        const double dy = getParameter("dy");
        const double dz = getParameter("dz");
        //-- delta in rotation/orientation
        const double drotx = getParameter("drotx");
        const double droty = getParameter("droty");
        const double drotz = getParameter("drotz");
        //-- delta in TOF
        const double dT0 = getParameter("dT0");
        //-- NOTE: given that these components are never used as
        //         one vector, there is no need to construct a
        //         xValues
        UNUSED_ARG(xValues);

        // Get workspace and component name (string type)
        m_ws = std::move(AnalysisDataService::Instance().retrieveWS<Workspace>(getAttribute("Workspace").asString()));
        m_cmpt = getAttribute("ComponentName").asString();

        // Special adjustment for CORELLI
        PeaksWorkspace_sptr pws =
            std::dynamic_pointer_cast<PeaksWorkspace>(m_ws);
        Instrument_sptr inst =
            std::const_pointer_cast<Instrument>(pws->getInstrument());
        if(inst->getName().compare("CORELLI")==0 && m_cmpt!="moderator")
            m_cmpt.append("/sixteenpack");

        // NOTE: Since the feature vectors are all deltas with respect to the starting position,
        //       we need to only operate on a copy instead of the original to avoid changing the
        //       base value
        std::shared_ptr<API::Workspace> wstmp = m_ws->clone();

        // translation
        moveInstruentComponentBy(dx, dy, dz, m_cmpt, wstmp);

        // rotation
        // NOTE: moderator should not be reoriented
        rotateInstrumentComponentBy(drotx, droty, drotz, m_cmpt, wstmp);

        //

        // TODO:
    }

    /**
     * @brief function derivatives
     * 
     * @param out      :: The output Jacobian matrix: function derivatives over its parameters
     * @param xValues  :: feature vector [shiftx3, rotx3, T0]
     * @param order    :: dimensionality of feature vector
     */
    void SCDCalibratePanels2ObjFunc::functionDeriv1D(
        API::Jacobian *out,
        const double *xValues,
        const size_t order) {
        FunctionDomain1DView domain(xValues, order);
        calNumericalDeriv(domain, *out);
    }


    // ------------------
    // ----- Helper -----
    // ------------------

    /**
     * @brief Translate the component of given workspace by delta_(x, y, z)
     * 
     * @param deltaX  :: The shift along the X-axis in m
     * @param deltaY  :: The shift along the Y-axis in m
     * @param deltaZ  :: The shift along the Z-axis in m
     * @param componentName  :: string representation of a component
     * @param ws  :: input workspace (mostly peaksworkspace)
     */
    void SCDCalibratePanels2ObjFunc::moveInstruentComponentBy(
        double deltaX, double deltaY, double deltaZ,
        std::string componentName,
        const API::Workspace_sptr &ws) const {
        // move instrument is really fast, even with zero input
        IAlgorithm_sptr mv_alg =
            Mantid::API::AlgorithmFactory::Instance().create("MoveInstrumentComponent", -1);
        mv_alg->initialize();
        mv_alg->setChild(true);
        mv_alg->setLogging(LOGCHILDALG);
        mv_alg->setProperty<Workspace_sptr>("Workspace", ws);
        mv_alg->setProperty("ComponentName", componentName);
        mv_alg->setProperty("X", deltaX);
        mv_alg->setProperty("Y", deltaY);
        mv_alg->setProperty("Z", deltaZ);
        mv_alg->setProperty("RelativePosition", true);
        mv_alg->executeAsChildAlg();
    }

    /**
     * @brief Rotate the component of given workspace by
     *          rotAngX@(1,0,0)
     *          rotAngY@(0,1,0)
     *          rotAngZ@(0,0,1)
     * 
     * @param rotAngX  :: The rotation around the X-axis
     * @param rotAngY  :: The rotation around the Y-axis
     * @param rotAngZ  :: The rotation around the Z-axis
     * @param componentName  :: string representation of a component
     * @param ws  :: input workspace (mostly peaksworkspace)
     */
    void SCDCalibratePanels2ObjFunc::rotateInstrumentComponentBy (
        double rotAngX, double rotAngY, double rotAngZ,
        std::string componentName,
        const API::Workspace_sptr &ws) const {
        // rotate
        IAlgorithm_sptr rot_alg = 
            Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
        //-- rotAngX@(1,0,0)
        rot_alg->initialize();
        rot_alg->setChild(true);
        rot_alg->setLogging(LOGCHILDALG);
        rot_alg->setProperty<Workspace_sptr>("Workspace", ws);
        rot_alg->setProperty("ComponentName", componentName);
        rot_alg->setProperty("X", 1.0);
        rot_alg->setProperty("Y", 0.0);
        rot_alg->setProperty("Z", 0.0);
        rot_alg->setProperty("Angle", rotAngX);
        rot_alg->setProperty("RelativeRotation", true);
        rot_alg->executeAsChildAlg();
        //-- rotAngY@(0,1,0)
        rot_alg->initialize();
        rot_alg->setChild(true);
        rot_alg->setLogging(LOGCHILDALG);
        rot_alg->setProperty<Workspace_sptr>("Workspace", ws);
        rot_alg->setProperty("ComponentName", componentName);
        rot_alg->setProperty("X", 0.0);
        rot_alg->setProperty("Y", 1.0);
        rot_alg->setProperty("Z", 0.0);
        rot_alg->setProperty("Angle", rotAngY);
        rot_alg->setProperty("RelativeRotation", true);
        rot_alg->executeAsChildAlg();
        //-- rotAngZ@(0,0,1)
        rot_alg->initialize();
        rot_alg->setChild(true);
        rot_alg->setLogging(LOGCHILDALG);
        rot_alg->setProperty<Workspace_sptr>("Workspace", ws);
        rot_alg->setProperty("ComponentName", componentName);
        rot_alg->setProperty("X", 0.0);
        rot_alg->setProperty("Y", 0.0);
        rot_alg->setProperty("Z", 1.0);
        rot_alg->setProperty("Angle", rotAngZ);
        rot_alg->setProperty("RelativeRotation", true);
        rot_alg->executeAsChildAlg();
    }

} // namespace Crystal
} // namespace Mantid
