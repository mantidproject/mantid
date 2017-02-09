#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidKernel/System.h"
#include "MantidNexus/NexusClasses.h"

Mantid::DataHandling::LoadHelper loadHelper;

namespace Mantid {
namespace DataHandling {

/** LoadILLReflectometry : Loads a ILL Reflectometry data file.

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
class DLLExport LoadILLReflectometry
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLReflectometry() = default;
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string name() const override {
    return "LoadILLReflectometry";
  }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1;}
  /// Algorithm's category for search and find. @see Algorithm::category
  const std::string category() const override {return "DataHandling\\Nexus";}
  /// Algorithm's summary. @see Algorithm::summary
  const std::string summary() const override {
    return "Loads an ILL reflectometry nexus file (instruments D17 and Figaro).";
  }
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;

  void initWorkspace(NeXus::NXEntry &entry,
                     std::vector<std::vector<int>> monitorsData, std::string &filename);
  void setInstrumentName(const NeXus::NXEntry &firstEntry,
                         const std::string &instrumentNamePath);
  void loadDataDetails(NeXus::NXEntry &entry);
  void getXValues(std::vector<double> &xVals);
  void loadData(NeXus::NXEntry &entry,
                                std::vector<std::vector<int>> monitorsData, std::vector<double> &xVals);
  void loadNexusEntriesIntoProperties(std::string nexusfilename);
  std::vector<int>loadSingleMonitor(NeXus::NXEntry &entry, std::string monitor_data);
  std::vector<std::vector<int>> loadMonitors(NeXus::NXEntry &entry);
  void runLoadInstrument();
  void placeDetector();

  API::MatrixWorkspace_sptr m_localWorkspace;

  /* Values parsed from the nexus file */
  std::string m_instrumentName; ///< Name of the instrument
  size_t m_numberOfTubes{0};         // number of tubes - X
  size_t m_numberOfPixelsPerTube{0}; // number of pixels per tube - Y
  size_t m_numberOfChannels{0};      // time channels - Z
  size_t m_numberOfHistograms{0};
  double m_wavelength{0};
  double m_channelWidth{0};
  std::unordered_set<std::string> m_supportedInstruments{"D17", "d17", "Figaro"};
  LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_ */
