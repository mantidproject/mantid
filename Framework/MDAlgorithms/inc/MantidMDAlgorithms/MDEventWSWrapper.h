// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_MDEVENT_WS_WRAPPER_H
#define MANTID_MDALGORITHMS_MDEVENT_WS_WRAPPER_H

#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/MDWSDescription.h"

namespace Mantid {
namespace MDAlgorithms {
/**  The class which wraps MD Events factory and allow to work with a
N-dimensional templated MDEvent workspace like usuall class with n-dimension as
a parameter
*
*   Introduced to decrease code bloat and increase efficiency of methods and
algorithms, which use DataObjects write interface and run-time defined number of
dimensions

@date 2011-28-12
*/
/// vectors of strings are often used here
using Strings = std::vector<std::string>;

/// predefenition of the class name
class MDEventWSWrapper;
// NOTICE: There is need to work with bare class-function pointers here, as
// Boost function pointers with multiple arguments
//        appear not portable to all architectures supported (Fail on MAC)
/// signature to void templated function
using fpVoidMethod = void (MDEventWSWrapper::*)();
/// signature for the internal templated function pointer to add data to an
/// existing workspace
using fpAddData = void (MDEventWSWrapper::*)(float *, uint16_t *, uint32_t *,
                                             coord_t *, size_t) const;
/// signature for the internal templated function pointer to create workspace
using fpCreateWS = void (MDEventWSWrapper::*)(const MDWSDescription &);

class DLLExport MDEventWSWrapper {
public:
  MDEventWSWrapper();
  virtual ~MDEventWSWrapper() = default;
  /// get maximal number of dimensions, allowed for the algorithm and embedded
  /// in algorithm during compilation time.
  static size_t getMaxNDim() { return MAX_N_DIM; }

  /// get number of dimensions, for the workspace, currently accessed by the
  /// algorithm.
  size_t nDimensions() const;
  /** function creates empty MD event workspace with given parameters (workspace
   * factory) and stores internal pointer to this workspace for further usage */
  API::IMDEventWorkspace_sptr createEmptyMDWS(const MDWSDescription &WSD);
  /// add the data to the internal workspace. The workspace has to exist and be
  /// initiated
  void addMDData(std::vector<float> &sigErr, std::vector<uint16_t> &runIndex,
                 std::vector<uint32_t> &detId, std::vector<coord_t> &Coord,
                 size_t dataSize) const;
  /// releases the shared pointer to the MD workspace, stored by the class and
  /// makes the class instance undefined;
  void releaseWorkspace();
  /// get access to the internal workspace
  API::IMDEventWorkspace_sptr pWorkspace() { return m_Workspace; }
  // should it be moved to the IDataObjects?
  // void refreshCentroid(){ (this->*(mdCalCentroid[m_NDimensions]))();   };
  /** initiate the class with pointer to existing MD workspace */
  void setMDWS(API::IMDEventWorkspace_sptr spWS);

  /// the accessor verify if there are boxes in box-splitter cash which need
  /// splitting;
  bool ifNeedsSplitting() const { return m_needSplitting; }
  /// method splits list of boxes not yet uses thread sheduler but may be later
  void splitList(Kernel::ThreadScheduler *) {
    (this->*(mdBoxListSplitter[m_NDimensions]))();
  }

private:
  /// maximal nuber of dimensions, currently supported by the class;
  enum { MAX_N_DIM = 8 };
  /// actual number of dimensions, initiated in current MD workspace; 0 if not
  /// initated;
  size_t m_NDimensions;
  /// pointer to taret  MD workspace:
  API::IMDEventWorkspace_sptr m_Workspace;

  /// VECTORS OF FUNCTION POINTERS to different number of dimensions methdods
  /// vector holding function pointers to the code, creating different number of
  /// dimension worspace as function of dimensions number
  std::vector<fpCreateWS> wsCreator;
  /// vector holding function pointers to the code, which adds diffrent
  /// dimension number events to the workspace
  std::vector<fpAddData> mdEvAddAndForget;
  /// vector holding function pointers to the code, which refreshes centroid
  /// (could it be moved to IMD?)
  std::vector<fpVoidMethod> mdCalCentroid;
  /// vector holding function pointers to the code, which split list of boxes
  /// need splitting
  std::vector<fpVoidMethod> mdBoxListSplitter;

  // helper class to generate methaloop on MD workspaces dimensions:
  template <size_t i> friend class LOOP;

  // internal function tempates to generate as function of dimensions and
  // assightn to function pointers
  template <size_t nd>
  void addMDDataND(float *sigErr, uint16_t *runIndex, uint32_t *detId,
                   coord_t *Coord, size_t dataSize) const;
  template <size_t nd>
  void addAndTraceMDDataND(float *sig_err, uint16_t *run_index,
                           uint32_t *det_id, coord_t *Coord,
                           size_t data_size) const;

  template <size_t nd> void calcCentroidND();

  template <size_t nd>
  void createEmptyEventWS(const MDWSDescription &description);

  template <size_t nd> void splitBoxList(); // for the time being
  // void splitBoxList(Kernel::ThreadScheduler * ts);

  // the variable, which informs the user of MD Event WS wrapper that there are
  // boxes to split; Very simple for the time being
  mutable bool m_needSplitting;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif
