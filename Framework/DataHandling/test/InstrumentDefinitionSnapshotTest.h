// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidFrameworkTestHelpers/ScopedFileHelper.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentMetadata.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/XMLInstrumentParameter.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/DigestEngine.h>
#include <Poco/SHA1Engine.h>
#include <cxxtest/TestSuite.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Geometry::ComponentInfo;
using Mantid::Geometry::DetectorInfo;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V2D;
using Mantid::Kernel::V3D;

namespace {

/** A canonical, line-by-line description of a loaded instrument, split into sections. Each section is hashed, and
 * optionally written out in full, so that two loads of the same definition can be compared field by field: what the
 * instrument definition parser builds must not change when it is ported to BeamlineBuilder.
 */
class InstrumentSnapshot {
public:
  explicit InstrumentSnapshot(std::ostream *const full = nullptr) : m_full(full) {}

  void section(std::string const &name) {
    m_sections.emplace_back(name, std::make_unique<Poco::SHA1Engine>());
    if (m_full) {
      *m_full << "== " << name << '\n';
    }
  }

  void line(std::string const &text) {
    m_sections.back().second->update(text + '\n');
    if (m_full) {
      *m_full << text << '\n';
    }
  }

  /// "section digest" for every section, in the order written. Ends the snapshot.
  std::vector<std::string> digests() {
    std::vector<std::string> result;
    for (auto &[name, engine] : m_sections) {
      result.emplace_back(name + ' ' + Poco::DigestEngine::digestToHex(engine->digest()));
    }
    return result;
  }

private:
  std::ostream *m_full;
  std::vector<std::pair<std::string, std::unique_ptr<Poco::SHA1Engine>>> m_sections;
};

/// 12 significant digits, so that rounding in the last bits does not count as a change; -0 prints as 0.
std::string num(double const value) { return std::format("{:.12g}", value + 0.0); }
std::string num(V3D const &v) { return num(v.X()) + ',' + num(v.Y()) + ',' + num(v.Z()); }
std::string num(V2D const &v) { return num(v.X()) + ',' + num(v.Y()); }
std::string num(Quat const &q) {
  return num(q.real()) + ',' + num(q.imagI()) + ',' + num(q.imagJ()) + ',' + num(q.imagK());
}

template <class Range> std::string join(Range const &range) {
  std::string result;
  for (auto const &item : range) {
    result += (result.empty() ? "" : ",") + std::to_string(item);
  }
  return result;
}

std::string shapeDescription(ComponentInfo const &componentInfo, size_t const index) {
  if (!componentInfo.hasValidShape(index)) {
    return "none";
  }
  auto const &shape = componentInfo.shape(index);
  auto const box = shape.getBoundingBox();
  auto description =
      std::format("box={};{} material={}", num(box.minPoint()), num(box.maxPoint()), shape.material().name());
  if (auto const *const csg = dynamic_cast<Mantid::Geometry::CSGObject const *>(&shape)) {
    // One line per component, so that dumps diff line by line.
    std::string xml;
    for (auto const c : csg->getShapeXML()) {
      bool const space = std::isspace(static_cast<unsigned char>(c));
      if (!space || (!xml.empty() && xml.back() != ' ')) {
        xml += space ? ' ' : c;
      }
    }
    description += " csg=" + xml;
  } else if (auto const *const mesh = dynamic_cast<Mantid::Geometry::MeshObject const *>(&shape)) {
    Poco::SHA1Engine engine;
    for (auto const &vertex : mesh->getV3Ds()) {
      engine.update(num(vertex) + ';');
    }
    engine.update(join(mesh->getTriangles()));
    description += std::format(" mesh vertices={} triangles={} sha={}", mesh->numberOfVertices(),
                               mesh->getTriangles().size(), Poco::DigestEngine::digestToHex(engine.digest()));
  } else {
    description += std::format(" other={}", typeid(shape).name());
  }
  return description;
}

/// The sections that both the instrument and its physical instrument have.
void snapshotBeamline(InstrumentSnapshot &snapshot, std::string const &prefix, ComponentInfo const &componentInfo,
                      DetectorInfo const &detectorInfo) {
  snapshot.section(prefix + "tree");
  snapshot.line(
      std::format("size={} detectors={} root={}", componentInfo.size(), detectorInfo.size(), componentInfo.root()));
  snapshot.line(std::format("source={} sample={}",
                            componentInfo.hasSource() ? std::to_string(componentInfo.source()) : "-",
                            componentInfo.hasSample() ? std::to_string(componentInfo.sample()) : "-"));
  for (size_t i = 0; i < componentInfo.size(); ++i) {
    snapshot.line(std::format("{} {} type={} parent={} children={}", i, componentInfo.fullName(i),
                              static_cast<int>(componentInfo.componentType(i)),
                              componentInfo.hasParent(i) ? std::to_string(componentInfo.parent(i)) : "-",
                              join(componentInfo.children(i))));
  }

  snapshot.section(prefix + "geometry");
  for (size_t i = 0; i < componentInfo.size(); ++i) {
    snapshot.line(std::format("{} pos={} rot={} scale={} sideBySide={}", i, num(componentInfo.position(i)),
                              num(componentInfo.rotation(i)), num(componentInfo.scaleFactor(i)),
                              num(componentInfo.sideBySideViewPosition(i))));
  }

  snapshot.section(prefix + "shapes");
  for (size_t i = 0; i < componentInfo.size(); ++i) {
    snapshot.line(std::format("{} {}", i, shapeDescription(componentInfo, i)));
  }

  snapshot.section(prefix + "banks");
  for (size_t i = 0; i < componentInfo.size(); ++i) {
    if (componentInfo.isGridDetector(i)) {
      auto const g = componentInfo.pixelGridComponent(i);
      snapshot.line(std::format("{} grid n={},{},{} start={},{},{} step={},{},{} id={},{},{} order={}{}{} ids={}-{}", i,
                                g.nX, g.nY, g.nZ, num(g.xStart), num(g.yStart), num(g.zStart), num(g.xStep),
                                num(g.yStep), num(g.zStep), g.idStart, g.idStep, g.idStepByRow, g.idFillOrder[0],
                                g.idFillOrder[1], g.idFillOrder[2], g.minDetectorID, g.maxDetectorID));
    }
    if (componentInfo.isStructuredDetector(i)) {
      auto const &s = componentInfo.structuredGridComponent(i);
      std::string vertices;
      for (size_t v = 0; v < s.xVertices.size(); ++v) {
        vertices += num(s.xVertices[v]) + ':' + (v < s.yVertices.size() ? num(s.yVertices[v]) : "-") + ';';
      }
      snapshot.line(std::format("{} structured n={},{} id={},{},{} byFirstY={} vertices={}", i, s.nX, s.nY, s.idStart,
                                s.idStep, s.idStepByRow, s.idFillByFirstY, vertices));
    }
  }

  snapshot.section(prefix + "detectors");
  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    snapshot.line(std::format("{} id={} monitor={} masked={}", i, detectorInfo.detectorIDs()[i],
                              detectorInfo.isMonitor(i), detectorInfo.isMasked(i)));
  }
}

void snapshotParameters(InstrumentSnapshot &snapshot, ComponentInfo const &componentInfo) {
  snapshot.section("parameters");
  for (size_t i = 0; i < componentInfo.size(); ++i) {
    for (auto const &[name, parameter] : componentInfo.parameters(i)) {
      snapshot.line(std::format("{} {} type={} value={} visible={} description={}", i, name, parameter->type(),
                                parameter->asString(), parameter->visible(), parameter->getDescription()));
    }
  }

  snapshot.section("logfile parameters");
  for (auto const &[key, p] : componentInfo.logfileParameters()) {
    std::string constraint;
    for (auto const &c : p->m_constraint) {
      constraint += c + ';';
    }
    snapshot.line(std::format("{} {} log={} value={} name={} type={} tie={} constraint={} penalty={} function={} "
                              "formula={} formulaUnit={} resultUnit={} extract={} eq={} angle={} description={} "
                              "visible={}",
                              key.first, key.second, p->m_logfileID, p->m_value, p->m_paramName, p->m_type, p->m_tie,
                              constraint, p->m_penaltyFactor, p->m_fittingFunction, p->m_formula, p->m_formulaUnit,
                              p->m_resultUnit, p->m_extractSingleValueAs, p->m_eq, num(p->m_angleConvertConst),
                              p->m_description, p->m_visible));
  }
}

/// Snapshots everything a load of an instrument definition gives a workspace.
std::vector<std::string> snapshotWorkspace(MatrixWorkspace const &workspace, std::ostream *const full = nullptr) {
  InstrumentSnapshot snapshot(full);
  auto const &componentInfo = workspace.componentInfo();
  auto const &metadata = workspace.instrumentMetadata();

  snapshot.section("metadata");
  snapshot.line("name=" + componentInfo.name(componentInfo.root()));
  snapshot.line(std::format("valid={} to {}", metadata.validFromDate().toISO8601String(),
                            metadata.validToDate().toISO8601String()));
  snapshot.line(std::format("file={} view={} axis={}", std::filesystem::path(metadata.filename()).filename().string(),
                            metadata.defaultView(), metadata.defaultAxis()));
  Poco::SHA1Engine xml;
  xml.update(metadata.xmlText());
  snapshot.line("xml=" + Poco::DigestEngine::digestToHex(xml.digest()));
  auto const frame = componentInfo.referenceFrame();
  snapshot.line(std::format("frame up={} beam={} horizontal={} handedness={} origin={}", frame->pointingUpAxis(),
                            frame->pointingAlongBeamAxis(), frame->pointingHorizontalAxis(),
                            static_cast<int>(frame->getHandedness()), frame->origin()));
  snapshot.line(std::format("physical={}", metadata.hasPhysicalInstrument()));

  snapshotBeamline(snapshot, "", componentInfo, workspace.detectorInfo());
  snapshotParameters(snapshot, componentInfo);
  if (metadata.hasPhysicalInstrument()) {
    snapshotBeamline(snapshot, "physical ", metadata.physicalComponentInfo(), metadata.physicalDetectorInfo());
  }

  snapshot.section("spectra");
  for (size_t i = 0; i < workspace.getNumberHistograms(); ++i) {
    auto const &spectrum = workspace.getSpectrum(i);
    snapshot.line(std::format("{} no={} ids={}", i, spectrum.getSpectrumNo(), join(spectrum.getDetectorIDs())));
  }
  return snapshot.digests();
}

MatrixWorkspace_sptr loadEmptyInstrument(std::string const &filename) {
  Mantid::DataHandling::LoadEmptyInstrument loader;
  loader.initialize();
  loader.setChild(true);
  loader.setRethrows(true);
  loader.setPropertyValue("Filename", filename);
  loader.setPropertyValue("OutputWorkspace", "unused");
  loader.execute();
  return loader.getProperty("OutputWorkspace");
}

/// Exercises what no shipped definition file uses: a 3D grid bank, a structured bank and side-by-side positions.
std::string const bankDefinition = R"(<?xml version="1.0" encoding="UTF-8"?>
<instrument xmlns="http://www.mantidproject.org/IDF/1.0" name="SNAPSHOTBANKS" valid-from="1900-01-31 23:59:59"
            valid-to="2100-01-31 23:59:59" last-modified="2026-11-24 00:00:00">
  <defaults>
    <length unit="meter"/>
    <angle unit="degree"/>
    <reference-frame>
      <along-beam axis="z"/>
      <pointing-up axis="y"/>
      <handedness val="right"/>
    </reference-frame>
    <default-view view="3D" axis-view="z"/>
  </defaults>
  <component type="moderator"><location z="-10"/></component>
  <type name="moderator" is="Source"/>
  <component type="sample-position"><location/></component>
  <type name="sample-position" is="SamplePos"/>
  <component type="monitor" idlist="monitors"><location z="-1" name="monitor1"/></component>
  <type name="monitor" is="monitor">
    <sphere id="ball"><centre x="0" y="0" z="0"/><radius val="0.01"/></sphere>
    <algebra val="ball"/>
  </type>
  <idlist idname="monitors"><id val="1"/></idlist>
  <component type="grid-bank" idstart="100" idfillorder="zyx" idstepbyrow="6" idstep="1">
    <location x="1" z="2" rot="30" axis-x="0" axis-y="1" axis-z="0"/>
    <side-by-side-view-location x="0.5" y="-0.25"/>
  </component>
  <type name="grid-bank" is="grid_detector" type="pixel" xpixels="2" xstart="-0.1" xstep="0.2" ypixels="3"
        ystart="-0.2" ystep="0.2" zpixels="2" zstart="0" zstep="0.1"/>
  <component type="structured-bank" idstart="200" idfillbyfirst="y" idstepbyrow="2" idstep="1">
    <location x="-1" z="2"/>
  </component>
  <type name="structured-bank" is="StructuredDetector" xpixels="2" ypixels="2" type="pixel">
    <vertex x="0" y="0"/><vertex x="1" y="0"/><vertex x="2" y="0"/>
    <vertex x="0" y="1"/><vertex x="1" y="1"/><vertex x="2" y="1"/>
    <vertex x="0" y="2"/><vertex x="1" y="2"/><vertex x="2" y="2"/>
  </type>
  <type name="pixel" is="detector">
    <cuboid id="box">
      <left-front-bottom-point x="-0.05" y="-0.05" z="0"/>
      <left-front-top-point x="-0.05" y="0.05" z="0"/>
      <left-back-bottom-point x="-0.05" y="-0.05" z="0.01"/>
      <right-front-bottom-point x="0.05" y="-0.05" z="0"/>
    </cuboid>
    <algebra val="box"/>
  </type>
  <component-link name="grid-bank">
    <parameter name="tube-pressure"><value val="10.0"/></parameter>
  </component-link>
</instrument>
)";

struct SnapshotCase {
  std::string name;
  /// A file under the instrument directory, or empty to use 'definition'.
  std::string file;
  std::string definition;
  /// "section digest" lines recorded from the parser before the BeamlineBuilder port.
  std::vector<std::string> expected;
};

std::vector<SnapshotCase> snapshotCases() {
  return {
      {"banks",
       "",
       bankDefinition,
       {
           "metadata 47577570e1f7cd14ab3f6e65a646d03ff27dd354",
           "tree 044ebe189009a9923df704fe3a25fe0a3d90b70b",
           "geometry 2447a435d216cecca20b7a99ed9d57f940935a66",
           "shapes e3fa628fb34bfc641ee85621641d28728a0ccac2",
           "banks 972bf6a15817f27ac9c7b0a4ad53397c813dfa58",
           "detectors ca7bd255311e0d9297c57c9cadd79c17f3a8500c",
           "parameters dc2a2db073ebeef6ba93406cf07bc5c143820a6b",
           "logfile parameters ff7d4f239cfb9ffa9ab1fa96bd3926351cfa0ca9",
           "spectra c306bd461088778551d3bc6310562d16632410ee",
       }},
      {"locations",
       "unit_testing/IDF_for_UNIT_TESTING.xml",
       "",
       {
           "metadata aa834a373d58af437b2b55b00c9bb8fef40e4ad2",
           "tree 2aea6fa376b6ae4005d331c30f7e71947a2a985a",
           "geometry 73eb3df767ba5a073ce0f7ccdaf45adad0b40233",
           "shapes 260ad512180190b72d8484175f7c826a671fd3a1",
           "banks da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "detectors 9b160fa9f267f94979825914d6ca36fd851d937d",
           "parameters da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "logfile parameters da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "spectra 38178a40130afbb589a94d1e37fa6ad9edafc72e",
       }},
      {"logfile and rotations",
       "unit_testing/IDF_for_UNIT_TESTING2.xml",
       "",
       {
           "metadata dfbb2318e55fcfe46711ba8188010ee7f8e1074c",
           "tree e8d6e4d2ed7a0cc69c9f74f004cd1deed37f1432",
           "geometry 0273ad0a0cb58ea39a147964e9a92284d7d5d064",
           "shapes 61501c38bb373d65d224f19787a1496f911d5a6a",
           "banks da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "detectors 1d3dca5e61358658502b26b889a73c23ad9c3673",
           "parameters 925f6a374da08dc9593b2254a47fb61295275a81",
           "logfile parameters c8e3662a258373d1dbffb5a9680a05ef840ca9af",
           "spectra 9a109997432b8044975c76c89af6b97f939e679e",
       }},
      {"combined shapes",
       "unit_testing/IDF_for_UNIT_TESTING5.xml",
       "",
       {
           "metadata 9e5ac558a87903ddbf541fe0d2674f0f858318b3",
           "tree 92a7f859ff9cae79d5320bcddd57d341bd25b68d",
           "geometry 17bef4e579317f4a3c47ff8a905717b4745d3b85",
           "shapes e41da71a9b8f0692f185ed4c9b6042c3777e7ada",
           "banks da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "detectors 316c56b95ae8468e2a3d3e40a6eafa778c42fffa",
           "parameters da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "logfile parameters da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "spectra a3124e37ed3bf773d54997f0edb30e37dd6a1d35",
       }},
      {"facing",
       "unit_testing/IDF_for_UNIT_TESTING6.xml",
       "",
       {
           "metadata 2793fd01afcae5faa53be0bdb73800f7608bb18e",
           "tree 7e4ac427baa6aa374c8f504a87121938ffd91da6",
           "geometry 73eb3df767ba5a073ce0f7ccdaf45adad0b40233",
           "shapes 260ad512180190b72d8484175f7c826a671fd3a1",
           "banks da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "detectors 9b160fa9f267f94979825914d6ca36fd851d937d",
           "parameters da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "logfile parameters da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "spectra 38178a40130afbb589a94d1e37fa6ad9edafc72e",
       }},
      {"rectangular banks",
       "unit_testing/IDF_for_RECTANGULAR_UNIT_TESTING.xml",
       "",
       {
           "metadata 01dd278a2b065a979ba0ae99b5f73bf30c12efdd",
           "tree b5f2f7b9b2aca50ea53262a91305e3e9db466f69",
           "geometry 599dde1c39826d46de20752cd6e0d43601de0dda",
           "shapes 7b74c78b73274885d46c2cfc116589b8baed9a54",
           "banks 8ed367912d98e74de054d02cd0bb60fe155d963c",
           "detectors 1ace9b583f25c8c2224ce02bb9b6a0f22196b612",
           "parameters da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "logfile parameters da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "spectra 599b718d5e25ec066b71fb88480ad72d7de92599",
       }},
      {"neutronic positions",
       "unit_testing/INDIRECT_Definition.xml",
       "",
       {
           "metadata 1af35fdfc96bbafc37f284850887612ba2a4bad7",
           "tree 1286e5b9c2ae2339e0ccf3235bc0cec400025d6e",
           "geometry e90e0acf0763584c61b94812e79f4e484c8e3b37",
           "shapes fc975063ed2d418d616a14ea37a8a032131ec056",
           "banks da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "detectors 116e5d3d275811c053b0c08c61102e3ec8dd20a7",
           "parameters da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "logfile parameters da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "physical tree 9e72ae754438f41ef1cf278cdfefc1e3869b9d6f",
           "physical geometry 05b98b547b862a8cefdabd5e01b729c6f920b005",
           "physical shapes fcdd18bfab64897e14d1605706fd93d3e37edd08",
           "physical banks da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "physical detectors 1799e4ed49e9a7633b4a4e8968c2cd66eb4a755c",
           "spectra 8002abd24fc2850da15c6b7d94808abb19031122",
       }},
      {"outlines",
       "unit_testing/MAPS_Definition_Reduced.xml",
       "",
       {
           "metadata 3277385ae8373810a939f505577376200f787a63",
           "tree 290ce8677243310f160a9676cf1b5fc7facc1823",
           "geometry 34d97f16b5aee1abd01bae187a17a25a30ce685b",
           "shapes f02f6079d8ec01c0e72e5c16eaa08b3db86c0173",
           "banks da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "detectors b5f1e237472acd50d8364558082af0fc61bc239f",
           "parameters 151eb3075c7850024d9ec630fca935077767688b",
           "logfile parameters 4e90c90968db3e9ba018b34d7eb0cdf82cb2035e",
           "spectra 0e75c001477a5d296adf1fd4fee60e2101619bc9",
       }},
      {"spherical offsets",
       "unit_testing/HRPDTEST_Definition.xml",
       "",
       {
           "metadata 848557c4cb5781f0a457d8a985614eff5d150d35",
           "tree c7b70fe9f3b1d6aa38bfc8462732fce60b5d4ea3",
           "geometry 1980795b4fefd82ad359ecf57241bbad80e92784",
           "shapes af76c731da3910181205b2cb8fb5c6789b457c61",
           "banks da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "detectors 7544a3450ed28253f4d3ad8347c03b16c86a4282",
           "parameters 81e033606790b1a9c3a0e5a253f0824c4ba28cae",
           "logfile parameters eadd4a5e798518331362aaaf020b828f3d5bf6ba",
           "spectra f12313e23dbc8b45506d6ff9894eed4c238b1d82",
       }},
      {"monitors and id lists",
       "unit_testing/DUM_Definition.xml",
       "",
       {
           "metadata 3066cb0b5f59e580c2bf5790707785918ff02217",
           "tree 453af600a24ea76d4a0210b520c021383b168624",
           "geometry 1ca74f6d4ea4c62aaf5049d15be8d65e4a8ce74c",
           "shapes 69323ab1c85e5fb0bca7e12d130a6a9050de32d1",
           "banks da39a3ee5e6b4b0d3255bfef95601890afd80709",
           "detectors 97082e2fc5ead293b30487e5ee263b4e6050e43b",
           "parameters 283b5a2b6e188366e12b8dd7149f4f7725cb612e",
           "logfile parameters 5f0840f8b4822b3ca948cfb91425082359315246",
           "spectra 50ef32cc9f34e1d30068934a4fe140e9cab95762",
       }},
  };
}

std::string environment(char const *const name) {
  char const *const value = std::getenv(name);
  return value ? value : "";
}

} // namespace

/** Pins down what the instrument definition parser builds, while it is ported from building the legacy component
 * tree to BeamlineBuilder. The fast test compares section digests for a curated set of definitions against digests
 * recorded before the port. test_every_definition_file is a local harness: with MANTID_IDF_SNAPSHOT_DIR set, it
 * writes digests for every definition file in the instrument directory (and full dumps for small ones), to be diffed
 * between builds; otherwise it does nothing. The largest definitions need several GB each, so run it under a memory
 * cap, e.g. systemd-run --user --scope -p MemoryMax=24G -p MemorySwapMax=0 bin/DataHandlingTest
 * InstrumentDefinitionSnapshotTest test_every_definition_file
 */
class InstrumentDefinitionSnapshotTest : public CxxTest::TestSuite {
public:
  static InstrumentDefinitionSnapshotTest *createSuite() { return new InstrumentDefinitionSnapshotTest(); }
  static void destroySuite(InstrumentDefinitionSnapshotTest *suite) { delete suite; }

  void test_curated_definitions_match_the_recorded_snapshots() {
    auto const directory = Mantid::Kernel::ConfigService::Instance().getInstrumentDirectory();
    auto const outputDirectory = environment("MANTID_IDF_SNAPSHOT_DIR");
    for (auto const &snapshotCase : snapshotCases()) {
      std::optional<ScopedFileHelper::ScopedFile> file;
      std::string filename = directory + snapshotCase.file;
      if (snapshotCase.file.empty()) {
        filename = file.emplace(snapshotCase.definition, "SNAPSHOTBANKS_Definition.xml").getFileName();
      }
      std::ofstream full;
      if (!outputDirectory.empty()) {
        std::filesystem::create_directories(std::filesystem::path(outputDirectory) / "curated");
        full.open(std::filesystem::path(outputDirectory) / "curated" / (snapshotCase.name + ".txt"));
      }
      std::vector<std::string> digests;
      TS_ASSERT_THROWS_NOTHING(digests =
                                   snapshotWorkspace(*loadEmptyInstrument(filename), full.is_open() ? &full : nullptr));
      if (snapshotCase.expected.empty()) {
        std::string recorded;
        for (auto const &digest : digests) {
          recorded += "\"" + digest + "\", ";
        }
        TS_FAIL("No snapshot recorded for '" + snapshotCase.name + "'; it is {" + recorded + "}");
        continue;
      }
      TS_ASSERT_EQUALS(digests.size(), snapshotCase.expected.size());
      for (size_t i = 0; i < std::min(digests.size(), snapshotCase.expected.size()); ++i) {
        TSM_ASSERT_EQUALS("'" + snapshotCase.name +
                              "' changed; set MANTID_IDF_SNAPSHOT_DIR to write full dumps, and diff them with a build "
                              "from before the change",
                          digests[i], snapshotCase.expected[i]);
      }
    }
  }

  void test_every_definition_file() {
    auto const outputDirectory = environment("MANTID_IDF_SNAPSHOT_DIR");
    if (outputDirectory.empty()) {
      return;
    }
    auto const filter = environment("MANTID_IDF_SNAPSHOT_FILTER");
    auto const maxText = environment("MANTID_IDF_SNAPSHOT_MAX_COMPONENTS");
    size_t const maxComponents = maxText.empty() ? 200000 : std::stoul(maxText);
    std::filesystem::create_directories(outputDirectory);

    std::vector<std::filesystem::path> files;
    for (auto const &entry :
         std::filesystem::directory_iterator(Mantid::Kernel::ConfigService::Instance().getInstrumentDirectory())) {
      auto const name = entry.path().filename().string();
      if (name.ends_with(".xml") && name.find("_Definition") != std::string::npos &&
          name.find(filter) != std::string::npos) {
        files.emplace_back(entry.path());
      }
    }
    std::ranges::sort(files);

    std::ofstream digestFile(std::filesystem::path(outputDirectory) / "digests.txt");
    for (auto const &file : files) {
      auto const name = file.filename().string();
      try {
        auto const workspace = loadEmptyInstrument(file.string());
        std::ofstream full;
        if (workspace->componentInfo().size() <= maxComponents) {
          full.open(std::filesystem::path(outputDirectory) / (name + ".txt"));
        }
        for (auto const &digest : snapshotWorkspace(*workspace, full.is_open() ? &full : nullptr)) {
          digestFile << name << ' ' << digest << '\n';
        }
      } catch (std::exception const &error) {
        digestFile << name << " failed " << error.what() << '\n';
      }
      digestFile.flush();
      // LoadEmptyInstrument keeps every instrument it parses; without this, all of them pile up in memory.
      Mantid::API::InstrumentDataService::Instance().clear();
      Mantid::API::AnalysisDataService::Instance().clear();
    }
  }
};
