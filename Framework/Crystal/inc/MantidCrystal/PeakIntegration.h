// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Crystal {
/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS, ORNL
 @date 02/08/2011
 */
class MANTID_CRYSTAL_DLL PeakIntegration : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PeakIntegration"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"IntegratePeakTimeSlices", "CentroidPeaks"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Crystal\\Integration"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Integrate single crystal peaks using IkedaCarpenter fit TOF"; }

private:
  API::MatrixWorkspace_sptr inputW;  ///< A pointer to the input workspace
  API::MatrixWorkspace_sptr outputW; ///< A pointer to the output workspace
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  /// Call Gaussian as a Child Algorithm to fit the peak in a spectrum
  void fitSpectra(const int s, double TOFPeakd, double &I, double &sigI);
  /// Read in all the input parameters
  void retrieveProperties();
  int fitneighbours(int ipeak, const std::string &det_name, int x0, int y0, int idet, double qspan,
                    DataObjects::PeaksWorkspace_sptr &Peaks, const detid2index_map &pixel_to_wi);

  bool m_IC = false; ///< Ikeida Carpenter fit of TOF
};

} // namespace Crystal
} // namespace Mantid
