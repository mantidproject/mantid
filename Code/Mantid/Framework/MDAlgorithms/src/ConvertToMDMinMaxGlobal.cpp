#include "MantidMDAlgorithms/ConvertToMDMinMaxGlobal.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ListValidator.h"
#include "MantidMDEvents/ConvToMDSelector.h"
#include "MantidMDEvents/MDWSTransform.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToMDMinMaxGlobal)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertToMDMinMaxGlobal::ConvertToMDMinMaxGlobal() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToMDMinMaxGlobal::~ConvertToMDMinMaxGlobal() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ConvertToMDMinMaxGlobal::name() const {
  return "ConvertToMDMinMaxGlobal";
}

/// Algorithm's version for identification. @see Algorithm::version
int ConvertToMDMinMaxGlobal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertToMDMinMaxGlobal::category() const {
  return "MDAlgorithms";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToMDMinMaxGlobal::init() {
  auto ws_valid = boost::make_shared<CompositeValidator>();
  //
  ws_valid->add<InstrumentValidator>();
  // the validator which checks if the workspace has axis and any units
  ws_valid->add<WorkspaceUnitValidator>("");
  // histogram needed by ConvertUnits
  ws_valid->add<HistogramValidator>();
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                             Direction::Input, ws_valid),
      "An input Matrix Workspace (Workspace2D or Event workspace) ");

  std::vector<std::string> Q_modes =
      MDEvents::MDTransfFactory::Instance().getKeys();
  // something to do with different moments of time when algorithm or test loads
  // library. To avoid empty factory always do this.
  if (Q_modes.empty())
    Q_modes.assign(1, "ERROR IN LOADING Q-converters");

  /// this variable describes default possible ID-s for Q-dimensions
  declareProperty(
      "QDimensions", Q_modes[0],
      boost::make_shared<StringListValidator>(Q_modes),
      "String, describing MD-analysis modes, this algorithm can process. "
      "There are 3 modes currently available and described in details on"
      "*MD Transformation factory* page. "
      "The modes names are **CopyToMD**, **|Q|** and **Q3D**",
      Direction::InOut);
  /// temporary, until dEMode is not properly defined on Workspace
  std::vector<std::string> dE_modes = Kernel::DeltaEMode().availableTypes();
  declareProperty("dEAnalysisMode", dE_modes[Kernel::DeltaEMode::Direct],
                  boost::make_shared<StringListValidator>(dE_modes),
                  "You can analyze neutron energy transfer in **Direct**, "
                  "**Indirect** or **Elastic** mode. "
                  "The analysis mode has to correspond to experimental set up. "
                  "Selecting inelastic mode increases "
                  "the number of the target workspace dimensions by one. See "
                  "*MD Transformation factory* for further details.",
                  Direction::InOut);

  setPropertySettings(
      "dEAnalysisMode",
      new VisibleWhenProperty("QDimensions", IS_NOT_EQUAL_TO, "CopyToMD"));

  std::vector<std::string> TargFrames;
  TargFrames.push_back("AutoSelect");
  TargFrames.push_back("Q");
  TargFrames.push_back("HKL");
  declareProperty(
      "Q3DFrames", "AutoSelect",
      boost::make_shared<StringListValidator>(TargFrames),
      "What will be the Q-dimensions of the output workspace in **Q3D** case?"
      "  **AutoSelect**: **Q** by default, **HKL** if sample has a UB matrix."
      "  **Q** - momentum in inverse angstroms. Can be used for both "
      "laboratory or sample frame."
      "  **HKL** - reciprocal lattice units");

  setPropertySettings(
      "Q3DFrames", new VisibleWhenProperty("QDimensions", IS_EQUAL_TO, "Q3D"));

  declareProperty(
      new ArrayProperty<std::string>("OtherDimensions", Direction::Input),
      "List(comma separated) of additional to **Q** and **DeltaE** variables "
      "which form additional "
      "(orthogonal) to **Q** dimensions in the target workspace (e.g. "
      "Temperature or Magnetic field). "
      "These variables had to be logged during experiment and the names of "
      "these variables have to coincide "
      "with the log names for the records of these variables in the source "
      "workspace.");

  declareProperty(new ArrayProperty<double>("MinValues", Direction::Output));
  declareProperty(new ArrayProperty<double>("MaxValues", Direction::Output));
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertToMDMinMaxGlobal::exec() {
  std::vector<double> MinValues, MaxValues;
  std::string QDimension = getPropertyValue("QDimensions");
  std::string GeometryMode = getPropertyValue("dEAnalysisMode");
  std::string Q3DFrames = getPropertyValue("Q3DFrames");
  std::vector<std::string> OtherDimensions = getProperty("OtherDimensions");

  MatrixWorkspace_sptr ws = getProperty("InputWorkspace"), wstemp;
  DataObjects::EventWorkspace_sptr evWS;

  if (QDimension == "CopyToMD") {
    double xmin, xmax;
    ws->getXMinMax(xmin, xmax);
    MinValues.push_back(xmin);
    MaxValues.push_back(xmax);
  } else // need to calculate the appropriate q values
  {
    double qmax, deltaEmax, deltaEmin;
    IAlgorithm_sptr conv = createChildAlgorithm("ConvertUnits", 0.0, 0.9);
    conv->setProperty<MatrixWorkspace_sptr>("InputWorkspace", ws);
    conv->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", wstemp);
    // Calculate maxumum momentum transfer Q
    if (GeometryMode == "Elastic") {
      conv->setProperty("Target", "Momentum");
      conv->setProperty("Emode", "Elastic");
      conv->executeAsChildAlg();

      wstemp = conv->getProperty("OutputWorkspace");
      evWS = boost::dynamic_pointer_cast<Mantid::DataObjects::EventWorkspace>(
          wstemp);
      if (evWS) {
        qmax = evWS->getTofMax() *
               2; // assumes maximum scattering angle 180 degrees
      } else {
        qmax = wstemp->getXMax() *
               2.; // assumes maximum scattering angle 180 degrees
      }
    } else // inelastic
    {
      conv->setProperty("Target", "DeltaE");
      conv->setProperty("Emode", GeometryMode);
      conv->executeAsChildAlg();
      wstemp = conv->getProperty("OutputWorkspace");
      evWS = boost::dynamic_pointer_cast<Mantid::DataObjects::EventWorkspace>(
          wstemp);
      if (evWS) {
        deltaEmin = evWS->getTofMin();
        deltaEmax = evWS->getTofMax();
      } else {
        wstemp->getXMinMax(deltaEmin, deltaEmax);
      }

      // Deal with nonphysical energies - conversion to DeltaE yields +-DBL_MAX
      if (deltaEmin < -DBL_MAX / 2)
        deltaEmin = -deltaEmax;
      if (deltaEmax > DBL_MAX / 2)
        deltaEmax = -deltaEmin;

      // Conversion constant for E->k. k(A^-1) = sqrt(energyToK*E(meV))
      const double energyToK = 8.0 * M_PI * M_PI *
                               PhysicalConstants::NeutronMass *
                               PhysicalConstants::meV * 1e-20 /
                               (PhysicalConstants::h * PhysicalConstants::h);
      if (GeometryMode == "Direct") {
        double Ei = boost::lexical_cast<double, std::string>(
            ws->run().getProperty("Ei")->value());
        qmax =
            std::sqrt(energyToK * Ei) + std::sqrt(energyToK * (Ei - deltaEmin));
      } else // indirect
      {
        double Ef = -DBL_MAX, Eftemp = Ef;
        const Geometry::ParameterMap &pmap = ws->constInstrumentParameters();
        for (size_t i = 0; i < ws->getNumberHistograms(); i++) {
          Geometry::IDetector_const_sptr spDet;
          try {
            spDet = ws->getDetector(i);
            Geometry::Parameter_sptr par =
                pmap.getRecursive(spDet.get(), "eFixed");
            if (par)
              Eftemp = par->value<double>();
            if (Eftemp > Ef)
              Ef = Eftemp;
          } catch (...) {
            continue;
          }
          if (Ef <= 0) {
            throw std::runtime_error("Could not find a fixed final energy for "
                                     "indirect geometry instrument.");
          }
        }
        qmax =
            std::sqrt(energyToK * Ef) + std::sqrt(energyToK * (Ef + deltaEmax));
      }
    }
    // Calculate limits from qmax
    if (QDimension == "|Q|") {
      MinValues.push_back(0.);
      MaxValues.push_back(qmax);
    } else // Q3D
    {
      // Q in angstroms
      if ((Q3DFrames == "Q") || ((Q3DFrames == "AutoSelect") &&
                                 (!ws->sample().hasOrientedLattice()))) {
        MinValues.push_back(-qmax);
        MinValues.push_back(-qmax);
        MinValues.push_back(-qmax);
        MaxValues.push_back(qmax);
        MaxValues.push_back(qmax);
        MaxValues.push_back(qmax);
      } else // HKL
      {
        if (!ws->sample().hasOrientedLattice()) {
          g_log.error() << "Sample has no oriented lattice" << std::endl;
          throw std::invalid_argument("No UB set");
        }
        Mantid::Geometry::OrientedLattice ol =
            ws->sample().getOrientedLattice();
        qmax /= (2. * M_PI);
        MinValues.push_back(-qmax * ol.a());
        MinValues.push_back(-qmax * ol.b());
        MinValues.push_back(-qmax * ol.c());
        MaxValues.push_back(qmax * ol.a());
        MaxValues.push_back(qmax * ol.b());
        MaxValues.push_back(qmax * ol.c());
      }
    }

    // Push deltaE if necessary
    if (GeometryMode != "Elastic") {
      MinValues.push_back(deltaEmin);
      MaxValues.push_back(deltaEmax);
    }
  }

  for (size_t i = 0; i < OtherDimensions.size(); ++i) {
    if (!ws->run().hasProperty(OtherDimensions[i])) {
      g_log.error() << "The workspace does not have a property "
                    << OtherDimensions[i] << std::endl;
      throw std::invalid_argument("Property not found. Please see error log.");
    }
    Kernel::Property *pProperty = (ws->run().getProperty(OtherDimensions[i]));
    TimeSeriesProperty<double> *p =
        dynamic_cast<TimeSeriesProperty<double> *>(pProperty);
    if (p) {
      MinValues.push_back(p->getStatistics().minimum);
      MaxValues.push_back(p->getStatistics().maximum);
    } else // it may be not a time series property but just number property
    {
      Kernel::PropertyWithValue<double> *p =
          dynamic_cast<Kernel::PropertyWithValue<double> *>(pProperty);
      if (!p) {
        std::string ERR =
            " Can not interpret property, used as dimension.\n Property: " +
            OtherDimensions[i] + " is neither a time series (run) property nor "
                                 "a property with value<double>";
        throw(std::invalid_argument(ERR));
      }
      double val = *p;
      MinValues.push_back(val);
      MaxValues.push_back(val);
    }
  }

  setProperty("MinValues", MinValues);
  setProperty("MaxValues", MaxValues);
}

} // namespace MDAlgorithms
} // namespace Mantid
