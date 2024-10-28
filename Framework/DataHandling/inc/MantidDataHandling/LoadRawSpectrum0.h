// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include <climits>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ISISRAW2;

namespace Mantid {
namespace DataHandling {
/**

Loads zeroth spectrum from ISIS RAW format file and stores it in a 2D workspace
(Workspace2D class). LoadRawSpectrum0 is an algorithm and  inherits
from the LoadRawHelper class .

Required Properties:
<UL>
<LI> Filename - The name of and path to the input RAW file </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the imported
data
     (a multiperiod file will store higher periods in workspaces called
OutputWorkspace_PeriodNo)</LI>
</UL>


@author Sofia Antony,ISIS,RAL
@date 12/04/2010
*/
class MANTID_DATAHANDLING_DLL LoadRawSpectrum0 : public LoadRawHelper {
public:
  /// Default constructor
  LoadRawSpectrum0();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadRawSpectrum0"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads spectrum zero  from  ISIS  raw file and stores it in a 2D "
           "workspace (Workspace2D class).";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadRawBin0", "LoadRaw"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diagnostics\\Raw;DataHandling\\Raw"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  /// ISISRAW class instance which does raw file reading. Shared pointer to
  /// prevent memory leak when an exception is thrown.
  std::shared_ptr<ISISRAW2> isisRaw;
  /// The name and path of the input file
  std::string m_filename;

  /// The number of spectra in the raw file
  specnum_t m_numberOfSpectra;

  /// Allowed values for the cache property
  std::vector<std::string> m_cache_options;
  /// A map for storing the time regime for each spectrum
  std::map<int64_t, int64_t> m_specTimeRegimes;
  /// The current value of the progress counter
  double m_prog;
  /// Read in the time bin boundaries
  int64_t m_lengthIn;
  /// number of time regime
  int64_t m_noTimeRegimes;
};
} // namespace DataHandling
} // namespace Mantid
