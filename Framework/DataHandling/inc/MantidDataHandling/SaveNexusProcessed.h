#ifndef MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_
#define MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <nexus/NeXusFile.hpp>
#include <boost/optional.hpp>
#include <climits>

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

Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SaveNexusProcessed : public API::Algorithm {
public:
  /// Default constructor
  SaveNexusProcessed();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveNexusProcessed"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The SaveNexusProcessed algorithm will write the given Mantid "
           "workspace to a Nexus file. SaveNexusProcessed may be invoked by "
           "SaveNexus.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

  void saveSpectraMapNexus(
      const API::MatrixWorkspace &ws, ::NeXus::File *file,
      const std::vector<int> &spec,
      const ::NeXus::NXcompression compression = ::NeXus::LZW) const;

protected:
  /// Override process groups
  bool processGroups() override;

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  void getSpectrumList(std::vector<int> &spec,
                       Mantid::API::MatrixWorkspace_const_sptr matrixWorkspace);

  template <class T>
  static void appendEventListData(std::vector<T> events, size_t offset,
                                  double *tofs, float *weights,
                                  float *errorSquareds, int64_t *pulsetimes);

  void execEvent(Mantid::NeXus::NexusFileIO *nexusFile,
                 const bool uniformSpectra, const std::vector<int> spec);
  /// sets non workspace properties for the algorithm
  void setOtherProperties(IAlgorithm *alg, const std::string &propertyName,
                          const std::string &propertyValue,
                          int perioidNum) override;
  /// execute the algorithm.
  void doExec(Mantid::API::Workspace_sptr inputWorkspace,
              boost::shared_ptr<Mantid::NeXus::NexusFileIO> &nexusFile,
              const bool keepFile = false,
              boost::optional<size_t> entryNumber = boost::optional<size_t>());

  /// The name and path of the input file
  std::string m_filename;
  /// The name and path of the input file
  std::string m_entryname;
  /// The title of the processed data section
  std::string m_title;
  /// Pointer to the local workspace
  API::MatrixWorkspace_const_sptr m_inputWorkspace;
  /// Pointer to the local workspace, cast to EventWorkspace
  DataObjects::EventWorkspace_const_sptr m_eventWorkspace;
  /// Proportion of progress time expected to write initial part
  double m_timeProgInit;
  /// Progress bar
  std::unique_ptr<API::Progress> m_progress;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_*/
