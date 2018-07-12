#ifndef MANTID_DATAHANDLING_LOADILLINDIRECT2_H_
#define MANTID_DATAHANDLING_LOADILLINDIRECT2_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/**
  Loads an ILL IN16B nexus file into a Mantid workspace.

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport LoadILLIndirect2
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLIndirect2();
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a ILL/IN16B nexus file.";
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  void loadDataDetails(NeXus::NXEntry &entry);
  void initWorkSpace(NeXus::NXEntry &entry,
                     std::vector<std::vector<int>> monitorsData);
  void setInstrumentName(const NeXus::NXEntry &firstEntry,
                         const std::string &instrumentNamePath);
  void loadNexusEntriesIntoProperties(std::string nexusfilename);
  void loadDataIntoTheWorkSpace(NeXus::NXEntry &entry,
                                std::vector<std::vector<int>> monitorsData);
  std::vector<std::vector<int>> loadMonitors(NeXus::NXEntry &entry);

  void runLoadInstrument();

  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }

  void moveComponent(const std::string &, double);
  void moveSingleDetectors(NeXus::NXEntry &entry);

  API::MatrixWorkspace_sptr m_localWorkspace;

  std::string m_instrumentName; ///< Name of the instrument

  // Variables describing the data in the detector
  size_t m_numberOfTubes;           // number of tubes - X
  size_t m_numberOfPixelsPerTube;   // number of pixels per tube - Y
  size_t m_numberOfChannels;        // time channels - Z
  size_t m_numberOfSimpleDetectors; // number of simple detector
  size_t m_numberOfHistograms;      // number of spectra
  size_t m_numberOfMonitors;        // number of monitor spectra
  std::set<int> m_activeSDIndices;  // set of Single Detector indices,
                                    // that were actually active

  std::vector<std::string> m_supportedInstruments;
  LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLINDIRECT2_H_ */
