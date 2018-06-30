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

#include <hdf5_hl.h>

namespace Mantid {
namespace DataHandling {

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadSassena)

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSassena::confidence(Kernel::NexusDescriptor &descriptor) const {
  if (descriptor.hasRootAttr("sassena_version") ||
      descriptor.pathExists("/qvectors")) {
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
void LoadSassena::registerWorkspace(API::WorkspaceGroup_sptr gws,
                                    const std::string wsName,
                                    DataObjects::Workspace2D_sptr ws,
                                    const std::string &description) {
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

herr_t LoadSassena::dataSetInfo(const hid_t &h5file, const std::string setName,
                                hsize_t *dims) const {
  H5T_class_t class_id;
  size_t type_size;
  herr_t errorcode = H5LTget_dataset_info(h5file, setName.c_str(), dims,
                                          &class_id, &type_size);
  if (errorcode < 0) {
    g_log.error("Unable to read " + setName + " dataset info");
  }
  return errorcode;
}

/**
 * Read the dataset
 * @param h5file file identifier
 * @param setName string name of dataset
 * @param buf storing dataset
 */
herr_t LoadSassena::dataSetDouble(const hid_t &h5file,
                                  const std::string setName,
                                  std::vector<double> &buf) {
  herr_t errorcode =
      H5LTread_dataset_double(h5file, setName.c_str(), buf.data());
  if (errorcode < 0) {
    this->g_log.error("Cannot read " + setName + " dataset");
  }
  return errorcode;
}

/* Helper object and function to sort modulus of Q-vectors
 */
using mypair = std::pair<double, int>;
bool compare(const mypair &left, const mypair &right) {
  return left.first < right.first;
}
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
HistogramData::Points
LoadSassena::loadQvectors(const hid_t &h5file, API::WorkspaceGroup_sptr gws,
                          std::vector<int> &sorting_indexes) {

  // store the modulus of the vector
  std::vector<double> qvmod;

  const std::string gwsName = this->getPropertyValue("OutputWorkspace");
  const std::string setName("qvectors");

  hsize_t dims[3];
  if (dataSetInfo(h5file, setName, dims) < 0) {
    throw Kernel::Exception::FileError(
        "Unable to read " + setName + " dataset info:", m_filename);
  }
  int nq = static_cast<int>(dims[0]); // number of q-vectors
  std::vector<double> buf(nq * 3);

  herr_t errorcode = this->dataSetDouble(h5file, "qvectors", buf);
  if (errorcode < 0) {
    this->g_log.error("LoadSassena::loadQvectors cannot proceed");
    qvmod.resize(0);
    return HistogramData::Points(std::move(qvmod));
  }

  qvmod.reserve(nq);
  for (auto curr = buf.cbegin(); curr != buf.cend(); curr += 3) {
    qvmod.push_back(
        sqrt(curr[0] * curr[0] + curr[1] * curr[1] + curr[2] * curr[2]));
  }

  if (getProperty("SortByQVectors")) {
    std::vector<mypair> qvmodpair;
    qvmodpair.reserve(nq);
    for (int iq = 0; iq < nq; iq++)
      qvmodpair.emplace_back(qvmod[iq], iq);
    std::sort(qvmodpair.begin(), qvmodpair.end(), compare);
    for (int iq = 0; iq < nq; iq++)
      sorting_indexes.push_back(qvmodpair[iq].second);
    std::sort(qvmod.begin(), qvmod.end());
  } else
    for (int iq = 0; iq < nq; iq++)
      sorting_indexes.push_back(iq);

  DataObjects::Workspace2D_sptr ws =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", nq, 3, 3));
  std::string wsName = gwsName + std::string("_") + setName;
  ws->setTitle(wsName);

  for (int iq = 0; iq < nq; iq++) {
    auto &Y = ws->mutableY(iq);
    auto curr = std::next(buf.cbegin(), 3 * sorting_indexes[iq]);
    Y.assign(curr, std::next(curr, 3));
  }

  ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create(
      "MomentumTransfer"); // Set the Units

  this->registerWorkspace(
      gws, wsName, ws, "X-axis: origin of Q-vectors; Y-axis: tip of Q-vectors");
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
void LoadSassena::loadFQ(const hid_t &h5file, API::WorkspaceGroup_sptr gws,
                         const std::string setName,
                         const HistogramData::Points &qvmod,
                         const std::vector<int> &sorting_indexes) {

  int nq = static_cast<int>(qvmod.size()); // number of q-vectors
  std::vector<double> buf(nq * 2);
  herr_t errorcode = this->dataSetDouble(h5file, setName, buf);
  if (errorcode < 0) {
    this->g_log.error("LoadSassena::loadFQ cannot proceed");
    return;
  }

  const std::string gwsName = this->getPropertyValue("OutputWorkspace");

  DataObjects::Workspace2D_sptr ws =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
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
  ws->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("MomentumTransfer");

  this->registerWorkspace(
      gws, wsName, ws,
      "X-axis: Q-vector modulus; Y-axis: intermediate structure factor");
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
void LoadSassena::loadFQT(const hid_t &h5file, API::WorkspaceGroup_sptr gws,
                          const std::string setName,
                          const HistogramData::Points &qvmod,
                          const std::vector<int> &sorting_indexes) {

  hsize_t dims[3];
  if (dataSetInfo(h5file, setName, dims) < 0) {
    this->g_log.error("Unable to read " + setName + " dataset info");
    this->g_log.error("LoadSassena::loadFQT cannot proceed");
    return;
  }
  int nnt = static_cast<int>(dims[1]); // number of non-negative time points
  int nt = 2 * nnt - 1;                // number of time points

  int nq = static_cast<int>(qvmod.size()); // number of q-vectors
  std::vector<double> buf(nq * nnt * 2);
  herr_t errorcode = this->dataSetDouble(h5file, setName, buf);
  if (errorcode < 0) {
    this->g_log.error("LoadSassena::loadFQT cannot proceed");
    return;
  }

  const std::string gwsName = this->getPropertyValue("OutputWorkspace");
  const double dt =
      getProperty("TimeUnit"); // time unit increment, in picoseconds;
  DataObjects::Workspace2D_sptr wsRe =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", nq, nt, nt));
  const std::string wsReName =
      gwsName + std::string("_") + setName + std::string(".Re");
  wsRe->setTitle(wsReName);

  DataObjects::Workspace2D_sptr wsIm =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", nq, nt, nt));
  const std::string wsImName =
      gwsName + std::string("_") + setName + std::string(".Im");
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
      reX[origin + it] = it * dt; // time point for the real part
      reY[origin + it] =
          *curr; // real part of the intermediate structure factor
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
  auto unitPtr = boost::dynamic_pointer_cast<Kernel::Units::Label>(
      wsRe->getAxis(0)->unit());
  unitPtr->setLabel("Time", "picoseconds");

  wsIm->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Label");
  unitPtr = boost::dynamic_pointer_cast<Kernel::Units::Label>(
      wsIm->getAxis(0)->unit());
  unitPtr->setLabel("Time", "picoseconds");

  // Create a numeric axis to replace the default vertical one
  API::Axis *const verticalAxisRe = new API::NumericAxis(nq);
  API::Axis *const verticalAxisIm = new API::NumericAxis(nq);

  wsRe->replaceAxis(1, verticalAxisRe);
  wsIm->replaceAxis(1, verticalAxisIm);

  // Now set the axis values
  for (int i = 0; i < nq; ++i) {
    verticalAxisRe->setValue(i, qvmod[i]);
    verticalAxisIm->setValue(i, qvmod[i]);
  }

  // Set the axis units
  verticalAxisRe->unit() =
      Kernel::UnitFactory::Instance().create("MomentumTransfer");
  verticalAxisRe->title() = "|Q|";
  verticalAxisIm->unit() =
      Kernel::UnitFactory::Instance().create("MomentumTransfer");
  verticalAxisIm->title() = "|Q|";

  // Set the X axis title (for conversion to MD)
  wsRe->getAxis(0)->title() = "Energy transfer";
  wsIm->getAxis(0)->title() = "Energy transfer";

  // Register the workspaces
  registerWorkspace(
      gws, wsReName, wsRe,
      "X-axis: time; Y-axis: real part of intermediate structure factor");
  registerWorkspace(
      gws, wsImName, wsIm,
      "X-axis: time; Y-axis: imaginary part of intermediate structure factor");
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
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Load, exts),
                  "A Sassena file");
  // Declare the OutputWorkspace property
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<API::Workspace>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "The name of the group workspace to be created.");
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<double>>(
                      "TimeUnit", 1.0, Kernel::Direction::Input),
                  "The Time unit in between data points, in picoseconds. "
                  "Default is 1.0 picosec.");
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<bool>>(
                      "SortByQVectors", true, Kernel::Direction::Input),
                  "Sort structure factors by increasing momentum transfer?");
}

/**
 * Execute the algorithm.
 */
void LoadSassena::exec() {
  // auto
  // gws=boost::dynamic_pointer_cast<API::WorkspaceGroup>(getProperty("OutputWorkspace"));
  // API::WorkspaceGroup_sptr gws=getProperty("OutputWorkspace");
  API::Workspace_sptr ows = getProperty("OutputWorkspace");

  API::WorkspaceGroup_sptr gws =
      boost::dynamic_pointer_cast<API::WorkspaceGroup>(ows);
  if (gws && API::AnalysisDataService::Instance().doesExist(gws->getName())) {
    // gws->deepRemoveAll(); // remove workspace members
    API::AnalysisDataService::Instance().deepRemoveGroup(gws->getName());
  } else {
    gws = boost::make_shared<API::WorkspaceGroup>();
    setProperty("OutputWorkspace",
                boost::dynamic_pointer_cast<API::Workspace>(gws));
  }

  // populate m_validSets
  int nvalidSets = 4;
  const char *validSets[] = {"fq", "fq0", "fq2", "fqt"};
  for (int iSet = 0; iSet < nvalidSets; iSet++)
    this->m_validSets.push_back(validSets[iSet]);

  // open the HDF5 file for reading
  m_filename = this->getPropertyValue("Filename");
  hid_t h5file = H5Fopen(m_filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (h5file < 0) {
    this->g_log.error("Cannot open " + m_filename);
    throw Kernel::Exception::FileError("Unable to open:", m_filename);
  }

  // find out the sassena version used
  char cversion[16];
  if (H5LTget_attribute_string(h5file, "/", "sassena_version", cversion) < 0) {
    this->g_log.error("Unable to read Sassena version");
  }
  // const std::string version(cversion);
  // determine which loader protocol to use based on the version
  // to be done at a later time, maybe implement a Version class

  // Block to read the Q-vectors
  std::vector<int> sorting_indexes;
  const auto qvmod = this->loadQvectors(h5file, gws, sorting_indexes);
  if (qvmod.empty()) {
    this->g_log.error("No Q-vectors read. Unable to proceed");
    H5Fclose(h5file);
    return;
  }

  // iterate over the valid sets
  std::string setName;
  for (std::vector<std::string>::const_iterator it = this->m_validSets.begin();
       it != this->m_validSets.end(); ++it) {
    setName = *it;
    if (H5Lexists(h5file, setName.c_str(), H5P_DEFAULT)) {
      if (setName == "fq" || setName == "fq0" || setName == "fq2")
        this->loadFQ(h5file, gws, setName, qvmod, sorting_indexes);
      else if (setName == "fqt")
        this->loadFQT(h5file, gws, setName, qvmod, sorting_indexes);
    } else
      this->g_log.information("Dataset " + setName + " not present in file");
  } // end of iterate over the valid sets

  H5Fclose(h5file);
} // end of LoadSassena::exec()

} // namespace DataHandling
} // namespace Mantid
