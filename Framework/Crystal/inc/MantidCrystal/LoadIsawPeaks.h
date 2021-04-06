// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/FileDescriptor.h"

namespace Mantid {
namespace Crystal {

/**
 * Load an ISAW-style .peaks or .integrate file
 * into a PeaksWorkspace
 *
 * @author Janik Zikovsky, SNS
 * @date 2011-03-07 15:22:11.897153
 */
class MANTID_CRYSTAL_DLL LoadIsawPeaks : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadIsawPeaks"; }

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Load an ISAW-style .peaks file into a PeaksWorkspace."; }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"SaveIsawPeaks"}; }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\DataHandling;DataHandling\\Isaw"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Flag for reading modulated structures
  bool m_isModulatedStructure;
  std::vector<double> m_offset1 = {0.0, 0.0, 0.0};
  std::vector<double> m_offset2 = {0.0, 0.0, 0.0};
  std::vector<double> m_offset3 = {0.0, 0.0, 0.0};

  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;

  /// Reads first line of peaks file and returns first word of next line
  std::string readHeader(const Mantid::DataObjects::PeaksWorkspace_sptr &outWS, std::ifstream &in, double &T0);

  /// Read a single peak from peaks file
  DataObjects::Peak readPeak(const DataObjects::PeaksWorkspace_sptr &outWS, std::string &lastStr, std::ifstream &in,
                             int &seqNum, std::string bankName, double qSign);

  int findPixelID(const Geometry::Instrument_const_sptr &inst, const std::string &bankName, int col, int row);

  /// Read the header of a peak block section, returns first word of next line
  std::string readPeakBlockHeader(std::string lastStr, std::ifstream &in, int &run, int &detName, double &chi,
                                  double &phi, double &omega, double &monCount);

  /// Append peaks from given file to given workspace
  void appendFile(const Mantid::DataObjects::PeaksWorkspace_sptr &outWS, const std::string &filename);

  /// Compare number of peaks in given file to given workspace
  /// Throws std::length_error on mismatch
  void checkNumberPeaks(const Mantid::DataObjects::PeaksWorkspace_sptr &outWS, const std::string &filename);

  /// Local cache of bank IComponents used in file
  std::map<std::string, std::shared_ptr<const Geometry::IComponent>> m_banks;

  /// Retrieve cached bank (or load and cache for next time)
  std::shared_ptr<const Geometry::IComponent>
  getCachedBankByName(const std::string &bankname, const std::shared_ptr<const Geometry::Instrument> &inst);
};

} // namespace Crystal
} // namespace Mantid
