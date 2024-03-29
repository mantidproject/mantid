// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/cow_ptr.h"

#include <fstream>

namespace Mantid {
namespace DataHandling {
/**
   Loads an RKH file into a Mantid 1D workspace

   Required properties:
   <UL>
   <LI> OutputWorkspace - The name output workspace.</LI>
   <LI> Filename - The path to the file in RKH format</LI>
   <LI> FirstColumnValue - The units of the first column in the file</LI>
   </UL>

   @author Martyn Gigg, Tessella Support Services plc
   @date 19/01/2009
*/
class MANTID_DATAHANDLING_DLL LoadRKH : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadRKH"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Load a file written in the RKH format"; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SaveRKH"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Text;SANS\\DataHandling"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Store the units known to the UnitFactory
  std::unordered_set<std::string> m_unitKeys;
  /// Store the units added as options for this algorithm
  std::unordered_set<std::string> m_RKHKeys;
  /// the input stream for the file being loaded
  std::ifstream m_fileIn;

  // Initialisation code
  void init() override;
  // Execution code
  void exec() override;

  bool is2D(const std::string &testLine);
  const API::MatrixWorkspace_sptr read1D();
  const API::MatrixWorkspace_sptr read2D(const std::string &firstLine);
  API::Progress read2DHeader(const std::string &initalLine, API::MatrixWorkspace_sptr &outWrksp, MantidVec &axis0Data);
  const std::string readUnit(const std::string &line);
  void readNumEntrys(const int nEntries, MantidVec &output);
  void binCenter(const MantidVec &oldBoundaries, MantidVec &toCenter) const;

  // Remove lines from an input stream
  void skipLines(std::istream &strm, int nlines);

  /// Check if we the data set stores an X-Error values
  bool hasXerror(std::ifstream &stream);

  /// Read data from the RKH file
  void readLinesForRKH1D(std::istream &stream, int readStart, int readEnd, HistogramData::Points &x,
                         HistogramData::Counts &y, HistogramData::CountStandardDeviations &ye,
                         HistogramData::PointStandardDeviations &xe, API::Progress &prog, bool readXError = false);
};
} // namespace DataHandling
} // namespace Mantid
