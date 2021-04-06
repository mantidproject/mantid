// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/JSONInstrumentBuilder.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidNexusGeometry/InstrumentBuilder.h"
#include "MantidNexusGeometry/JSONGeometryParser.h"
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include "MantidNexusGeometry/TubeHelpers.h"
#include <json/json.h>
#include <memory>

namespace {
using namespace Mantid::Geometry;
using namespace Mantid::NexusGeometry;

IObject_const_sptr createShape(const JSONGeometryParser &parser, const size_t bank) {
  if (parser.isOffGeometry(bank))
    return NexusShapeFactory::createFromOFFMesh(parser.faces(bank), parser.windingOrder(bank), parser.vertices(bank));
  else
    return NexusShapeFactory::createCylinder(parser.cylinders(bank), parser.vertices(bank));
}

IObject_const_sptr createMonitorShape(const Monitor &monitor) {
  if (monitor.vertices.empty())
    return nullptr;

  if (monitor.isOffGeometry)
    return NexusShapeFactory::createFromOFFMesh(monitor.faces, monitor.windingOrder, monitor.vertices);
  else
    return NexusShapeFactory::createCylinder(monitor.cylinders, monitor.vertices);
}

Eigen::Matrix<double, 3, Eigen::Dynamic> getOffsets(const JSONGeometryParser &parser, const size_t index) {
  // Initialise matrix
  Eigen::Matrix<double, 3, Eigen::Dynamic> offsets;
  int rowLength = 0;
  const auto &x = parser.xPixelOffsets(index);
  const auto &y = parser.yPixelOffsets(index);
  const auto &z = parser.zPixelOffsets(index);

  if (!x.empty())
    rowLength = static_cast<int>(x.size());
  else if (!y.empty())
    rowLength = static_cast<int>(y.size());
  // Need at least 2 dimensions to define points
  else
    return offsets;

  // Default x,y,z to zero if no data provided
  offsets.resize(3, rowLength);
  offsets.setZero(3, rowLength);

  if (!x.empty()) {
    for (int i = 0; i < rowLength; i++)
      offsets(0, i) = x[i];
  }
  if (!y.empty()) {
    for (int i = 0; i < rowLength; i++)
      offsets(1, i) = y[i];
  }
  if (!z.empty()) {
    for (int i = 0; i < rowLength; i++)
      offsets(2, i) = z[i];
  }
  // Return the coordinate matrix
  return offsets;
}

Eigen::Vector3d applyRotation(const Eigen::Vector3d &pos, const Eigen::Quaterniond &rot) {
  auto transform = Eigen::Transform<double, 3, Eigen::Affine>::Identity();
  transform = rot * transform;
  return transform * pos;
}

void addMonitors(const JSONGeometryParser &parser, InstrumentBuilder &builder) {
  const auto &monitors = parser.monitors();
  for (const auto &monitor : monitors) {
    auto shape = createMonitorShape(monitor);
    auto name = monitor.componentName.empty() ? std::to_string(monitor.detectorID) : monitor.componentName;
    builder.addMonitor(name, monitor.detectorID, applyRotation(monitor.translation, monitor.orientation), shape);
  }
}

} // namespace

namespace Mantid {
namespace NexusGeometry {
JSONInstrumentBuilder::JSONInstrumentBuilder(const std::string &jsonGeometry)
    : m_parser(std::make_unique<JSONGeometryParser>(jsonGeometry)) {}

const std::vector<Chopper> &JSONInstrumentBuilder::choppers() const { return m_parser->choppers(); }

Geometry::Instrument_const_uptr JSONInstrumentBuilder::buildGeometry() const {
  InstrumentBuilder builder(m_parser->name());
  for (size_t bank = 0; bank < m_parser->numberOfBanks(); ++bank) {
    auto bankName = m_parser->detectorName(bank);
    builder.addBank(bankName, m_parser->translation(bank), m_parser->orientation(bank));
    auto shape = createShape(*m_parser, bank);
    auto pixelOffsets = getOffsets(*m_parser, bank);
    Eigen::Matrix<double, 3, Eigen::Dynamic> detectorPixels = Eigen::Affine3d::Identity() * pixelOffsets;
    const auto &ids = m_parser->detectorIDs(bank);
    if (m_parser->isOffGeometry(bank)) {
      for (size_t i = 0; i < ids.size(); ++i) {
        Eigen::Vector3d relativePos = detectorPixels.col(i);
        builder.addDetectorToLastBank(bankName + "_" + std::to_string(i), ids[i], relativePos, shape);
      }
    } else {
      auto tubes = TubeHelpers::findAndSortTubes(*shape, detectorPixels, ids);
      builder.addTubes(bankName, tubes, shape);
    }
  }

  builder.addSample(m_parser->sampleName(), applyRotation(m_parser->samplePosition(), m_parser->sampleOrientation()));
  builder.addSource(m_parser->sourceName(), applyRotation(m_parser->sourcePosition(), m_parser->sourceOrientation()));
  addMonitors(*m_parser, builder);

  return builder.createInstrument();
}

} // namespace NexusGeometry
} // namespace Mantid
