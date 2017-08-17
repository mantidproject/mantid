#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"

namespace Mantid {
namespace DataHandling {

/*! LoadILLReflectometry : Loads an ILL reflectometry Nexus data file.

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
  const std::string name() const override { return "LoadILLReflectometry"; }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1; }
  /// Algorithm's category for search and find. @see Algorithm::category
  const std::string category() const override { return "DataHandling\\Nexus"; }
  /// Algorithm's summary. @see Algorithm::summary
  const std::string summary() const override {
    return "Loads an ILL reflectometry Nexus file (instrument D17 or "
           "Figaro).";
  }
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;
  /// Return the detector position of the peak
  std::vector<double>
  fitReflectometryPeak(const std::string &beam = "ReflectedBeam",
                       const std::string &angleDirectBeam = "");

private:
  void init() override;
  void exec() override;

  void initWorkspace(const std::vector<std::vector<int>> &monitorsData);
  void initNames(NeXus::NXEntry &entry);
  void loadDataDetails(NeXus::NXEntry &entry);
  double doubleFromRun(const std::string& entryName);
  std::vector<double> getXValues();
  void convertTofToWavelength();
  void loadData(NeXus::NXEntry &entry,
                const std::vector<std::vector<int> > &monitorsData,
                const std::vector<double> &xVals);
  void loadNexusEntriesIntoProperties();
  std::vector<int> loadSingleMonitor(NeXus::NXEntry &entry,
                                     const std::string &monitor_data);
  std::vector<std::vector<int>> loadMonitors(NeXus::NXEntry &entry);
  void loadInstrument();
  void loadBeam(API::MatrixWorkspace_sptr &beamWS,
                const std::string &beam,
                std::string angleDirectBeam);
  double computeBraggAngle();
  void placeDetector();
  void placeSource();

  API::MatrixWorkspace_sptr m_localWorkspace;

  /* Values parsed from the nexus file */
  std::string m_instrumentName; ///< Name of the instrument
  size_t m_acqMode{1}; ///< Acquisition mode (1 TOF (default), 0 monochromatic)
  size_t m_numberOfChannels{0};
  double m_tofDelay{0.0};
  size_t m_numberOfHistograms{
      0}; // number of tubes (always 1) times number of pixels per tube
  double m_wavelength{0.0};
  double m_channelWidth{0.0};
  double m_angleDirectBeam{0.0}; /// detector angle of the direct beam
  double m_offsetAngle{0.0};
  std::string m_detectorDistance{std::string()};
  std::string m_detectorAngleName{std::string()};
  std::string m_sampleAngleName{std::string()};
  std::string m_offsetName{std::string()};
  std::string m_offsetFrom{std::string()};
  std::string m_chopper1Name{std::string()};
  std::string m_chopper2Name{std::string()};
  double m_detectorDistanceDirectBeam{0.0}; ///< Sample detector distance
  double m_detectorDistanceValue{0.0};
  double m_pixelCentre{0.0};
  double m_pixelWidth{0.0};
  std::unordered_set<std::string> m_supportedInstruments{"D17", "d17", "Figaro",
                                                         "figaro"};
  Mantid::DataHandling::LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_ */
