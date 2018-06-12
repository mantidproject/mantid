#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/*! LoadILLReflectometry : Loads an ILL reflectometry Nexus data file.

 Copyright &copy; 2014-2017 ISIS Rutherford Appleton Laboratory, NScD Oak
 Ridge National Laboratory & European Spallation Source

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
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }
  /// Algorithm's category for search and find. @see Algorithm::category
  const std::string category() const override {
    return "DataHandling\\Nexus;ILL\\Reflectometry";
  }
  /// Algorithm's summary. @see Algorithm::summary
  const std::string summary() const override {
    return "Loads an ILL reflectometry Nexus file (instrument D17 or "
           "Figaro).";
  }

private:
  /// ID tags for supported instruments.
  enum class Supported { D17, Figaro };

  void init() override;
  void exec() override;

  void initWorkspace(const std::vector<std::vector<int>> &monitorsData);
  void initNames(NeXus::NXEntry &entry);
  void initPixelWidth();
  void loadDataDetails(NeXus::NXEntry &entry);
  double doubleFromRun(const std::string &entryName) const;
  std::vector<double> getXValues();
  void convertTofToWavelength();
  double reflectometryPeak();
  void loadData(NeXus::NXEntry &entry,
                const std::vector<std::vector<int>> &monitorsData,
                const std::vector<double> &xVals);
  void loadNexusEntriesIntoProperties();
  std::vector<int> loadSingleMonitor(NeXus::NXEntry &entry,
                                     const std::string &monitor_data);
  std::vector<std::vector<int>> loadMonitors(NeXus::NXEntry &entry);
  void loadInstrument();
  double peakOffsetAngle();
  double detectorRotation();
  void placeDetector();
  void placeSlits();
  void placeSource();
  double collimationAngle() const;
  double detectorAngle() const;
  double offsetAngle(const double peakCentre, const double detectorCentre,
                     const double detectorDistance) const;
  double sampleDetectorDistance() const;
  double sampleHorizontalOffset() const;
  double sourceSampleDistance() const;
  API::MatrixWorkspace_sptr m_localWorkspace;

  Supported m_instrument{Supported::D17}; ///< Name of the instrument
  size_t m_acqMode{1}; ///< Acquisition mode (1 TOF (default), 0 monochromatic)
  size_t m_numberOfChannels{0};
  double m_tofDelay{0.0};
  size_t m_numberOfHistograms{0};
  double m_channelWidth{0.0};
  std::string m_detectorAngleName;
  std::string m_sampleAngleName;
  std::string m_offsetName;
  std::string m_offsetFrom;
  std::string m_chopper1Name;
  std::string m_chopper2Name;
  double m_detectorAngle{0.0};
  double m_detectorDistance{0.0};
  const static double PIXEL_CENTER;
  double m_pixelWidth{0.0};
  double m_sampleZOffset{0.0};
  Mantid::DataHandling::LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_ */
