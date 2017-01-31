#ifndef MANTID_DATAHANDLING_LOADILLTOF_H_
#define MANTID_DATAHANDLING_LOADILLTOF_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/Progress.h"

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
class DLLExport LoadILLTOF : public API::IFileLoader<Kernel::NexusDescriptor>,
                             public API::DeprecatedAlgorithm {
public:
  /// Constructor
  LoadILLTOF();
  /// Algorithm's name
  const std::string name() const override { return "LoadILLTOF"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads an ILL ToF NeXus file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  // Initialisation code
  void init() override;
  // Execution code
  void exec() override;

  int getEPPFromVanadium(const std::string &,
                         Mantid::API::MatrixWorkspace_sptr);
  void loadInstrumentDetails(NeXus::NXEntry &);
  std::vector<std::vector<int>> getMonitorInfo(NeXus::NXEntry &firstEntry);
  void initWorkSpace(NeXus::NXEntry &entry,
                     const std::vector<std::vector<int>> &);
  void initInstrumentSpecific();
  void addAllNexusFieldsAsProperties(std::string filename);
  void addEnergyToRun();
  void addPulseInterval();

  int getDetectorElasticPeakPosition(const NeXus::NXInt &data);
  void loadTimeDetails(NeXus::NXEntry &entry);
  void
  loadDataIntoTheWorkSpace(NeXus::NXEntry &entry,
                           const std::vector<std::vector<int>> &,
                           int vanaCalculatedDetectorElasticPeakPosition = -1);
  void loadSpectra(size_t &spec, size_t numberOfMonitors, size_t numberOfTubes,
                   std::vector<Mantid::detid_t> &detectorIDs, NeXus::NXInt data,
                   Mantid::API::Progress progress);

  void runLoadInstrument();

  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }
  int validateVanadium(const std::string &);

  API::MatrixWorkspace_sptr m_localWorkspace;

  //	NeXus::NXRoot m_dataRoot;
  //	NeXus::NXRoot m_vanaRoot;

  std::string m_instrumentName; ///< Name of the instrument
  std::string m_instrumentPath; ///< Name of the instrument path

  // Variables describing the data in the detector
  size_t m_numberOfTubes;         // number of tubes - X
  size_t m_numberOfPixelsPerTube; // number of pixels per tube - Y
  size_t m_numberOfChannels;      // time channels - Z
  size_t m_numberOfHistograms;

  /* Values parsed from the nexus file */
  int m_monitorElasticPeakPosition;
  double m_wavelength;
  double m_channelWidth;

  double m_l1; //=2.0;
  double m_l2; //=4.0;

  std::vector<std::string> m_supportedInstruments;
  LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADILLTOF_H_*/
