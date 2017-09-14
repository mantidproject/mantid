//--------------------
// Includes
//--------------------

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidNexusGeometry/InstrumentGeometryAbstraction.h"

namespace Mantid{
namespace NexusGeometry{

///Constructor
InstrumentGeometryAbstraction::InstrumentGeometryAbstraction(std::string &instrumentName){
    Geometry::Instrument_sptr inst_sptr(new Geometry::Instrument(instrumentName));
    this->instrument_sptr = inst_sptr;
}

///Adds component to instrument
Geometry::IComponent* InstrumentGeometryAbstraction::addComponent(std::string &compName, Eigen::Vector3d &position){
    Geometry::IComponent *component(new Geometry::ObjCompAssembly(compName));
    component->setPos(position(0), position(1), position(2));
    this->instrument_sptr->add(component);
    return component;
}

///Adds detector to instrument
void InstrumentGeometryAbstraction::addDetector(std::string &detName, int detId, Eigen::Vector3d &position){
  auto *detector(new Geometry::Detector(
      detName, detId,
      const_cast<Geometry::IComponent *>(this->instrument_sptr->getBaseComponent())));
  detector->setPos(position(0), position(1), position(2));
  this->instrument_sptr->add(detector);
  this->instrument_sptr->markAsDetectorIncomplete(detector);
}

///Sorts detectors
void InstrumentGeometryAbstraction::sortDetectors(){
    this->instrument_sptr->markAsDetectorFinalize();
}

///Add sample
void InstrumentGeometryAbstraction::addSample(std::string &sampleName, Eigen::Vector3d &position){
  auto *sample(this->addComponent(sampleName, position));
  this->instrument_sptr->markAsSamplePos(sample);
}
///Add source
void InstrumentGeometryAbstraction::addSource(std::string &sourceName, Eigen::Vector3d &position){
    auto *source(this->addComponent(sourceName, position));
    this->instrument_sptr->markAsSource(source);
}
}
}
