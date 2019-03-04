// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_
#define MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_

#include "MantidAPI/Progress.h"
#include "MantidAPI/SerialAlgorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <boost/optional.hpp>
#include <climits>
#include <nexus/NeXusFile.hpp>

namespace Mantid {
namespace NeXus {
class NexusFileIO;
}
namespace DataHandling {
/** @class SaveNexusProcessed SaveNexusProcessed.h
DataHandling/SaveNexusProcessed.h

Saves a workspace as a Nexus Processed entry in a Nexus file.
SaveNexusProcessed is an algorithm and as such inherits
from the Algorithm class, via DataHandlingCommand, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> Filename - The name of and path to the output NeXus file (may exist) </LI>
<LI> InputWorkspace - The name of the workspace to store the file </LI>
<LI> Title - the title to describe the saved processed data
</UL>
*/
class DLLExport SaveNexusProcessed : public API::SerialAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveNexusProcessed"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The SaveNexusProcessed algorithm will write the given Mantid "
           "workspace to a Nexus file. SaveNexusProcessed may be invoked by "
           "SaveNexus.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveISISNexus", "SaveNexus", "LoadNexusProcessed"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

  void saveSpectraDetectorMapNexus(
      const API::MatrixWorkspace &ws, ::NeXus::File *file,
      const std::vector<int> &wsIndices,
      const ::NeXus::NXcompression compression = ::NeXus::LZW) const;

  void saveSpectrumNumbersNexus(
      const API::MatrixWorkspace &ws, ::NeXus::File *file,
      const std::vector<int> &wsIndices,
      const ::NeXus::NXcompression compression = ::NeXus::LZW) const;

protected:
  /// Override process groups
  bool processGroups() override;

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  void getWSIndexList(std::vector<int> &indices,
                      Mantid::API::MatrixWorkspace_const_sptr matrixWorkspace);

  template <class T>
  static void appendEventListData(const std::vector<T> &events, size_t offset,
                                  double *tofs, float *weights,
                                  float *errorSquareds, int64_t *pulsetimes);

  void execEvent(Mantid::NeXus::NexusFileIO *nexusFile,
                 const bool uniformSpectra, const std::vector<int> &spec);
  /// sets non workspace properties for the algorithm
  void setOtherProperties(IAlgorithm *alg, const std::string &propertyName,
                          const std::string &propertyValue,
                          int perioidNum) override;
  /// execute the algorithm.
  void doExec(Mantid::API::Workspace_sptr inputWorkspace,
              boost::shared_ptr<Mantid::NeXus::NexusFileIO> &nexusFile,
              const bool keepFile = false,
              boost::optional<size_t> entryNumber = boost::optional<size_t>());

  /// Pointer to the local workspace
  API::MatrixWorkspace_const_sptr m_inputWorkspace;
  /// Pointer to the local workspace, cast to EventWorkspace
  DataObjects::EventWorkspace_const_sptr m_eventWorkspace;
  /// Proportion of progress time expected to write initial part
  double m_timeProgInit{0.0};
  /// Progress bar
  std::unique_ptr<API::Progress> m_progress;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_*/
