// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/MDEventWSWrapper.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

namespace Mantid {
namespace MDAlgorithms {

/** internal helper function to create empty MDEventWorkspace with nd dimensions
 and set up internal pointer to this workspace
  template parameter:
  * nd -- number of dimensions

 *@param  description : MDWorkspaceDescription memento.
*/
template <size_t nd>
void MDEventWSWrapper::createEmptyEventWS(const MDWSDescription &description) {

  boost::shared_ptr<DataObjects::MDEventWorkspace<DataObjects::MDEvent<nd>, nd>>
      ws = boost::shared_ptr<
          DataObjects::MDEventWorkspace<DataObjects::MDEvent<nd>, nd>>(
          new DataObjects::MDEventWorkspace<DataObjects::MDEvent<nd>, nd>());

  auto numBins = description.getNBins();
  size_t nBins(10); // HACK. this means we have 10 bins artificially. This can't
                    // be right.
  // Give all the dimensions
  for (size_t d = 0; d < nd; d++) {
    if (!numBins.empty())
      nBins = numBins[d];

    Geometry::MDHistoDimension *dim = nullptr;
    if (d < 3 && description.isQ3DMode()) {
      // We should have frame and scale information that we can use correctly
      // for our Q dimensions.
      auto mdFrame = description.getFrame(d);

      dim = new Geometry::MDHistoDimension(
          description.getDimNames()[d], description.getDimIDs()[d], *mdFrame,
          Mantid::coord_t(description.getDimMin()[d]),
          Mantid::coord_t(description.getDimMax()[d]), nBins);

    } else {
      Mantid::Geometry::GeneralFrame frame(description.getDimNames()[d],
                                           description.getDimUnits()[d]);
      dim = new Geometry::MDHistoDimension(
          description.getDimNames()[d], description.getDimIDs()[d], frame,
          Mantid::coord_t(description.getDimMin()[d]),
          Mantid::coord_t(description.getDimMax()[d]), nBins);
    }

    ws->addDimension(Geometry::MDHistoDimension_sptr(dim));
  }
  ws->initialize();

  m_Workspace = ws;
}
/// terminator for attempting initiate 0 dimensions workspace, will throw.
template <>
void MDEventWSWrapper::createEmptyEventWS<0>(
    const MDWSDescription & /*unused*/) {
  throw(std::invalid_argument("MDEventWSWrapper:createEmptyEventWS can not be "
                              "initiated with 0 dimensions"));
}

/** templated by number of dimensions function to add multidimensional data to
the workspace
* it is  expected that all MD coordinates are within the ranges of MD defined
workspace, so no checks are performed

   tempate parameter:
     * nd -- number of dimensions

*@param sigErr   -- pointer to the beginning of 2*data_size array containing
signal and squared error
*@param runIndex -- pointer to the beginning of data_size  containing run index
*@param detId    -- pointer to the beginning of dataSize array containing
detector id-s
*@param Coord    -- pointer to the beginning of dataSize*nd array containing the
coordinates of nd-dimensional events
*
*@param dataSize -- the length of the vector of MD events
*/
template <size_t nd>
void MDEventWSWrapper::addMDDataND(float *sigErr, uint16_t *runIndex,
                                   uint32_t *detId, coord_t *Coord,
                                   size_t dataSize) const {

  auto *const pWs = dynamic_cast<
      DataObjects::MDEventWorkspace<DataObjects::MDEvent<nd>, nd> *>(
      m_Workspace.get());
  if (pWs) {
    for (size_t i = 0; i < dataSize; i++) {
      pWs->addEvent(DataObjects::MDEvent<nd>(
          *(sigErr + 2 * i), *(sigErr + 2 * i + 1), *(runIndex + i),
          *(detId + i), (Coord + i * nd)));
    }
  } else {
    auto *const pLWs = dynamic_cast<
        DataObjects::MDEventWorkspace<DataObjects::MDLeanEvent<nd>, nd> *>(
        m_Workspace.get());

    if (!pLWs)
      throw std::runtime_error("Bad Cast: Target MD workspace to add events "
                               "does not correspond to type of events you try "
                               "to add to it");

    for (size_t i = 0; i < dataSize; i++) {
      pLWs->addEvent(DataObjects::MDLeanEvent<nd>(
          *(sigErr + 2 * i), *(sigErr + 2 * i + 1), (Coord + i * nd)));
    }
  }
}

/// the function used in template metaloop termination on 0 dimensions and to
/// throw the error in attempt to add data to 0-dimension workspace
template <>
void MDEventWSWrapper::addMDDataND<0>(float * /*unused*/, uint16_t * /*unused*/,
                                      uint32_t * /*unused*/,
                                      coord_t * /*unused*/,
                                      size_t /*unused*/) const {
  throw(std::invalid_argument(" class has not been initiated, can not add data "
                              "to 0-dimensional workspace"));
}

/***/
template <size_t nd> void MDEventWSWrapper::splitBoxList() {
  auto *const pWs = dynamic_cast<
      DataObjects::MDEventWorkspace<DataObjects::MDEvent<nd>, nd> *>(
      m_Workspace.get());
  if (!pWs)
    throw(std::bad_cast());

  m_needSplitting = false;
}

template <> void MDEventWSWrapper::splitBoxList<0>() {
  throw(std::invalid_argument(" class has not been initiated, can not split "
                              "0-dimensional workspace boxes"));
}

/// helper function to refresh centroid on MDEventWorkspace with nd dimensions
template <size_t nd> void MDEventWSWrapper::calcCentroidND() {

  auto *const pWs = dynamic_cast<
      DataObjects::MDEventWorkspace<DataObjects::MDEvent<nd>, nd> *>(
      this->m_Workspace.get());
  if (!pWs)
    throw(std::bad_cast());

  // pWs->getBox()->refreshCentroid(NULL);
}
/// the function used in template metaloop termination on 0 dimensions and as
/// the function which will throw the error on undefined MDWorkspaceWrapper
template <> void MDEventWSWrapper::calcCentroidND<0>() {
  throw(std::invalid_argument(" class has not been initiated"));
}

/**function returns the number of dimensions in current MDEvent workspace or
 * throws if the workspace has not been defined */
size_t MDEventWSWrapper::nDimensions() const {
  if (m_NDimensions == 0)
    throw(std::invalid_argument("The workspace has not been initiated yet"));
  return size_t(m_NDimensions);
}

/** function creates empty MD event workspace with given parameters (workspace
 *factory) and stores internal pointer to this workspace for further usage.
 *  IT ASLO SETS UP W-TRANSFORMATON. TODO: reconcile w-transformation with MD
 *geometry.
 *
 *@param WSD the class which describes an MD workspace
 *
 *@returns shared pointer to the created workspace
 */
API::IMDEventWorkspace_sptr
MDEventWSWrapper::createEmptyMDWS(const MDWSDescription &WSD) {

  if (WSD.nDimensions() < 1 || WSD.nDimensions() > MAX_N_DIM) {
    std::string ERR = " Number of requested MD dimensions: " +
                      std::to_string(WSD.nDimensions()) +
                      " exceeds maximal number of MD dimensions: " +
                      std::to_string(static_cast<int>(MAX_N_DIM)) +
                      " instantiated during compilation\n";
    throw(std::invalid_argument(ERR));
  }

  m_NDimensions = static_cast<int>(WSD.nDimensions());
  // call the particular function, which creates the workspace with n_dimensions
  (this->*(wsCreator[m_NDimensions]))(WSD);

  // set up the matrix, which convert momentums from Q in orthogonal crystal
  // coordinate system and units of Angstrom^-1 to hkl or orthogonal hkl or
  // whatever
  m_Workspace->setWTransf(WSD.m_Wtransf);
  return m_Workspace;
}

/// set up existing workspace pointer as internal pointer for the class to
/// perform proper MD operations on this workspace
void MDEventWSWrapper::setMDWS(API::IMDEventWorkspace_sptr spWS) {
  m_Workspace = spWS;
  m_NDimensions = m_Workspace->getNumDims();
}

/** method adds the data to the workspace which was initiated before;
 *@param sigErr   -- pointer to the beginning of 2*data_size array containing
 *signal and squared error
 *@param runIndex -- pointer to the beginnign of data_size  containing run index
 *@param detId    -- pointer to the beginning of dataSize array containing
 *detector id-s
 *@param Coord    -- pointer to the beginning of dataSize*nd array containig the
 *coordinates od nd-dimensional events
 *
 *@param dataSize -- the length of the vector of MD events
 */
void MDEventWSWrapper::addMDData(std::vector<float> &sigErr,
                                 std::vector<uint16_t> &runIndex,
                                 std::vector<uint32_t> &detId,
                                 std::vector<coord_t> &Coord,
                                 size_t dataSize) const {

  if (dataSize == 0)
    return;
  // perform the actual dimension-dependent addition
  (this->*(mdEvAddAndForget[m_NDimensions]))(&sigErr[0], &runIndex[0],
                                             &detId[0], &Coord[0], dataSize);
}

/** method should be called at the end of the algorithm, to let the workspace
manager know that it has whole responsibility for the workspace
(As the algorithm is static, it will hold the pointer to the workspace
otherwise, not allowing the WS manager to delete WS on request or when it finds
this usefull)*/
void MDEventWSWrapper::releaseWorkspace() {
  // decrease the sp count by one
  m_Workspace.reset();
  // mark the number of dimensions invalid;
  m_NDimensions = 0;
}

// the class instantiated by compiler at compilation time and generates the map,
// between the number of dimensions and the function, which process this number
// of dimensions
template <size_t i> class LOOP {
public:
  static inline void EXEC(MDEventWSWrapper *pH) {
    LOOP<i - 1>::EXEC(pH);
    pH->wsCreator[i] = &MDEventWSWrapper::createEmptyEventWS<i>;
    pH->mdEvAddAndForget[i] = &MDEventWSWrapper::addMDDataND<i>;
    pH->mdCalCentroid[i] = &MDEventWSWrapper::calcCentroidND<i>;
    pH->mdBoxListSplitter[i] = &MDEventWSWrapper::splitBoxList<i>;
  }
};
// the class terminates the compitlation-time metaloop and sets up functions
// which process 0-dimension workspace operations
template <> class LOOP<0> {
public:
  static inline void EXEC(MDEventWSWrapper *pH) {
    pH->wsCreator[0] = &MDEventWSWrapper::createEmptyEventWS<0>;
    pH->mdEvAddAndForget[0] = &MDEventWSWrapper::addMDDataND<0>;
    pH->mdCalCentroid[0] = &MDEventWSWrapper::calcCentroidND<0>;
    pH->mdBoxListSplitter[0] = &MDEventWSWrapper::splitBoxList<0>;
  }
};

/**constructor */
MDEventWSWrapper::MDEventWSWrapper()
    : m_NDimensions(0), m_needSplitting(false) {
  wsCreator.resize(MAX_N_DIM + 1);
  mdEvAddAndForget.resize(MAX_N_DIM + 1);
  mdCalCentroid.resize(MAX_N_DIM + 1);
  mdBoxListSplitter.resize(MAX_N_DIM + 1);
  LOOP<MAX_N_DIM>::EXEC(this);
}

} // namespace MDAlgorithms
} // namespace Mantid
