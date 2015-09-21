#include "MantidKernel/MDUnitFactory.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabel.h"
#include "MantidKernel/UnitLabelTypes.h"
#include <memory>
#include <boost/regex.hpp>

namespace Mantid {
namespace Kernel {

LabelUnit *LabelUnitFactory::createRaw(const std::string &unitString) const {
  return new LabelUnit(UnitLabel(unitString));
}

bool LabelUnitFactory::canInterpret(const std::string &) const {
  return true; // Can always treat a unit as a label unit.
}

InverseAngstromsUnit *
InverseAngstromsUnitFactory::createRaw(const std::string &) const {
  return new InverseAngstromsUnit;
}

bool
InverseAngstromsUnitFactory::canInterpret(const std::string &unitString) const {
    boost::regex pattern("(Angstrom\\^-1)");
    boost::smatch match; // Unused.
    return boost::regex_search(unitString, match, pattern);
}

ReciprocalLatticeUnit *
ReciprocalLatticeUnitFactory::createRaw(const std::string &) const {
  return new ReciprocalLatticeUnit;
}

bool ReciprocalLatticeUnitFactory::canInterpret(
    const std::string &unitString) const {
  return unitString == Units::Symbol::RLU.ascii();
}

MDUnitFactory_uptr makeMDUnitFactoryChain() {
  typedef MDUnitFactory_uptr FactoryType;
  auto first = FactoryType(new InverseAngstromsUnitFactory);
  first->setSuccessor(FactoryType(new ReciprocalLatticeUnitFactory))
   // Add more factories here! 
   // Make sure that LabelUnitFactory is the last in the chain to give a fall through
    .setSuccessor(FactoryType(new LabelUnitFactory));
  return first;
}

} // namespace Kernel
} // namespace Mantid
