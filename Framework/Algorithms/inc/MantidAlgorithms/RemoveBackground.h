// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {

namespace HistogramData {
class HistogramX;
class HistogramY;
class HistogramE;
} // namespace HistogramData
// forward declarations from other Mantid modules
namespace Kernel {
class Unit;
}

namespace API {
class SpectrumInfo;
}

namespace Algorithms {
/** Performs removal of constant (and possibly non-constant after simple
modification) background calculated in TOF units
from a matrix workspace, expressed in units, different from TOF.

@date 26/10/2014
*/

/**Class actually performing background removal from a workspace spectra */
class MANTID_ALGORITHMS_DLL BackgroundHelper {
public:
  BackgroundHelper();

  // MSVC 2017 does not properly detect that this type contains a move-only
  // type, so we force delete the copy constructor and copy assignment here.
  BackgroundHelper &operator=(const BackgroundHelper &) = delete;
  BackgroundHelper(const BackgroundHelper &) = delete;

  void initialize(const API::MatrixWorkspace_const_sptr &bkgWS, const API::MatrixWorkspace_sptr &sourceWS,
                  Kernel::DeltaEMode::Type emode, Kernel::Logger *pLog = nullptr, int nThreads = 1, bool inPlace = true,
                  bool nullifyNegative = false);
  void removeBackground(int nHist, HistogramData::HistogramX &x_data, HistogramData::HistogramY &y_data,
                        HistogramData::HistogramE &e_data, int threadNum = 0) const;

private:
  // vector of pointers to the units conversion class for the working workspace;
  std::vector<std::unique_ptr<Kernel::Unit>> m_WSUnit;

  // shared pointer to the workspace containing background
  API::MatrixWorkspace_const_sptr m_bgWs;
  // shared pointer to the workspace where background should be removed
  API::MatrixWorkspace_const_sptr m_wkWS;

  const API::SpectrumInfo *m_spectrumInfo;

  // logger from the hosting algorithm
  Kernel::Logger *m_pgLog;
  // perform background removal in-place
  bool m_inPlace;

  // if the background workspace is single value workspace
  bool m_singleValueBackground;
  // average number of counts at background for first spectra of a background
  // workspace
  double m_NBg;
  // time interval for measuring the background
  double m_dtBg;
  // Squared error of the background for first spectra of the background
  // workspace
  double m_ErrSq;
  // energy conversion mode
  Kernel::DeltaEMode::Type m_Emode;
  // if true, negative signals are nullified
  bool m_nullifyNegative;
  // removing negative values from ws with background removed previously.
  bool m_previouslyRemovedBkgMode;
};

class MANTID_ALGORITHMS_DLL RemoveBackground final : public API::Algorithm {
public:
  RemoveBackground() {}
  RemoveBackground(const RemoveBackground &) = delete;
  RemoveBackground &operator=(const RemoveBackground &) = delete;

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RemoveBackground"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Removes background (constant for now) calculated in TOF units "
           "from a matrix workspace, expressed in units, different from TOF";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "CorrectionFunctions\\BackgroundCorrections"; }

protected:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

private:
  // class responsible for background removal
  BackgroundHelper m_BackgroundHelper;
};

} // namespace Algorithms
} // namespace Mantid
