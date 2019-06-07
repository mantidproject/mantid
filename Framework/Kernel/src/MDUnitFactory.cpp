// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/MDUnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"

#include <boost/regex.hpp>

namespace Mantid {
namespace Kernel {

LabelUnit *LabelUnitFactory::createRaw(const std::string &unitString) const {
  return new LabelUnit(UnitLabel(unitString));
}

bool LabelUnitFactory::canInterpret(const std::string & /*unitString*/) const {
  return true; // Can always treat a unit as a label unit.
}

InverseAngstromsUnit *InverseAngstromsUnitFactory::createRaw(
    const std::string & /*unitString*/) const {
  return new InverseAngstromsUnit;
}

bool InverseAngstromsUnitFactory::canInterpret(
    const std::string &unitString) const {
  boost::regex pattern("(Angstrom\\^-1)");
  boost::regex pattern2("A\\^-1");
  boost::smatch match; // Unused.

  auto isFullAngstrom = boost::regex_search(unitString, match, pattern);
  auto isPartialAngstrom = boost::regex_search(unitString, match, pattern2);

  return isFullAngstrom || isPartialAngstrom;
}

ReciprocalLatticeUnit *
ReciprocalLatticeUnitFactory::createRaw(const std::string &unitString) const {
  return new ReciprocalLatticeUnit(UnitLabel(unitString));
}

bool ReciprocalLatticeUnitFactory::canInterpret(
    const std::string &unitString) const {
  auto isRLU = unitString == Units::Symbol::RLU.ascii();

  // In addition to having RLU we can encounter units of type "in 6.28 A^-1"
  // We need to account for the latter here
  boost::regex pattern("in.*A.*\\^-1");
  auto isHoraceStyle = boost::regex_match(unitString, pattern);
  return isRLU || isHoraceStyle;
}

MDUnitFactory_uptr makeMDUnitFactoryChain() {
  MDUnitFactory_uptr first = std::make_unique<ReciprocalLatticeUnitFactory>();
  first
      ->setSuccessor(std::make_unique<InverseAngstromsUnitFactory>())
      // Add more factories here!
      // Make sure that LabelUnitFactory is the last in the chain to give a fall
      // through
      .setSuccessor(std::make_unique<LabelUnitFactory>());
  return first;
}

} // namespace Kernel
} // namespace Mantid
