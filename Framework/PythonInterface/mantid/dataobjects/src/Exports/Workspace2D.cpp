#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Unit.h"
#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include "MantidPythonInterface/kernel/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::API::MatrixWorkspace;
using Mantid::DataObjects::Workspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Workspace2D);


class Workspace2DPickleSuite : public boost::python::pickle_suite {
public:
  static dict getstate(const Workspace2D& ws) {
    using namespace Mantid::PythonInterface::Converters;

    auto spectra = ws.getNumberHistograms();

    dict state;

    state["instrument_name"] = ws.getInstrument()->getName();
    state["instrument_xml"] = ws.getInstrument()->getXmlText();
    state["unit_x"] = ws.getAxis(0)->unit()->unitID();

    list spectraList;
    list errorList;
    list binEdgeList;

    for (decltype(spectra) i = 0; i < spectra; ++i) {
      const auto &histo = ws.histogram(i);

      const auto &spectraData = histo.counts().data().rawData();
      const auto &errorData = histo.countStandardDeviations().rawData();
      const auto &binEdges = histo.binEdges().rawData();

      spectraList.append(object(handle<>(VectorToNDArray<double, WrapReadOnly>()(spectraData))));
      errorList.append(object(handle<>(VectorToNDArray<double, WrapReadOnly>()(errorData))));
      binEdgeList.append(object(handle<>(VectorToNDArray<double, WrapReadOnly>()(binEdges))));
    }

    state["spectra"] = spectraList;
    state["error"] = errorList;
    state["bin_edges"] = binEdgeList;

    return state;
  }

  static void setstate(Workspace2D& ws, dict state)
  {
    using namespace Mantid::PythonInterface::Converters;

    list spectraList = extract<list>(state["spectra"]);
    list errorList = extract<list>(state["error"]);
    list binEdgeList = extract<list>(state["bin_edges"]);

    ws.initialize(len(spectraList), len(binEdgeList[0]), len(spectraList[0]));

    for(size_t i = 0; i < static_cast<size_t>(len(spectraList)); ++i){
      std::vector<double> spectraData = NDArrayToVector<double>(spectraList[i])();
      std::vector<double> errorData = NDArrayToVector<double>(errorList[i])();
      std::vector<double> binEdgeData = NDArrayToVector<double>(binEdgeList[i])();

      ws.setCounts(i, std::move(spectraData));
      ws.setCountStandardDeviations(i, std::move(errorData));
      ws.setBinEdges(i, std::move(binEdgeData));
    }

    std::string unitX = extract<std::string>(state["unit_x"]);
    ws.getAxis(0)->setUnit(unitX);

    std::string instrumentXML = extract<std::string>(state["instrument_xml"]);
    std::string instrumentName = extract<std::string>(state["instrument_name"]);
    try {
      auto alg =
          Mantid::API::AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
      // Do not put the workspace in the ADS
      alg->setChild(true);
      alg->initialize();
      alg->setPropertyValue("InstrumentName", instrumentName);
      alg->setPropertyValue("InstrumentXML", instrumentXML);
      alg->setProperty("Workspace", boost::shared_ptr<Workspace2D>(&ws, [](Workspace2D*){}));
      alg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
      alg->execute();
    } catch (std::exception &exc) {
      Mantid::Kernel::Logger("Workspace2DPickleSuite").warning()
          << "Could not load instrument XML: " << exc.what() << "\n";
    }
  }
};

boost::shared_ptr<Mantid::API::Workspace> makeWorkspace2D() {
  return boost::make_shared<Workspace2D>();
}


void export_Workspace2D() {
  class_<Workspace2D, bases<MatrixWorkspace>, boost::noncopyable>(
        "Workspace2D")
        .def_pickle(Workspace2DPickleSuite())
      .def("__init__", boost::python::make_constructor(&makeWorkspace2D));

  // register pointers
  RegisterWorkspacePtrToPython<Workspace2D>();
}
