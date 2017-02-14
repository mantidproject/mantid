#ifndef MANTID_DATAOBJECTS_VIRTUAL_PEAK_H_
#define MANTID_DATAOBJECTS_VIRTUAL_PEAK_H_

#include "MantidDataObjects/Peak.h"

namespace Mantid {
namespace DataObjects {

/** Structure describing a virtual single-crystal peak
 */
class DLLExport VirtualPeak : public Peak {
public:

  // Override the detector position
  Mantid::Kernel::V3D getDetPos() const override;

};
}
}

#endif /* MANTID_DATAOBJECTS_VIRTUAL_PEAK_H_ */
