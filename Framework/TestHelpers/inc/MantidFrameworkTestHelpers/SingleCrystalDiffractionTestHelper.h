// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/NearestNeighbours.h"

#include <random>
#include <tuple>

namespace Mantid {
namespace SingleCrystalDiffractionTestHelper {

class WorkspaceBuilder {

public:
  WorkspaceBuilder() : m_numPixels(0), m_totalNPixels(0), m_outputAsHistogram(false), m_generator(std::mt19937()()) {};

  /// Set the total number of peaks to use
  void setNumPixels(const int numPixels);
  /// Set whether to create an event workspace or a histogram workspace
  void outputAsHistogram(const bool outputAsHistogram) { m_outputAsHistogram = outputAsHistogram; };
  /// Set the rebin parameters to use
  void setRebinParameters(const std::vector<double> &rebinParams) { m_rebinParams = rebinParams; }
  void addBackground(const bool useBackground) { m_useBackground = useBackground; }
  /// Set the parameters for the uniform background
  void setBackgroundParameters(const int nEvents, const double detRange, const double tofRange) {
    m_backgroundParameters = std::make_tuple(nEvents, detRange, tofRange);
  }
  /// Set the random seed for generating events
  void setRandomSeed(const int seed) { m_generator.seed(seed); }
  /// Add a HKL peak to the diffraction dataset
  void addPeakByHKL(const Mantid::Kernel::V3D &hkl, const int numEvents,
                    const std::tuple<double, double, double> &sigmas);
  /// Make a tuple of event workspace and peaks workspace
  std::tuple<Mantid::API::MatrixWorkspace_sptr, Mantid::DataObjects::PeaksWorkspace_sptr> build();

private:
  using HKLPeakDescriptor = std::tuple<Mantid::Kernel::V3D, int, std::tuple<double, double, double>>;

  /// Create a dummy instrument
  void createInstrument();
  /// Create a peaks workspace with the request HKL peaks
  void createPeaksWorkspace();
  /// Create an empty event workspace with the instrument attached
  void createEventWorkspace();
  /// Create a neighbour search tree for finding nearest neighbours
  void createNeighbourSearch();
  /// Create peaks at the requested HKL positions
  void createPeaks();
  /// Create a single HKL peak in the event workspace
  void createPeak(const HKLPeakDescriptor &descriptor);
  /// Create a flat background for the workspace
  void createBackground(const int peakIndex);
  /// Rebin the event workspace to a histogram workspace
  void rebinWorkspace();

  /// Nearest neighbour search tree for detectors
  std::unique_ptr<Mantid::Kernel::NearestNeighbours<3>> m_detectorSearcher;
  /// List of peak descriptors for creating peaks
  std::vector<HKLPeakDescriptor> m_peakDescriptors;
  /// Handle to the instrument object
  Mantid::Geometry::Instrument_sptr m_instrument;
  /// Handle to the final output workspace (event OR histogram)
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  /// Handle to the event workspace
  Mantid::DataObjects::EventWorkspace_sptr m_eventWorkspace;
  /// Handle to the peaks workspace
  Mantid::DataObjects::PeaksWorkspace_sptr m_peaksWorkspace;

  // Instance variables for builder settings

  /// number of pixels along a single axis on the detector bank
  int m_numPixels;
  /// total number of pixels in the detector bank
  int m_totalNPixels;
  /// whether to output event or histogram data
  bool m_outputAsHistogram;
  /// whether to add a background
  bool m_useBackground;
  /// rebin parameters
  std::vector<double> m_rebinParams;
  /// background parameters
  std::tuple<int, double, double> m_backgroundParameters;

  // Other instance varianbles

  /// Random generator for making events
  std::mt19937 m_generator;
};
} // namespace SingleCrystalDiffractionTestHelper
} // namespace Mantid
