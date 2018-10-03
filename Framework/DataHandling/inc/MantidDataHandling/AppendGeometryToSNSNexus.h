// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUS_H_
#define MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** AppendGeometryToSNSNexus : Appends geometry information to a NeXus file.

  @date 2012-06-01
*/
class DLLExport AppendGeometryToSNSNexus : public API::Algorithm {
public:
  AppendGeometryToSNSNexus();
  ~AppendGeometryToSNSNexus() override;

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Appends the resolved instrument geometry (detectors and monitors "
           "for now) to a SNS ADARA NeXus file.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  /// The filename of the NeXus file to append geometry info to
  std::string m_filename;

  /// Instrument name
  std::string m_instrument;

  /// IDF filename
  std::string m_idf_filename;

  /// Get the instrument name from the NeXus file
  std::string getInstrumentName(const std::string &nxfilename);

  /// Run LoadInstrument as a Child Algorithm
  bool runLoadInstrument(const std::string &idf_filename,
                         API::MatrixWorkspace_sptr localWorkspace,
                         Algorithm *alg);

  /// Load logs from the NeXus file
  static bool runLoadNexusLogs(const std::string &nexusFileName,
                               API::MatrixWorkspace_sptr localWorkspace,
                               Algorithm *alg);

  /// Are we going to make a copy of the NeXus file to operate on ?
  bool m_makeNexusCopy;

  /// The workspace to load instrument and logs
  API::MatrixWorkspace_sptr ws;

  /// Was the instrument loaded?
  bool m_instrumentLoadedCorrectly;

  /// Were the logs loaded?
  bool m_logsLoadedCorrectly;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUS_H_ */
