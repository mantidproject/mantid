// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexusGeometry/JSONInstrumentBuilder.h"
#include "MantidNexusGeometry/JSONGeometryParser.h"
#include "MantidNexusGeometry/NexusShapeFactory.h"

namespace Mantid {
namespace NexusGeometry {

JSONInstrumentBuilder::JSONInstrumentBuilder(const std::string &jsonGeometry)
    : m_parser(std::make_unique<JSONGeometryParser>(jsonGeometry)) {}

void JSONInstrumentBuilder::buildGeometry() {}
} // namespace NexusGeometry
} // namespace Mantid
