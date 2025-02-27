// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace HistogramData {
class BinEdges;
class CountStandardDeviations;
class Counts;
} // namespace HistogramData
} // namespace Mantid

namespace Mantid {
namespace Algorithms {
/** Normalizes a 2D workspace by a specified monitor spectrum. By default ,the
    normalization is done bin-by-bin following this formula:
    Norm(s_i)=(s_i/m_i)*Dlam_i*Sum(m_i)/Sum(Dlam_i)
    where s_i is the signal in bin i, m_i the count in the corresponding monitor
   bin,
    Dlam_i the width of the bin, Sum(m_i) is the integrated monitor count and
    Sum(Dlam_i) the sum of all bin widths (the full range).

    Optionally, can instead normalize by the integrated monitor count over a
   specified
    range in X. In this case all bins in all spectra will simply be divided by
   this
    integrated count. No bin width correction takes place in this case.

    The monitor spectrum can be provided either as an index in the main input
   workspace
    or as a separate single-spectrum workspace.

    Required Properties:
    <UL>
    <LI> InputWorkspace   - The name of the input workspace. Must be a histogram
                            and not a distribution.</LI>
    <LI> OutputWorkspace  - The name of the output workspace. </LI>
    <LI> MonitorSpectrum  - The spectrum number for the monitor to normalize
   with. </LI>
    <LI> MonitorWorkspace - A workspace containing the monitor spectrum. </LI>
    </UL>

    Optional Properties:
    These should be set to normalize by an integrated monitor count over the
   range given
    <UL>
    <LI> IntegrationRangeMin - The lower bound of the range to use. </LI>
    <LI> IntegrationRangeMax - The upper bound of the range to use. </LI>
    <LI> IncludePartialBins  - Scales counts in end bins if min/max not on bin
   boundary. </LI>
    </UL>
*/
class MANTID_ALGORITHMS_DLL NormaliseToMonitor final : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "NormaliseToMonitor"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Normalizes a 2D workspace by a specified spectrum, spectrum, "
           "described by a monitor ID or spectrun provided in a separate "
           "worskspace. ";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Divide"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "CorrectionFunctions\\NormalisationCorrections"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

protected: // for testing
  void checkProperties(const API::MatrixWorkspace_sptr &inputWorkspace);
  API::MatrixWorkspace_sptr getInWSMonitorSpectrum(const API::MatrixWorkspace_sptr &inputWorkspace);
  size_t getInWSMonitorIndex(const API::MatrixWorkspace_sptr &inputWorkspace);
  API::MatrixWorkspace_sptr getMonitorWorkspace(const API::MatrixWorkspace_sptr &inputWorkspace);
  API::MatrixWorkspace_sptr extractMonitorSpectra(const API::MatrixWorkspace_sptr &ws,
                                                  const std::vector<size_t> &workspaceIndexes);
  bool setIntegrationProps(const bool isSingleCountWorkspace);

  void normaliseByIntegratedCount(const API::MatrixWorkspace_sptr &inputWorkspace,
                                  API::MatrixWorkspace_sptr &outputWorkspace, const bool isSingleCountWorkspace);

  void performHistogramDivision(const API::MatrixWorkspace_sptr &inputWorkspace,
                                API::MatrixWorkspace_sptr &outputWorkspace);

  void normaliseBinByBin(const API::MatrixWorkspace_sptr &inputWorkspace, API::MatrixWorkspace_sptr &outputWorkspace);
  void normalisationFactor(const HistogramData::BinEdges &X, HistogramData::Counts &Y,
                           HistogramData::CountStandardDeviations &E);

private:
  /// A single spectrum workspace containing the monitor
  API::MatrixWorkspace_sptr m_monitor;
  /// Whether the input workspace has common bins
  bool m_commonBins = false;
  /// The lower bound of the integration range
  double m_integrationMin = EMPTY_DBL();
  /// The upper bound of the integration range
  double m_integrationMax = EMPTY_DBL();
  bool m_scanInput = false;
  std::vector<size_t> m_workspaceIndexes;
};

// the internal class to verify and modify interconnected properties affecting
// the different ways to normalize ws by this ws spectrum.
class MANTID_ALGORITHMS_DLL MonIDPropChanger : public Kernel::IPropertySettings {

public:
  //   properties this property depends on:
  //   "InputWorkspace","MonitorSpectrum","MonitorWorkspace"
  MonIDPropChanger(const std::string &WSProperty, const std::string &SpectrToNormByProperty,
                   const std::string &MonitorWorkspace)
      : hostWSname(WSProperty), SpectraNum(SpectrToNormByProperty), MonitorWorkspaceProp(MonitorWorkspace),
        is_enabled(true) {}
  // if input to this property is enabled
  bool isEnabled(const Mantid::Kernel::IPropertyManager *algo) const override;
  bool isConditionChanged(const Mantid::Kernel::IPropertyManager *algo,
                          const std::string &changedPropName = "") const override;
  void applyChanges(const Mantid::Kernel::IPropertyManager *algo, Kernel::Property *const pProp) override;

  // interface needs it but if indeed proper clone used -- do not know.
  IPropertySettings *clone() const override {
    return new MonIDPropChanger(hostWSname, SpectraNum, MonitorWorkspaceProp);
  }

private:
  // the name of the property, which specifies the workspace which has to be
  // modified
  std::string hostWSname;
  // the name of the property, which specifies the spectra num (from WS) to
  // normalize by
  std::string SpectraNum;
  // the name of the property, which specifies if you want to allow normalizing
  // by any spectra.
  std::string MonitorWorkspaceProp;

  // the string with existing allowed monitors indexes
  mutable std::vector<int> iExistingAllowedValues;
  // if the monitors id input string is enabled.
  mutable bool is_enabled;
  // auxiliary function to obtain list of monitor's ID-s (allowed_values) from a
  // workspace;
  bool monitorIdReader(const API::MatrixWorkspace_const_sptr &inputWS) const;
};

} // namespace Algorithms
} // namespace Mantid
