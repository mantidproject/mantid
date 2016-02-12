#ifndef MANTID_ALGORITHMS_BASICINSTRUMENT_INFO_H_
#define MANTID_ALGORITHMS_BASICINSTRUMENT_INFO_H_

#include <boost/shared_ptr.hpp>

#include "MantidKernel/V3D.h"

namespace Mantid {

namespace Geometry {
class Instrument;
class IComponent;
}

namespace API {
class MatrixWorkspace;
}

namespace Algorithms {

class BasicInstrumentInfo {
public:
  BasicInstrumentInfo(const API::MatrixWorkspace &workspace);

  const Geometry::Instrument &getInstrument() const;
  const Geometry::IComponent &getSource() const;
  const Geometry::IComponent &getSample() const;
  Kernel::V3D getSourcePos() const;
  Kernel::V3D getSamplePos() const;
  double getL1() const;

private:
  boost::shared_ptr<const Geometry::Instrument> m_instrument;
  boost::shared_ptr<const Geometry::IComponent> m_source;
  boost::shared_ptr<const Geometry::IComponent> m_sample;
  Kernel::V3D m_sourcePos;
  Kernel::V3D m_samplePos;
  double m_L1;
};
}
}

#endif /* MANTID_ALGORITHMS_BASICINSTRUMENT_INFO_H_ */
