#include "MantidDataObjects/VirtualPeak.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Kernel;

namespace Mantid {
namespace DataObjects {

//----------------------------------------------------------------------------------------------
/** Constructor that uses the Q position of the peak (in the lab frame).
 * No detector ID is set.
 *
 * @param m_inst :: Shared pointer to the instrument for this peak detection
 * @param QLabFrame :: Q of the center of the peak, in reciprocal space
 * @param detectorDistance :: Optional distance between the sample and the
 *detector. Calculated if not explicitly provided.
 *        Used to give a valid TOF. Default 1.0 meters.
 */
VirtualPeak::VirtualPeak(const Geometry::Instrument_const_sptr &m_inst,
           const Mantid::Kernel::V3D &QLabFrame,
           boost::optional<double> detectorDistance) : Peak(m_inst, QLabFrame, detectorDistance) {
}

V3D VirtualPeak::getDetPos() const {
    return V3D(0, 0, 0);
}

}
}
