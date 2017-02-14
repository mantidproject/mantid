#ifndef MANTID_DATAOBJECTS_VIRTUAL_PEAK_H_
#define MANTID_DATAOBJECTS_VIRTUAL_PEAK_H_

#include "MantidDataObjects/Peak.h"

namespace Mantid {
namespace DataObjects {

/** Structure describing a virtual single-crystal peak
 */
class DLLExport VirtualPeak : public Peak {
public:

  VirtualPeak() {};
  VirtualPeak(const Geometry::Instrument_const_sptr &m_inst,
       const Mantid::Kernel::V3D &QLabFrame,
       boost::optional<double> detectorDistance = boost::none);

  // Override the detector position
  Mantid::Kernel::V3D getDetPos() const override;

};
}
}

#endif /* MANTID_DATAOBJECTS_VIRTUAL_PEAK_H_ */
