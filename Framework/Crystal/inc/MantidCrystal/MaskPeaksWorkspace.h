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
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

namespace Mantid {
namespace Crystal {
/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS, ORNL
 @date 02/08/2011
 */
class MANTID_CRYSTAL_DLL MaskPeaksWorkspace : public API::Algorithm {
public:
  /// Default constructor
  MaskPeaksWorkspace();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MaskPeaksWorkspace"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CreatePeaksWorkspace"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Crystal\\Peaks"; }

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Masks a peaks workspace."; }

private:
  API::MatrixWorkspace_sptr m_inputW; ///< A pointer to the input workspace

  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  std::size_t getWkspIndex(const detid2index_map &pixel_to_wi, const Geometry::IComponent_const_sptr &comp, const int x,
                           const int y);
  void getTofRange(double &tofMin, double &tofMax, const double tofPeak, const HistogramData::HistogramX &tof);
  int findPixelID(const std::string &bankName, int col, int row);

  /// Read in all the input parameters
  void retrieveProperties();
  int m_xMin;      ///< The start of the X range for fitting
  int m_xMax;      ///< The end of the X range for fitting
  int m_yMin;      ///< The start of the Y range for fitting
  int m_yMax;      ///< The end of the Y range for fitting
  double m_tofMin; ///< The start of the box around the peak in tof
  double m_tofMax; ///< The end of the box around the peak in tof
};

} // namespace Crystal
} // namespace Mantid
