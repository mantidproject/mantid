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
#include "MantidAPI/SpectrumInfo.h"
#include <cmath>
#include <boost/shared_ptr.hpp>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

/** Constructor
 */
TimeAtSampleStrategyIndirect::TimeAtSampleStrategyIndirect(
    MatrixWorkspace_const_sptr ws)
    : m_ws(ws), m_spectrumInfo(m_ws->spectrumInfo()) {}

Correction
TimeAtSampleStrategyIndirect::calculate(const size_t &workspace_index) const {

  // A constant among all spectra
  double twomev_d_mass =
      2. * PhysicalConstants::meV / PhysicalConstants::NeutronMass;

  // Get the parameter map
  const ParameterMap &pmap = m_ws->constInstrumentParameters();

  double shift;
  const IDetector *det = &m_spectrumInfo.detector(workspace_index);
  if (!m_spectrumInfo.isMonitor(workspace_index)) {
    // Get E_fix
    double efix = 0.;
    try {
      Parameter_sptr par = pmap.getRecursive(det, "Efixed");
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

    double l2 = m_spectrumInfo.l2(workspace_index);
    shift = -1. * l2 / sqrt(efix * twomev_d_mass);

  } else {
    std::stringstream errormsg;
    errormsg << "Workspace index " << workspace_index << " is a monitor. ";
    throw std::invalid_argument(errormsg.str());
  }

  Correction retvalue(0, 0);
  retvalue.factor = 1.0;
  retvalue.offset = shift;

  return retvalue;
}

} // namespace Algorithms
} // namespace Mantid
