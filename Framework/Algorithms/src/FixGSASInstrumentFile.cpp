#include "MantidAlgorithms/FixGSASInstrumentFile.h"

#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"

#include <fstream>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(FixGSASInstrumentFile)

const size_t LINESIZE = 80;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
FixGSASInstrumentFile::FixGSASInstrumentFile() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
FixGSASInstrumentFile::~FixGSASInstrumentFile() {}

//----------------------------------------------------------------------------------------------
/** Implement abstract Algorithm methods
 */
void FixGSASInstrumentFile::init() {
  // Input file
  vector<std::string> exts;
  exts.push_back(".prm");
  exts.push_back(".iparm");
  declareProperty(
      new FileProperty("InputFilename", "", FileProperty::Load, exts),
      "Name of the GSAS instrument parameter file to get fixed for format. ");

  // Output file
  declareProperty(
      new FileProperty("OutputFilename", "", FileProperty::Save, exts),
      "Name of the output GSAS instrument parameter file to have format "
      "fixed. ");

  return;
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
  string line;
  while (getline(infile, line)) {
    // Split "\n"
    vector<string> fields;
    boost::algorithm::split(fields, line, boost::algorithm::is_any_of("\n"));
    if (fields.size() == 0)
      throw runtime_error("Impossible to have an empty line. ");
    vec_line.push_back(fields[0]);
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

  for (size_t i = 0; i < vec_line.size(); ++i) {
    string &line = vec_line[i];
    ofile << line;
    for (size_t j = line.size(); j < LINESIZE; ++j)
      ofile << " ";
    ofile << "\n";
  }

  ofile.close();

  return;
}

} // namespace Algorithms
} // namespace Mantid
