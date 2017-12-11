#include "MantidDataObjects/Workspace2D.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include "MantidKernel/Logger.h"

#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <MantidPythonInterface/kernel/Converters/WrapWithNumpy.h>
#include <MantidPythonInterface/kernel/Converters/VectorToNDArray.h>
#include <MantidPythonInterface/kernel/Converters/NDArrayToVector.h>

using Mantid::API::MatrixWorkspace;
using Mantid::DataObjects::Workspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Workspace2D)


#include <iostream>
#include <MantidPythonInterface/kernel/Converters/VectorToNDArray.h>


class Workspace2DPickleSuite : public boost::python::pickle_suite {
public:
  static boost::python::dict getstate(const Workspace2D& ws) {
    using namespace Mantid::PythonInterface::Converters;
    auto spectra = ws.getNumberHistograms();

    boost::python::dict state;
    state["version"] = "0.1-alpha";

    boost::python::list spectraList;
    boost::python::list errorList;
    boost::python::list binEdgeList;

    for (decltype(spectra) i = 0; i < spectra; ++i) {
      const auto &histo = ws.histogram(i);
      const auto &spectraData = histo.counts().data().rawData();
      spectraList.append(object(handle<>(VectorToNDArray<double, WrapReadOnly>()(spectraData))));
      const auto &errorData = histo.countStandardDeviations().rawData();
      errorList.append(object(handle<>(VectorToNDArray<double, WrapReadOnly>()(errorData))));
      const auto &binEdges = histo.binEdges().rawData();
      binEdgeList.append(object(handle<>(VectorToNDArray<double, WrapReadOnly>()(binEdges))));
    }

    state["spectra"] = spectraList;
    state["error"] = errorList;
    state["bin_edges"] = binEdgeList;
    return state;
  }

  static
  void
  setstate(Workspace2D& ws, boost::python::dict state)
  {
      using namespace Mantid::PythonInterface::Converters;
      using namespace boost::python;


      std::string number = extract<std::string>(state["version"]);
      list spectraList = extract<list>(state["spectra"]);
      list errorList = extract<list>(state["error"]);
      list binEdgeList = extract<list>(state["bin_edges"]);

      ws.initialize(len(spectraList), len(binEdgeList[0]), len(spectraList[0]));

      for(size_t i = 0; i < static_cast<size_t>(len(spectraList)); ++i){
        std::vector<double> spectraData = NDArrayToVector<double>(spectraList[i])();
        std::vector<double> errorData = NDArrayToVector<double>(errorList[i])();
        std::vector<double> binEdgeData = NDArrayToVector<double>(binEdgeList[i])();

        ws.setCounts(i, spectraData);
        ws.setCountStandardDeviations(i, errorData);
        ws.setBinEdges(i, binEdgeData);
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
