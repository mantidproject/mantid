// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FixGSASInstrumentFile.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"

#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(FixGSASInstrumentFile)

const size_t LINESIZE = 80;

//----------------------------------------------------------------------------------------------
/** Implement abstract Algorithm methods
 */
void FixGSASInstrumentFile::init() {

  std::initializer_list<std::string> exts = {".prm", ".iparm"};

  // Input file
  declareProperty(
      std::make_unique<FileProperty>("InputFilename", "", FileProperty::Load,
                                     exts),
      "Name of the GSAS instrument parameter file to get fixed for format. ");

  // Output file
  declareProperty(
      std::make_unique<FileProperty>("OutputFilename", "", FileProperty::Save,
                                     exts),
      "Name of the output GSAS instrument parameter file to have format "
      "fixed. ");
}

//----------------------------------------------------------------------------------------------
/** Execution
 */
void FixGSASInstrumentFile::exec() {
  // Properties
  string infilename = getProperty("InputFilename");

  // Parse file
  vector<string> vec_line;
  ifstream infile;
  infile.open(infilename.c_str(), ios::in);
  if (!infile.is_open()) {
    stringstream errss;
    errss << "File " << infilename << " cannot be opened for reading. "
          << ".\n";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }
  {
    string line;
    while (getline(infile, line)) {
      // Split "\n"
      vector<string> fields;
      boost::algorithm::split(fields, line, boost::algorithm::is_any_of("\n"));
      if (fields.empty())
        throw runtime_error("Impossible to have an empty line. ");
      vec_line.push_back(fields[0]);
    }
  }
  infile.close();

  // Write out
  string outfilename = getPropertyValue("OutputFilename");
  ofstream ofile;
  ofile.open(outfilename.c_str(), ios::out);
  if (!ofile.is_open()) {
    stringstream errss;
    errss << "File " << outfilename << " cannot be opened for writing. "
          << ".\n";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  for (const auto &line : vec_line) {
    ofile << line;
    for (size_t j = line.size(); j < LINESIZE; ++j)
      ofile << " ";
    ofile << "\n";
  }

  ofile.close();
}

} // namespace Algorithms
} // namespace Mantid
