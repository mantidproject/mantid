// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/MDUnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidKernel/make_unique.h"
#include <boost/regex.hpp>
namespace Mantid {
namespace Geometry {

GeneralFrame *
GeneralFrameFactory::createRaw(const MDFrameArgument &argument) const {
  using namespace Mantid::Kernel;

  // Try to generate a proper md unit, don't just assume a label unit.
  auto unitFactoryChain = Kernel::makeMDUnitFactoryChain();
  auto mdUnit = unitFactoryChain->create(argument.unitString);

  return new GeneralFrame(argument.frameString, MDUnit_uptr(mdUnit->clone()));
}

/// Indicate an ability to intepret the string
bool GeneralFrameFactory::canInterpret(const MDFrameArgument &argument) const {
  auto canInterpret = true;
  if (argument.frameString == UnknownFrame::UnknownFrameName) {
    canInterpret = false;
  }
  return canInterpret;
}

QLab *QLabFrameFactory::createRaw(const MDFrameArgument & /*argument*/) const {
  return new QLab;
}

bool QLabFrameFactory::canInterpret(const MDFrameArgument &argument) const {
  // We only need to check the frame QLab only makes sense in inverse Angstroms
  return argument.frameString == QLab::QLabName;
}

QSample *
QSampleFrameFactory::createRaw(const MDFrameArgument & /*argument*/) const {
  return new QSample;
}

bool QSampleFrameFactory::canInterpret(const MDFrameArgument &argument) const {
  // We only need to check the frame QSample only makes sense in inverse
  // Angstroms
  return argument.frameString == QSample::QSampleName;
}

HKL *HKLFrameFactory::createRaw(const MDFrameArgument &argument) const {
  using namespace Mantid::Kernel;
  auto unitFactoryChain = Kernel::makeMDUnitFactoryChain();
  auto productMDUnit = unitFactoryChain->create(argument.unitString);
  return new HKL(productMDUnit);
}

bool HKLFrameFactory::canInterpret(const MDFrameArgument &argument) const {
  using namespace Mantid::Kernel;
  auto unitFactoryChain = Kernel::makeMDUnitFactoryChain();
  auto mdUnit = unitFactoryChain->create(argument.unitString);
  // We expect units to be RLU or A^-1
  auto isInverseAngstrom =
      mdUnit->getUnitLabel() == Units::Symbol::InverseAngstrom;
  auto isRLU = mdUnit->getUnitLabel() == Units::Symbol::RLU;
  boost::regex pattern("in.*A.*\\^-1");
  auto isHoraceStyle =
      boost::regex_match(mdUnit->getUnitLabel().ascii(), pattern);
  const bool compatibleUnit = isInverseAngstrom || isRLU || isHoraceStyle;
  // Check both the frame name and the unit name
  return argument.frameString == HKL::HKLName && compatibleUnit;
}

UnknownFrame *
UnknownFrameFactory::createRaw(const MDFrameArgument &argument) const {
  using namespace Mantid::Kernel;

  // Try to generate a proper md unit, don't just assume a label unit.
  auto unitFactoryChain = Kernel::makeMDUnitFactoryChain();
  auto mdUnit = unitFactoryChain->create(argument.unitString);

  return new UnknownFrame(MDUnit_uptr(mdUnit->clone()));
}

/// Indicate an ability to intepret the string
bool UnknownFrameFactory::canInterpret(
    const MDFrameArgument & /*unitString*/) const {
  return true; // This can interpret everything
}

MDFrameFactory_uptr makeMDFrameFactoryChain() {
  MDFrameFactory_uptr first = Kernel::make_unique<QLabFrameFactory>();
  first->setSuccessor(Kernel::make_unique<QSampleFrameFactory>())
      .setSuccessor(Kernel::make_unique<HKLFrameFactory>())
      // Make sure that GeneralFrameFactory is the last in the chain to give a
      // fall-through option
      .setSuccessor(Kernel::make_unique<GeneralFrameFactory>())
      .setSuccessor(Kernel::make_unique<UnknownFrameFactory>());
  return first;
}

} // namespace Geometry
} // namespace Mantid
