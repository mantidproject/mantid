// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_LOADLIVEDATA_H_
#define MANTID_LIVEDATA_LOADLIVEDATA_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/System.h"
#include "MantidLiveData/LiveDataAlgorithm.h"

namespace Mantid {
namespace LiveData {

/** Algorithm to load a chunk of live data.
 * Called by StartLiveData and MonitorLiveData

  @date 2012-02-16
*/
class DLLExport LoadLiveData : public LiveDataAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load a chunk of live data. You should call StartLiveData, and not "
           "this algorithm directly.";
  }

  const std::string category() const override;
  int version() const override;

  void exec() override;

private:
  void init() override;

  Mantid::API::Workspace_sptr runProcessing(Mantid::API::Workspace_sptr inputWS,
                                            bool PostProcess);
  Mantid::API::Workspace_sptr processChunk(Mantid::API::Workspace_sptr chunkWS);
  void runPostProcessing();

  void replaceChunk(Mantid::API::Workspace_sptr chunkWS);
  void addChunk(Mantid::API::Workspace_sptr chunkWS);
  void addMatrixWSChunk(API::Workspace_sptr accumWS,
                        API::Workspace_sptr chunkWS);
  void addMDWSChunk(API::Workspace_sptr &accumWS,
                    const API::Workspace_sptr &chunkWS);
  void appendChunk(Mantid::API::Workspace_sptr chunkWS);
  API::Workspace_sptr appendMatrixWSChunk(API::Workspace_sptr accumWS,
                                          Mantid::API::Workspace_sptr chunkWS);
  void updateDefaultBinBoundaries(API::Workspace *workspace);

  /// The "accumulation" workspace = after adding, but before post-processing
  Mantid::API::Workspace_sptr m_accumWS;

  /// The final output = the post-processed accumulation workspace
  Mantid::API::Workspace_sptr m_outputWS;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_LOADLIVEDATA_H_ */
