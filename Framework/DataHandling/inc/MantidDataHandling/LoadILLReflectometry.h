// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses_fwd.h"
#include "MantidTypes/Core/DateAndTime.h"

namespace Mantid {
namespace DataHandling {

/*! LoadILLReflectometry : Loads an ILL reflectometry Nexus data file.
 */
class MANTID_DATAHANDLING_DLL LoadILLReflectometry : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLReflectometry() = default;
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string name() const override { return "LoadILLReflectometry"; }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }
  /// Algorithm's category for search and find. @see Algorithm::category
  const std::string category() const override { return "DataHandling\\Nexus;ILL\\Reflectometry"; }
  /// Algorithm's summary. @see Algorithm::summary
  const std::string summary() const override {
    return "Loads an ILL reflectometry Nexus file (instrument D17 or "
           "FIGARO).";
  }
  double doubleFromRun(const std::string &entryName) const;

private:
  /// ID tags for supported instruments.
  enum class Supported { D17, FIGARO };
  void init() override;
  void exec() override;
  double sampleDetectorDistance() const;
  double sourceSampleDistance() const;
  void sampleHorizontalOffset();
  void sampleAngle(const NeXus::NXEntry &entry);
  void initWorkspace(const std::vector<std::string> &monitorNames);
  void initNames(const NeXus::NXEntry &entry);
  void initPixelWidth();
  void loadDataDetails(const NeXus::NXEntry &entry);
  std::vector<double> getXValues();
  void convertTofToWavelength();
  double reflectometryPeak();
  void loadData(const NeXus::NXEntry &entry, const std::vector<std::string> &monitorNames,
                const std::vector<double> &xVals);
  void loadNexusEntriesIntoProperties();
  std::vector<int> loadSingleMonitor(const NeXus::NXEntry &entry, const std::string &monitor_data);
  std::vector<std::string> getMonitorNames();
  double peakOffsetAngle();
  void addSampleLogs();
  double detectorRotation();
  void placeDetector();
  void placeSlits();
  void placeSource();
  double collimationAngle() const;
  double offsetAngle(const double peakCentre, const double detectorCentre, const double detectorDistance) const;
  Supported m_instrument{Supported::D17};
  size_t m_acqMode{1}; ///(1: TOF (default), 0: monochromatic)
  size_t m_numberOfChannels{0};
  size_t m_numberOfHistograms{0};
  std::string m_sampleAngleName;
  std::string m_offsetFrom;
  std::string m_chopper1Name;
  std::string m_chopper2Name;
  double m_tofDelay{0.0};
  double m_channelWidth{0.0};
  double m_detectorDistance{0.0};
  double m_pixelWidth{0.0};
  double m_sampleZOffset{0.0};
  double m_sourceDistance{0.0};
  double m_sampleAngle{0.0};
  Mantid::Types::Core::DateAndTime m_startTime;
  API::MatrixWorkspace_sptr m_localWorkspace;
};

} // namespace DataHandling
} // namespace Mantid
