// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/Converters/CloneToNDArray.h"
#include "MantidPythonInterface/core/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/core/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/tuple.hpp>

using Mantid::SpectrumDefinition;
using Mantid::API::MatrixWorkspace;
using Mantid::DataObjects::Workspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace Mantid::Indexing;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Workspace2D)

class Workspace2DPickleSuite : public boost::python::pickle_suite {
public:
  GNU_DIAG_OFF("dangling-reference")
  static dict getstate(const Workspace2D &ws) {
    using namespace Mantid::PythonInterface::Converters;

    // Python lists that will be added to dict
    list spectraList;
    list errorList;
    list binEdgeList;
    list detectorList;
    list specNumList;

    auto spectra = ws.getNumberHistograms();
    const auto &indexInfo = ws.indexInfo();
    const auto &spectrumDefinitions = indexInfo.spectrumDefinitions();
    const auto &componentInfo = ws.detectorInfo();
    if (componentInfo.isScanning()) {
      throw std::invalid_argument("Cannot pickle Scanning Workspace2D");
    }

    for (decltype(spectra) i = 0; i < spectra; ++i) {
      const auto &histo = ws.histogram(i);

      const auto &spectraData = histo.counts().data().rawData();
      const auto &errorData = histo.countStandardDeviations().rawData();
      const auto &binEdges = histo.binEdges().rawData();

      spectraList.append(object(handle<>(VectorToNDArray<double, WrapReadOnly>()(spectraData))));
      errorList.append(object(handle<>(VectorToNDArray<double, WrapReadOnly>()(errorData))));
      binEdgeList.append(object(handle<>(VectorToNDArray<double, WrapReadOnly>()(binEdges))));

      auto spectrumNumber = indexInfo.spectrumNumber(i);
      const auto &spectrumDefinition = (*spectrumDefinitions)[i];
      std::vector<size_t> detectorIndices;

      for (const auto &j : spectrumDefinition) {
        size_t detectorIndex = j.first;
        detectorIndices.emplace_back(detectorIndex);
      }

      detectorList.append(object(handle<>(VectorToNDArray<size_t, Clone>()(detectorIndices))));
      specNumList.append(static_cast<int32_t>(spectrumNumber));
    }

    dict state;
    state["title"] = ws.getTitle();
    state["instrument_name"] = ws.getInstrument()->getName();
    state["instrument_xml"] = ws.getInstrument()->getXmlText();
    state["unit_x"] = ws.getAxis(0)->unit()->unitID();
    state["unit_y"] = ws.getAxis(1)->unit()->unitID();
    state["spectra"] = spectraList;
    state["error"] = errorList;
    state["bin_edges"] = binEdgeList;
    state["detectors"] = detectorList;
    state["spectrum_numbers"] = specNumList;
    return state;
  }

  GNU_DIAG_ON("dangling-reference")

  static void setstate(Workspace2D &ws, dict state) {
    using namespace Mantid::PythonInterface::Converters;

    std::vector<SpectrumNumber> spectrumNumbers;
    std::vector<SpectrumDefinition> spectrumDefinitions;

    list spectraList = extract<list>(state["spectra"]);
    list errorList = extract<list>(state["error"]);
    list binEdgeList = extract<list>(state["bin_edges"]);
    list detectorList = extract<list>(state["detectors"]);
    list specNumList = extract<list>(state["spectrum_numbers"]);

    ws.initialize(len(spectraList), len(binEdgeList[0]), len(spectraList[0]));
    ws.setTitle(extract<std::string>(state["title"]));

    std::string unitX = extract<std::string>(state["unit_x"]);
    ws.getAxis(0)->setUnit(unitX);
    std::string unitY = extract<std::string>(state["unit_y"]);
    ws.getAxis(1)->setUnit(unitY);

    for (size_t i = 0; i < static_cast<size_t>(len(spectraList)); ++i) {
      std::vector<double> spectraData = NDArrayToVector<double>(boost::python::object(spectraList[i]))();
      std::vector<double> errorData = NDArrayToVector<double>(boost::python::object(errorList[i]))();
      std::vector<double> binEdgeData = NDArrayToVector<double>(boost::python::object(binEdgeList[i]))();
      std::vector<size_t> detectorIndices = NDArrayToVector<size_t>(boost::python::object(detectorList[i]))();
      SpectrumNumber specNum = static_cast<SpectrumNumber>(extract<int32_t>(specNumList[i]));

      ws.setCounts(i, std::move(spectraData));
      ws.setCountStandardDeviations(i, std::move(errorData));
      ws.setBinEdges(i, std::move(binEdgeData));

      SpectrumDefinition specDef;
      for (auto detectorIndex : detectorIndices) {
        specDef.add(detectorIndex);
      }
      spectrumDefinitions.emplace_back(std::move(specDef));
      spectrumNumbers.emplace_back(specNum);
    }

    std::string instrumentXML = extract<std::string>(state["instrument_xml"]);
    std::string instrumentName = extract<std::string>(state["instrument_name"]);
    if (!instrumentName.empty() && !instrumentXML.empty()) {
      try {
        auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
        // Do not put the workspace in the ADS
        alg->setChild(true);
        alg->initialize();
        alg->setPropertyValue("InstrumentName", instrumentName);
        alg->setPropertyValue("InstrumentXML", instrumentXML);
        alg->setProperty("Workspace", std::shared_ptr<Workspace2D>(&ws, [](Workspace2D *) {}));
        alg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
        alg->execute();
      } catch (std::exception &exc) {
        Mantid::Kernel::Logger("Workspace2DPickleSuite").warning()
            << "Could not load instrument XML: " << exc.what() << "\n";
      }
    }

    Mantid::Indexing::IndexInfo indexInfo(spectrumNumbers);
    indexInfo.setSpectrumDefinitions(spectrumDefinitions);
    ws.setIndexInfo(indexInfo);
  }
};

std::shared_ptr<Mantid::API::Workspace> makeWorkspace2D() { return std::make_shared<Workspace2D>(); }

void export_Workspace2D() {
  class_<Workspace2D, bases<MatrixWorkspace>, boost::noncopyable>("Workspace2D")
      .def_pickle(Workspace2DPickleSuite())
      .def("__init__", boost::python::make_constructor(&makeWorkspace2D));

  // register pointers
  RegisterWorkspacePtrToPython<Workspace2D>();
}
