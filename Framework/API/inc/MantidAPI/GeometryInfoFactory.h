#ifndef MANTID_API_GEOMETRYINFOFACTORY_H_
#define MANTID_API_GEOMETRYINFOFACTORY_H_

#include <boost/shared_ptr.hpp>

#include "MantidKernel/V3D.h"
#include "MantidAPI/DllConfig.h"

namespace Mantid {

namespace Geometry {
class Instrument;
class IComponent;
}

namespace API {
class MatrixWorkspace;

class MANTID_API_DLL GeometryInfoFactory {
public:
  GeometryInfoFactory(const MatrixWorkspace &workspace);

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

#endif /* MANTID_API_GEOMETRYINFOFACTORY_H_ */
