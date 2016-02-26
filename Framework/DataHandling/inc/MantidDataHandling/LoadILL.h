#ifndef MANTID_DATAHANDLING_LOADILL_H_
#define MANTID_DATAHANDLING_LOADILL_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidDataHandling/LoadHelper.h"

namespace Mantid {
namespace DataHandling {
/**
 Loads an ILL nexus file into a Mantid workspace.

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadILL : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Constructor
  LoadILL(); /// Virtual destructor
  ~LoadILL() override {}
  /// Algorithm's name
  const std::string name() const override { return "LoadILL"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a ILL nexus file.";
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

  int getEPPFromVanadium(const std::string & /*filenameVanadium*/,
                         Mantid::API::MatrixWorkspace_sptr /*vanaWS*/);
  void loadInstrumentDetails(NeXus::NXEntry & /*firstEntry*/);
  std::vector<std::vector<int>> getMonitorInfo(NeXus::NXEntry &firstEntry);
  void initWorkSpace(NeXus::NXEntry &entry,
                     const std::vector<std::vector<int>> & /*monitors*/);
  void initInstrumentSpecific();
  void addAllNexusFieldsAsProperties(std::string filename);
  void addEnergyToRun();

  int getDetectorElasticPeakPosition(const NeXus::NXInt &data);
  void loadTimeDetails(NeXus::NXEntry &entry);
  NeXus::NXData loadNexusFileData(NeXus::NXEntry &entry);
  void
  loadDataIntoTheWorkSpace(NeXus::NXEntry &entry,
                           const std::vector<std::vector<int>> & /*monitors*/,
                           int vanaCalculatedDetectorElasticPeakPosition = -1);

  void runLoadInstrument();

  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }
  int validateVanadium(const std::string & /*filenameVanadium*/);

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

#endif /*MANTID_DATAHANDLING_LoadILL_H_*/
