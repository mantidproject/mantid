// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSassena.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/H5Util.h"

#include <utility>

namespace Mantid::DataHandling {

DECLARE_NEXUS_HDF5_FILELOADER_ALGORITHM(LoadSassena)

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSassena::confidence(Kernel::NexusHDF5Descriptor &descriptor) const {
  if (descriptor.hasRootAttr("sassena_version") || descriptor.isEntry("/qvectors")) {
    return 99;
  }
  return 0;
}

/**
 * Register a workspace in the Analysis Data Service and add it to the
 * groupWorkspace
 * @param gws pointer to WorkspaceGroup being filled
 * @param wsName name of workspace to be added and registered
 * @param ws pointer to workspace to be added and registered
 * @param description
 */
void LoadSassena::registerWorkspace(const API::WorkspaceGroup_sptr &gws, const std::string &wsName,
                                    const DataObjects::Workspace2D_sptr &ws, const std::string &description) {
  UNUSED_ARG(description);
  API::AnalysisDataService::Instance().add(wsName, ws);
  gws->addWorkspace(ws);
}

/**
 * Read dataset dimensionality
 * @param h5file file identifier
 * @param setName string name of dataset
 * @param dims storing dimensionality
 */

void LoadSassena::dataSetInfo(const H5::H5File &h5file, const std::string &setName, hsize_t *dims) const {
  h5file.openDataSet(setName).getSpace().getSimpleExtentDims(dims);
}

/**
 * Read the dataset
 * @param h5file file identifier
 * @param setName string name of dataset
 * @param buf storing dataset
 */
void LoadSassena::dataSetDouble(const H5::H5File &h5file, const std::string &setName, std::vector<double> &buf) {
  Mantid::NeXus::H5Util::readArray1DCoerce(h5file.openDataSet(setName), buf);
}

/* Helper object and function to sort modulus of Q-vectors
 */
using mypair = std::pair<double, int>;
bool compare(const mypair &left, const mypair &right) { return left.first < right.first; }
/**
 * load vectors onto a Workspace2D with 3 bins (the three components of the
 * vectors)
 * dataX for the origin of the vector (assumed (0,0,0) )
 * dataY for the tip of the vector
 * dataE is assumed (0,0,0), no errors
 * @param h5file file identifier
 * @param gws pointer to WorkspaceGroup being filled
 * @param sorting_indexes permutation of qvmod indexes to render it in
 * increasing order of momemtum transfer
 */
HistogramData::Points LoadSassena::loadQvectors(const H5::H5File &h5file, const API::WorkspaceGroup_sptr &gws,
                                                std::vector<int> &sorting_indexes) {

  // store the modulus of the vector
  std::vector<double> qvmod;

  const std::string gwsName = this->getPropertyValue("OutputWorkspace");
  const std::string setName("qvectors");

  hsize_t dims[3];
  try {
    this->dataSetInfo(h5file, setName, dims);
  } catch (H5::Exception &) {
    throw Kernel::Exception::FileError("Unable to read " + setName + " dataset info:", m_filename);
  }

  auto nq = static_cast<int>(dims[0]); // number of q-vectors
  std::vector<double> buf(nq * 3);

  try {
    this->dataSetDouble(h5file, "qvectors", buf);
  } catch (H5::Exception &) {
    this->g_log.error("LoadSassena::loadQvectors cannot proceed");
    qvmod.resize(0);
    return HistogramData::Points(std::move(qvmod));
  }

  qvmod.reserve(nq);
  for (auto curr = buf.cbegin(); curr != buf.cend(); curr += 3) {
    qvmod.emplace_back(sqrt(curr[0] * curr[0] + curr[1] * curr[1] + curr[2] * curr[2]));
  }

  if (getProperty("SortByQVectors")) {
    std::vector<mypair> qvmodpair;
    qvmodpair.reserve(nq);
    for (int iq = 0; iq < nq; iq++)
      qvmodpair.emplace_back(qvmod[iq], iq);
    std::sort(qvmodpair.begin(), qvmodpair.end(), compare);
    for (int iq = 0; iq < nq; iq++)
      sorting_indexes.emplace_back(qvmodpair[iq].second);
    std::sort(qvmod.begin(), qvmod.end());
  } else
    for (int iq = 0; iq < nq; iq++)
      sorting_indexes.emplace_back(iq);

  DataObjects::Workspace2D_sptr ws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", nq, 3, 3));
  std::string wsName = gwsName + std::string("_") + setName;
  ws->setTitle(wsName);

  for (int iq = 0; iq < nq; iq++) {
    auto &Y = ws->mutableY(iq);
    auto curr = std::next(buf.cbegin(), 3 * sorting_indexes[iq]);
    Y.assign(curr, std::next(curr, 3));
  }

  ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("MomentumTransfer"); // Set the Units

  this->registerWorkspace(gws, wsName, ws, "X-axis: origin of Q-vectors; Y-axis: tip of Q-vectors");
  return HistogramData::Points(std::move(qvmod));
}

/**
 * Create workspace to store the structure factor.
 * First spectrum is the real part, second spectrum is the imaginary part
 * X values are the modulus of the Q-vectors
 * @param h5file file identifier
 * @param gws pointer to WorkspaceGroup being filled
 * @param setName string name of dataset
 * @param qvmod vector of Q-vectors' moduli
 * @param sorting_indexes permutation of qvmod indexes to render it in
 * increasing order of momemtum transfer
 */
void LoadSassena::loadFQ(const H5::H5File &h5file, const API::WorkspaceGroup_sptr &gws, const std::string &setName,
                         const HistogramData::Points &qvmod, const std::vector<int> &sorting_indexes) {

  auto nq = static_cast<int>(qvmod.size()); // number of q-vectors
  std::vector<double> buf(nq * 2);
  try {
    this->dataSetDouble(h5file, setName, buf);
  } catch (H5::Exception &) {
    this->g_log.error("LoadSassena::loadFQ cannot proceed");
    return;
  }

  const std::string gwsName = this->getPropertyValue("OutputWorkspace");

  DataObjects::Workspace2D_sptr ws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 2, nq, nq));
  const std::string wsName = gwsName + std::string("_") + setName;
  ws->setTitle(wsName);

  auto &re = ws->mutableY(0); // store the real part
  ws->setPoints(0, qvmod);    // X-axis values are the modulus of the q vector
  auto &im = ws->mutableY(1); // store the imaginary part
  ws->setPoints(1, qvmod);

  for (int iq = 0; iq < nq; ++iq) {
    auto curr = std::next(buf.cbegin(), 2 * sorting_indexes[iq]);
    re[iq] = *curr;
    im[iq] = *std::next(curr);
  }

  // Set the Units
  ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("MomentumTransfer");

  this->registerWorkspace(gws, wsName, ws, "X-axis: Q-vector modulus; Y-axis: intermediate structure factor");
}

/**
 * Create one workspace to hold the real part and another to hold the imaginary
 * part.
 * We symmetrize the structure factor to negative times
 * Y-values are structure factor for each Q-value
 * X-values are time bins
 * @param h5file file identifier
 * @param gws pointer to WorkspaceGroup being filled
 * @param setName string name of dataset
 * @param qvmod vector of Q-vectors' moduli
 * @param sorting_indexes permutation of qvmod indexes to render it in
 * increasing order of momemtum transfer
 */
void LoadSassena::loadFQT(const H5::H5File &h5file, const API::WorkspaceGroup_sptr &gws, const std::string &setName,
                          const HistogramData::Points &qvmod, const std::vector<int> &sorting_indexes) {

  hsize_t dims[3];
  try {
    this->dataSetInfo(h5file, setName, dims);
  } catch (H5::Exception &) {
    this->g_log.error("Unable to read " + setName + " dataset info");
    this->g_log.error("LoadSassena::loadFQT cannot proceed");
    return;
  }
  auto nnt = static_cast<int>(dims[1]); // number of non-negative time points
  int nt = 2 * nnt - 1;                 // number of time points

  auto nq = static_cast<int>(qvmod.size()); // number of q-vectors
  std::vector<double> buf(nq * nnt * 2);
  try {
    this->dataSetDouble(h5file, setName, buf);
  } catch (H5::Exception &) {
    this->g_log.error("LoadSassena::loadFQT cannot proceed");
    return;
  }

  const std::string gwsName = this->getPropertyValue("OutputWorkspace");
  const double dt = getProperty("TimeUnit"); // time unit increment, in picoseconds;
  DataObjects::Workspace2D_sptr wsRe = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", nq, nt, nt));
  const std::string wsReName = gwsName + std::string("_") + setName + std::string(".Re");
  wsRe->setTitle(wsReName);

  DataObjects::Workspace2D_sptr wsIm = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", nq, nt, nt));
  const std::string wsImName = gwsName + std::string("_") + setName + std::string(".Im");
  wsIm->setTitle(wsImName);

  int origin = nnt - 1;
  for (int iq = 0; iq < nq; iq++) {
    auto &reX = wsRe->mutableX(iq);
    auto &imX = wsIm->mutableX(iq);
    auto &reY = wsRe->mutableY(iq);
    auto &imY = wsIm->mutableY(iq);
    const int index = sorting_indexes[iq];
    auto curr = std::next(buf.cbegin(), index * nnt * 2);
    for (int it = 0; it < nnt; it++) {
      reX[origin + it] = it * dt;  // time point for the real part
      reY[origin + it] = *curr;    // real part of the intermediate structure factor
      reX[origin - it] = -it * dt; // symmetric negative time
      reY[origin - it] = *curr;    // symmetric value for the negative time
      ++curr;
      imX[origin + it] = it * dt;
      imY[origin + it] = *curr;
      imX[origin - it] = -it * dt;
      imY[origin - it] = -(*curr); // antisymmetric value for negative times
      ++curr;
    }
  }

  // Set the Time unit for the X-axis
  wsRe->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Label");
  auto unitPtr = std::dynamic_pointer_cast<Kernel::Units::Label>(wsRe->getAxis(0)->unit());
  unitPtr->setLabel("Time", "picoseconds");

  wsIm->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Label");
  unitPtr = std::dynamic_pointer_cast<Kernel::Units::Label>(wsIm->getAxis(0)->unit());
  unitPtr->setLabel("Time", "picoseconds");

  // Create a numeric axis to replace the default vertical one
  auto verticalAxisRe = std::make_unique<API::NumericAxis>(nq);
  auto verticalAxisIm = std::make_unique<API::NumericAxis>(nq);
  auto verticalAxisReRaw = verticalAxisRe.get();
  auto verticalAxisImRaw = verticalAxisIm.get();
  wsRe->replaceAxis(1, std::move(verticalAxisRe));
  wsIm->replaceAxis(1, std::move(verticalAxisIm));

  // Now set the axis values
  for (int i = 0; i < nq; ++i) {
    verticalAxisReRaw->setValue(i, qvmod[i]);
    verticalAxisImRaw->setValue(i, qvmod[i]);
  }

  // Set the axis units
  verticalAxisReRaw->unit() = Kernel::UnitFactory::Instance().create("MomentumTransfer");
  verticalAxisReRaw->title() = "|Q|";
  verticalAxisImRaw->unit() = Kernel::UnitFactory::Instance().create("MomentumTransfer");
  verticalAxisImRaw->title() = "|Q|";

  // Set the X axis title (for conversion to MD)
  wsRe->getAxis(0)->title() = "Energy transfer";
  wsIm->getAxis(0)->title() = "Energy transfer";

  // Register the workspaces
  registerWorkspace(gws, wsReName, wsRe, "X-axis: time; Y-axis: real part of intermediate structure factor");
  registerWorkspace(gws, wsImName, wsIm, "X-axis: time; Y-axis: imaginary part of intermediate structure factor");
}

/**
 * Initialise the algorithm. Declare properties which can be set before
 * execution (input) or
 * read from after the execution (output).
 */
void LoadSassena::init() {
  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  const std::vector<std::string> exts{".h5", ".hd5"};
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load, exts), "A Sassena file");
  // Declare the OutputWorkspace property
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The name of the group workspace to be created.");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>("TimeUnit", 1.0, Kernel::Direction::Input),
                  "The Time unit in between data points, in picoseconds. "
                  "Default is 1.0 picosec.");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("SortByQVectors", true, Kernel::Direction::Input),
                  "Sort structure factors by increasing momentum transfer?");
}

/**
 * Execute the algorithm.
 */
void LoadSassena::exec() {
  // auto
  // gws=std::dynamic_pointer_cast<API::WorkspaceGroup>(getProperty("OutputWorkspace"));
  // API::WorkspaceGroup_sptr gws=getProperty("OutputWorkspace");
  API::Workspace_sptr ows = getProperty("OutputWorkspace");

  API::WorkspaceGroup_sptr gws = std::dynamic_pointer_cast<API::WorkspaceGroup>(ows);
  if (gws && API::AnalysisDataService::Instance().doesExist(gws->getName())) {
    // gws->deepRemoveAll(); // remove workspace members
    API::AnalysisDataService::Instance().deepRemoveGroup(gws->getName());
  } else {
    gws = std::make_shared<API::WorkspaceGroup>();
    setProperty("OutputWorkspace", std::dynamic_pointer_cast<API::Workspace>(gws));
  }

  // populate m_validSets
  int nvalidSets = 4;
  const char *validSets[] = {"fq", "fq0", "fq2", "fqt"};
  for (int iSet = 0; iSet < nvalidSets; iSet++)
    this->m_validSets.emplace_back(validSets[iSet]);

  // open the HDF5 file for reading
  m_filename = this->getPropertyValue("Filename");
  H5::H5File h5file;
  try {
    h5file = H5::H5File(m_filename.c_str(), H5F_ACC_RDONLY);
  } catch (H5::FileIException &) {
    this->g_log.error("Cannot open " + m_filename);
    throw Kernel::Exception::FileError("Unable to open:", m_filename);
  }

  // Block to read the Q-vectors
  std::vector<int> sorting_indexes;
  const auto qvmod = this->loadQvectors(h5file, gws, sorting_indexes);
  if (qvmod.empty()) {
    this->g_log.error("No Q-vectors read. Unable to proceed");
    h5file.close();
    return;
  }

  // iterate over the valid sets
  for (std::vector<std::string>::const_iterator it = this->m_validSets.begin(); it != this->m_validSets.end(); ++it) {
    std::string setName = *it;
    if (h5file.nameExists(setName)) {
      if (setName == "fq" || setName == "fq0" || setName == "fq2")
        this->loadFQ(h5file, gws, setName, qvmod, sorting_indexes);
      else if (setName == "fqt")
        this->loadFQT(h5file, gws, setName, qvmod, sorting_indexes);
    } else
      this->g_log.information("Dataset " + setName + " not present in file");
  } // end of iterate over the valid sets

  h5file.close();
} // end of LoadSassena::exec()

} // namespace Mantid::DataHandling
