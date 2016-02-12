#ifndef MANTID_ALGORITHMS_BASICINSTRUMENT_INFO_H_
#define MANTID_ALGORITHMS_BASICINSTRUMENT_INFO_H_

#include <boost/shared_ptr.hpp>

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

private:
  boost::shared_ptr<const Geometry::Instrument> m_instrument;
  boost::shared_ptr<const Geometry::IComponent> m_source;
  boost::shared_ptr<const Geometry::IComponent> m_sample;
};
}
}

#endif /* MANTID_ALGORITHMS_BASICINSTRUMENT_INFO_H_ */
