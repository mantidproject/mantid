#include "MantidAlgorithms/TimeAtSampleStrategyIndirect.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <cmath>
#include <boost/shared_ptr.hpp>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid {

namespace API {
class MatrixWorkspace;
}

namespace Algorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
TimeAtSampleStrategyIndirect::TimeAtSampleStrategyIndirect(
    MatrixWorkspace_const_sptr ws)
    : m_ws(ws) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
TimeAtSampleStrategyIndirect::~TimeAtSampleStrategyIndirect() {}

Correction
TimeAtSampleStrategyIndirect::calculate(const size_t &workspace_index) const {

  // A constant among all spectra
  double twomev_d_mass =
      2. * PhysicalConstants::meV / PhysicalConstants::NeutronMass;
  V3D samplepos = m_ws->getInstrument()->getSample()->getPos();

  // Get the parameter map
  const ParameterMap &pmap = m_ws->constInstrumentParameters();

  double shift;
  IDetector_const_sptr det = m_ws->getDetector(workspace_index);
  if (!det->isMonitor()) {
    // Get E_fix
    double efix = 0.;
    try {
      Parameter_sptr par = pmap.getRecursive(det.get(), "Efixed");
      if (par) {
        efix = par->value<double>();
      }
    } catch (std::runtime_error &) {
      // Throws if a DetectorGroup, use single provided value
      std::stringstream errmsg;
      errmsg << "Inelastic instrument detector " << det->getID()
             << " of spectrum " << workspace_index << " does not have EFixed ";
      throw std::runtime_error(errmsg.str());
    }

    // Get L2
    double l2 = det->getPos().distance(samplepos);

    // Calculate shift
    shift = -1. * l2 / sqrt(efix * twomev_d_mass);

  } else {
    std::stringstream errormsg;
    errormsg << "Workspace index " << workspace_index << " is a monitor. ";
    throw std::invalid_argument(errormsg.str());
  }
  return Correction(1.0, shift);
}

} // namespace Algorithms
} // namespace Mantid
