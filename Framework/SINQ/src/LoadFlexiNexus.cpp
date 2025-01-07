// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSINQ/LoadFlexiNexus.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Utils.h"
#include "MantidNexusCpp/NeXusException.hpp"

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <sstream>

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadFlexiNexus)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace ::NeXus;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Points;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void LoadFlexiNexus::init() {

  declareProperty(
      std::make_unique<FileProperty>("Filename", "", FileProperty::Load, std::vector<std::string>{".hdf", ".h5", ""}),
      "A NeXus file");
  declareProperty(std::make_unique<FileProperty>("Dictionary", "", FileProperty::Load,
                                                 std::vector<std::string>{".txt", ".dic", ""}),
                  "A Dictionary for controlling NeXus loading");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output));
}

void LoadFlexiNexus::exec() {

  std::string filename = getProperty("Filename");
  std::string dictname = getProperty("Dictionary");
  g_log.information() << "Running FlexiNexus for " << filename << " with  " << dictname << '\n';

  loadDictionary(getProperty("Dictionary"));

  /* print the dictionary for debugging..........
  for(std::map<std::string, std::string>::const_iterator it =
  dictionary.begin();
  it!= dictionary.end(); it++){
  std::cout << it->first << "\t" << it->second << '\n';
  }
  */

  File fin(filename);
  readData(&fin);
}

void LoadFlexiNexus::loadDictionary(const std::string &dictFile) {
  std::ifstream in(dictFile.c_str(), std::ifstream::in);
  std::string line, key, value;

  while (std::getline(in, line)) {
    // skip comments
    if (line[0] == '#') {
      continue;
    }
    // skip empty lines
    if (line.length() < 2) {
      continue;
    }
    std::istringstream l(line);
    std::getline(l, key, '=');
    std::getline(l, value, '\n');
    boost::algorithm::trim(key);
    boost::algorithm::trim(value);
    dictionary[key] = value;
  }
  in.close();
}

void LoadFlexiNexus::readData(NeXus::File *fin) {
  std::map<std::string, std::string>::const_iterator it;

  if ((it = dictionary.find("data")) == dictionary.end()) {
    throw std::runtime_error("Required dictionary element data not found");
  }

  // inspect the data element and create WS matching dims
  if (!safeOpenpath(fin, it->second)) {
    throw std::runtime_error("data NeXus path not found!");
  }

  Info inf = fin->getInfo();
  size_t rank = inf.dims.size();

  if (rank <= 2) {
    load2DWorkspace(fin);
  } else {
    loadMD(fin);
  }
}

void LoadFlexiNexus::load2DWorkspace(NeXus::File *fin) {
  Mantid::DataObjects::Workspace2D_sptr ws;
  int nSpectra, spectraLength;
  std::vector<int> data;

  // read the data first
  fin->getDataCoerce(data);

  Info inf = fin->getInfo();
  if (inf.dims.size() == 1) {
    nSpectra = 1;
    spectraLength = static_cast<int>(inf.dims[0]);
  } else {
    nSpectra = static_cast<int>(inf.dims[0]);
    spectraLength = static_cast<int>(inf.dims[1]);
  }

  g_log.debug() << "Reading " << nSpectra << " spectra of length " << spectraLength << ".\n";

  // need to locate x-axis data too.....
  std::map<std::string, std::string>::const_iterator it;
  std::vector<double> xData;
  if ((it = dictionary.find("x-axis")) == dictionary.end()) {
    xData.resize(spectraLength);
    for (int i = 0; i < spectraLength; i++) {
      xData[i] = static_cast<double>(i);
    }
  } else {
    if (safeOpenpath(fin, it->second)) {
      fin->getDataCoerce(xData);
    }
  }

  // need to locate y-axis data too.....
  std::vector<double> yData;
  if ((it = dictionary.find("y-axis")) == dictionary.end()) {
    yData.resize(nSpectra);
    for (int i = 0; i < nSpectra; i++) {
      yData[i] = static_cast<double>(i);
    }
  } else {
    if (safeOpenpath(fin, it->second)) {
      fin->getDataCoerce(yData);
    }
  }

  // fill the data.......
  ws = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", nSpectra, xData.size(), spectraLength));

  // x can be bin edges or points, depending on branching above
  auto x = Kernel::make_cow<HistogramData::HistogramX>(xData);
  for (int wsIndex = 0; wsIndex < nSpectra; wsIndex++) {
    auto beg = data.begin() + spectraLength * wsIndex;
    auto end = beg + spectraLength;
    if (static_cast<size_t>(spectraLength) == xData.size())
      ws->setHistogram(wsIndex, Points(x), Counts(beg, end));
    else
      ws->setHistogram(wsIndex, BinEdges(x), Counts(beg, end));

    ws->getSpectrum(wsIndex).setSpectrumNo(static_cast<specnum_t>(yData[wsIndex]));
    ws->getSpectrum(wsIndex).setDetectorID(static_cast<detid_t>(yData[wsIndex]));
  }

  ws->setYUnit("Counts");

  // assign an x-axis-name
  if ((it = dictionary.find("x-axis-name")) == dictionary.end()) {
    const std::string xname("no axis name found");
    ws->getAxis(0)->title() = xname;
  } else {
    const std::string xname(it->second);
    ws->getAxis(0)->title() = xname;
    if (xname == "TOF") {
      g_log.debug() << "Setting X-unit to be TOF\n";
      ws->getAxis(0)->setUnit("TOF");
    }
  }

  addMetaData(fin, ws, (ExperimentInfo_sptr)ws);

  // assign the workspace
  setProperty("OutputWorkspace", std::dynamic_pointer_cast<Workspace>(ws));
}

void LoadFlexiNexus::loadMD(NeXus::File *fin) {
  std::vector<double> data;

  // read the data first
  fin->getDataCoerce(data);

  Info inf = fin->getInfo();

  std::vector<MDHistoDimension_sptr> dimensions;
  for (int k = static_cast<int>(inf.dims.size()) - 1; k >= 0; k--) {
    dimensions.emplace_back(makeDimension(fin, k, static_cast<int>(inf.dims[k])));
  }

  auto ws = std::make_shared<MDHistoWorkspace>(dimensions);

  signal_t *dd = ws->mutableSignalArray();
  signal_t *ddE = ws->mutableErrorSquaredArray();

  // assign data
  for (size_t i = 0; i < data.size(); i++) {
    dd[i] = data[i];
    ddE[i] = dblSqrt(data[i]);
  }

  if (ws->getNumExperimentInfo() == 0) {
    ws->addExperimentInfo((ExperimentInfo_sptr) new ExperimentInfo());
  }
  addMetaData(fin, ws, ws->getExperimentInfo(0));

  // assign the workspace
  setProperty("OutputWorkspace", ws);
}
int LoadFlexiNexus::calculateCAddress(const int *pos, const int *dim, int rank) {
  int result = pos[rank - 1];
  for (int i = 0; i < rank - 1; i++) {
    int mult = 1;
    for (int j = rank - 1; j > i; j--) {
      mult *= dim[j];
    }
    if (pos[i] < dim[i] && pos[i] > 0) {
      result += mult * pos[i];
    }
  }
  return result;
}
int LoadFlexiNexus::calculateF77Address(int * /*unused*/, int /*unused*/) {
  // --- Unused code ---
  // int result = 0;

  // for(int i = 0; i < rank; i++){
  // 	result += pos[i]*indexMaker[i];
  // }
  // return result;
  return 0;
}

double LoadFlexiNexus::dblSqrt(double in) { return sqrt(in); }

MDHistoDimension_sptr LoadFlexiNexus::makeDimension(NeXus::File *fin, int index, int length) {
  static const char *axisNames[] = {"x", "y", "z"};
  std::map<std::string, std::string>::const_iterator it;

  // get a name
  std::string axisName(axisNames[index]);
  axisName += "axis-name";
  if ((it = dictionary.find(axisName)) != dictionary.end()) {
    axisName = it->second;
  } else {
    axisName = axisNames[index];
  }

  // get axis data
  std::string dataName(axisNames[index]);
  dataName += "-axis";
  std::vector<double> dData;
  if ((it = dictionary.find(dataName)) == dictionary.end()) {
    dData.resize(length);
    for (int i = 0; i < length; i++) {
      dData[i] = static_cast<double>(i);
    }
  } else {
    if (safeOpenpath(fin, it->second)) {
      fin->getDataCoerce(dData);
    } else {
      dData.resize(length);
      for (int i = 0; i < length; i++) {
        dData[i] = static_cast<double>(i);
      }
    }
  }
  auto min = static_cast<coord_t>(dData[0]);
  auto max = static_cast<coord_t>(dData[length - 1]);
  if (min > max) {
    const auto tmp = max;
    max = min;
    min = tmp;
    g_log.notice("WARNING: swapped axis values on " + axisName);
  }
  Mantid::Geometry::GeneralFrame frame(axisName, "");
  return MDHistoDimension_sptr(new MDHistoDimension(axisName, axisName, frame, min, max, length));
}
void LoadFlexiNexus::addMetaData(NeXus::File *fin, const Workspace_sptr &ws, const ExperimentInfo_sptr &info) {
  std::map<std::string, std::string>::const_iterator it;

  // assign a title
  if ((it = dictionary.find("title")) == dictionary.end()) {
    const std::string title("No title found");
    ws->setTitle(title);
  } else {
    if (it->second.find('/') == it->second.npos) {
      const std::string title(it->second);
      ws->setTitle(title);
    } else {
      if (safeOpenpath(fin, it->second)) {
        const std::string title = fin->getStrData();
        ws->setTitle(title);
      }
    }
  }

  // assign a sample name
  std::string sample;
  if ((it = dictionary.find("sample")) == dictionary.end()) {
    sample = "No sample found";
  } else {
    if (it->second.find('/') == it->second.npos) {
      sample = it->second;
    } else {
      if (safeOpenpath(fin, it->second)) {
        Info inf = fin->getInfo();
        if (inf.dims.size() == 1) {
          sample = fin->getStrData();
        } else { // something special for 2-d array
          std::vector<char> val_array;
          fin->getData(val_array);
          fin->closeData();
          sample = std::string(val_array.begin(), val_array.end());
        }
      } else {
        sample = "Sample path not found";
      }
    }
  }
  info->mutableSample().setName(sample);

  /**
   * load all the extras into the Run information
   */
  Run &r = info->mutableRun();
  auto specialMap = populateSpecialMap();
  for (it = dictionary.begin(); it != dictionary.end(); ++it) {
    if (specialMap.find(it->first) == specialMap.end()) {
      // not in specials!
      if (it->second.find('/') == it->second.npos) {
        r.addProperty(it->first, it->second, true);
      } else {
        if (safeOpenpath(fin, it->second)) {
          NeXus::Info inf = fin->getInfo();
          if (inf.type == ::NeXus::CHAR) {
            std::string data = fin->getStrData();
            r.addProperty(it->first, data, true);
          } else if (inf.type == ::NeXus::FLOAT32 || inf.type == ::NeXus::FLOAT64) {
            std::vector<double> data;
            fin->getDataCoerce(data);
            r.addProperty(it->first, data, true);
          } else {
            std::vector<int> data;
            fin->getDataCoerce(data);
            r.addProperty(it->first, data, true);
          }
        }
      }
    }
  }
}
std::unordered_set<std::string> LoadFlexiNexus::populateSpecialMap() {
  std::unordered_set<std::string> specialMap;

  specialMap.insert("title");
  specialMap.insert("data");
  specialMap.insert("sample");
  specialMap.insert("x-axis");
  specialMap.insert("x-axis-name");
  specialMap.insert("y-axis");
  specialMap.insert("y-axis-name");
  specialMap.insert("z-axis");
  specialMap.insert("z-axis-name");

  return specialMap;
}

int LoadFlexiNexus::safeOpenpath(NeXus::File *fin, const std::string &path) {
  try {
    fin->openPath(path);
  } catch (NeXus::Exception &) {
    getLogger().error("NeXus path " + path + " kaputt");
    return 0;
  }
  return 1;
}
