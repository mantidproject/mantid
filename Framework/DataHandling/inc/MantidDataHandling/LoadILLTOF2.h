#ifndef MANTID_DATAHANDLING_LOADILLTOF2_H_
#define MANTID_DATAHANDLING_LOADILLTOF2_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {
/**
 Loads an ILL IN4/5/6 nexus file into a Mantid workspace.

 Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadILLTOF2 : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Constructor
  LoadILLTOF2();
  /// Algorithm's name
  const std::string name() const override { return "LoadILLTOF"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads an ILL TOF NeXus file.";
  }

  /// Algorithm's version
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Nexus;ILL\\Direct";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  const static std::vector<std::string> SUPPORTED_INSTRUMENTS;

  // Initialisation code
  void init() override;
  // Execution code
  void exec() override;

  void loadInstrumentDetails(NeXus::NXEntry &);
  std::vector<std::vector<int>> getMonitorInfo(NeXus::NXEntry &firstEntry);
  void initWorkSpace(NeXus::NXEntry &entry,
                     const std::vector<std::vector<int>> &);
  void initInstrumentSpecific();
  void addAllNexusFieldsAsProperties(const std::string &filename);
  void addEnergyToRun();
  void addFacility();
  void addPulseInterval();

  void loadTimeDetails(NeXus::NXEntry &entry);
  void loadDataIntoTheWorkSpace(NeXus::NXEntry &entry,
                                const std::vector<std::vector<int>> &);
  void loadSpectra(size_t &spec, const size_t numberOfTubes,
                   const std::vector<Mantid::detid_t> &detectorIDs,
                   const NeXus::NXInt &data, Mantid::API::Progress &progress);

  void runLoadInstrument();

  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }

  API::MatrixWorkspace_sptr m_localWorkspace;

  std::string m_instrumentName = ""; ///< Name of the instrument
  std::string m_instrumentPath = ""; ///< Name of the instrument path

  // Variables describing the data in the detector
  size_t m_numberOfTubes = 0;         // number of tubes - X
  size_t m_numberOfPixelsPerTube = 0; // number of pixels per tube - Y
  size_t m_numberOfChannels = 0;      // time channels - Z
  size_t m_numberOfHistograms = 0;

  /* Values parsed from the nexus file */
  double m_wavelength = 0;
  double m_channelWidth = 0;
  double m_timeOfFlightDelay = 0;

  LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADILLTOF2_H_*/
