#ifndef MANTID_DATAHANDLING_LOADMLZ_H_
#define MANTID_DATAHANDLING_LOADMLZ_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidDataHandling/LoadHelper.h"

namespace Mantid {
namespace DataHandling {
/**
    LoadMLZ : Loads MLZ nexus or hdf file into a Mantid workspace.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
   National Laboratory

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
class DLLExport LoadMLZ : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadMLZ();
  virtual ~LoadMLZ();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads a nexus file from MLZ facility.";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const;

private:
  void init();
  void exec();

  // int getEPPFromVanadium(const std::string
  // &,Mantid::API::MatrixWorkspace_sptr);
  void loadInstrumentDetails(NeXus::NXEntry &);
  void loadTimeDetails(NeXus::NXEntry &entry);

  std::vector<std::vector<int>> getMonitorInfo(NeXus::NXEntry &firstEntry);

  // void initWorkSpace(NeXus::NXEntry& entry, const
  // std::vector<std::vector<int> >&);
  void initWorkSpace(NeXus::NXEntry &entry);
  void initInstrumentSpecific();
  void loadRunDetails(NeXus::NXEntry &entry);
  void loadExperimentDetails(NeXus::NXEntry &entry);
  // int getDetectorElasticPeakPosition(const NeXus::NXInt &data);

  NeXus::NXData loadNexusFileData(NeXus::NXEntry &entry);
  void maskDetectors(NeXus::NXEntry &entry);
  void loadDataIntoTheWorkSpace(
      NeXus::NXEntry &entry); //, int ElasticPeakPosition = -1);

  void runLoadInstrument();

  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }
  // int validateVanadium(const std::string &);

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
  double m_timeOfFlightDelay;
  int m_monitorCounts;
  double m_chopper_speed;
  int m_chopper_ratio;

  double m_l1;
  double m_l2;

  std::vector<std::string> m_supportedInstruments;
  LoadHelper m_mlzloader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADMLZ_H_ */
