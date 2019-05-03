// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/*! LoadILLReflectometry : Loads an ILL reflectometry Nexus data file.
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
           "FIGARO).";
  }
  double doubleFromRun(const std::string &entryName) const;
  double sampleDetectorDistance() const;
  double sampleHorizontalOffset() const;
  double sourceSampleDistance() const;

private:
  /// ID tags for supported instruments.
  enum class Supported { D17, FIGARO };

  void init() override;
  void exec() override;

  void initWorkspace(const std::vector<std::vector<int>> &monitorsData);
  void initNames(NeXus::NXEntry &entry);
  void initPixelWidth();
  void loadDataDetails(NeXus::NXEntry &entry);
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
  void addSampleLogs();
  double detectorRotation();
  void placeDetector();
  void placeSlits();
  void placeSource();
  double collimationAngle() const;
  double detectorAngle() const;
  double offsetAngle(const double peakCentre, const double detectorCentre,
                     const double detectorDistance) const;
  API::MatrixWorkspace_sptr m_localWorkspace;

  Supported m_instrument{Supported::D17}; ///< Name of the instrument
  size_t m_acqMode{1}; ///< Acquisition mode (1 TOF (default), 0 monochromatic)
  size_t m_numberOfChannels{0};
  double m_tofDelay{0.0};
  size_t m_numberOfHistograms{0};
  double m_channelWidth{0.0};
  std::string m_detectorAngleName;
  std::string m_sampleAngleName;
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
