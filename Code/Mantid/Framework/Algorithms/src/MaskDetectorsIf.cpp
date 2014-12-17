//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MaskDetectorsIf.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <fstream>
#include <iomanip>
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(MaskDetectorsIf)

using namespace Kernel;

/// Constructor
MaskDetectorsIf::MaskDetectorsIf() : API::Algorithm() {}

/// Destructor
MaskDetectorsIf::~MaskDetectorsIf() {}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void MaskDetectorsIf::init() {
  using namespace Mantid::Kernel;
  declareProperty(
      new API::WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "A 1D Workspace that contains values to select against");
  std::vector<std::string> select_mode(2);
  select_mode[0] = "SelectIf";
  select_mode[1] = "DeselectIf";
  declareProperty(
      "Mode", "SelectIf", boost::make_shared<StringListValidator>(select_mode),
      "Mode to select or deselect detectors based on comparison with values. " +
          allowedValuesStatement(select_mode));
  std::vector<std::string> select_operator(6);
  select_operator[0] = "Equal";
  select_operator[1] = "NotEqual";
  select_operator[2] = "Greater";
  select_operator[3] = "GreaterEqual";
  select_operator[4] = "Less";
  select_operator[5] = "LessEqual";
  declareProperty("Operator", "Equal",
                  boost::make_shared<StringListValidator>(select_operator),
                  "Unary operator to compare to given values. " +
                      allowedValuesStatement(select_operator));
  declareProperty("Value", 0.0);
  declareProperty(
      new API::FileProperty("InputCalFile", "", API::FileProperty::Load,
                            ".cal"),
      "The name of the CalFile with grouping data. Allowed Values: .cal .");
  declareProperty(
      new API::FileProperty("OutputCalFile", "",
                            API::FileProperty::OptionalSave, ".cal"),
      "The name of the CalFile with grouping data. Allowed Values: .cal .");
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 *successfully
 *  @throw std::runtime_error If the rebinning process fails
 */
void MaskDetectorsIf::exec() {
  retrieveProperties();
  const size_t nspec = inputW->getNumberHistograms();

  for (size_t i = 0; i < nspec; ++i) {
    // Get the list of udets contributing to this spectra
    const std::set<detid_t> &dets = inputW->getSpectrum(i)->getDetectorIDs();

    if (dets.empty())
      continue;
    else {
      double val = inputW->readY(i)[0];
      if (compar_f(val, value)) {
        std::set<detid_t>::const_iterator it = dets.begin();
        for (; it != dets.end(); ++it)
          umap.insert(std::make_pair(*it, select_on));
      }
    }
    double p = static_cast<double>(i) / static_cast<double>(nspec);
    progress(p, "Generating detector map");
  }
  std::string oldf = getProperty("InputCalFile");
  std::string newf = getProperty("OutputCalFile");
  progress(0.99, "Creating new cal file");
  createNewCalFile(oldf, newf);
  return;
}

/**
 * Get the input properties and store them in the object variables
 */
void MaskDetectorsIf::retrieveProperties() {
  inputW = getProperty("InputWorkspace");

  //
  value = getProperty("Value");

  // Get the selction mode (select if or deselect if)
  std::string select_mode = getProperty("Mode");
  if (select_mode == "SelectIf")
    select_on = true;
  else
    select_on = false;

  // Select function object based on the type of comparison operator
  std::string select_operator = getProperty("Operator");

  if (select_operator == "LessEqual")
    compar_f = std::less_equal<double>();
  else if (select_operator == "Less")
    compar_f = std::less<double>();
  else if (select_operator == "GreaterEqual")
    compar_f = std::greater_equal<double>();
  else if (select_operator == "Greater")
    compar_f = std::greater<double>();
  else if (select_operator == "Equal")
    compar_f = std::equal_to<double>();
  else if (select_operator == "NotEqual")
    compar_f = std::not_equal_to<double>();

  std::string newf = getProperty("OutputCalFile");
  // MG 2012-10-01: A bug fixed the save file property to be invalid by default
  // which would have moved the argument order. The property is now
  // optional and checked here
  if (newf.empty()) {
    throw std::runtime_error("OutputCalFile is empty. Enter a filename");
  }

  return;
}

/**
 * Create a new cal file based on the old file
 * @param oldfile :: The old cal file path
 * @param newfile :: The new cal file path
 */
void MaskDetectorsIf::createNewCalFile(const std::string &oldfile,
                                       const std::string &newfile) {
  std::ifstream oldf(oldfile.c_str());
  if (!oldf.is_open()) {
    g_log.error() << "Unable to open grouping file " << oldfile << std::endl;
    throw Exception::FileError("Error reading .cal file", oldfile);
  }
  std::ofstream newf(newfile.c_str());
  if (!newf.is_open()) {
    g_log.error() << "Unable to open grouping file " << newfile << std::endl;
    throw Exception::FileError("Error reading .cal file", newfile);
  }
  std::string str;
  while (getline(oldf, str)) {
    // Comment or empty lines get copied into the new cal file
    if (str.empty() || str[0] == '#') {
      newf << str << std::endl;
      continue;
    }
    std::istringstream istr(str);
    int n, udet, sel, group;
    double offset;
    istr >> n >> udet >> offset >> sel >> group;
    udet2valuem::iterator it = umap.find(udet);
    bool selection;

    if (it == umap.end())
      selection = (sel == 0) ? false : true;
    else
      selection = (*it).second;

    newf << std::fixed << std::setw(9) << n << std::fixed << std::setw(15)
         << udet << std::fixed << std::setprecision(7) << std::setw(15)
         << offset << std::fixed << std::setw(8) << selection << std::fixed
         << std::setw(8) << group << std::endl;
  }
  oldf.close();
  newf.close();
  return;
}

std::string
MaskDetectorsIf::allowedValuesStatement(std::vector<std::string> vals) {
  std::ostringstream statement;
  statement << "Allowed Values: ";
  for (size_t i = 0; i < vals.size(); ++i) {
    statement << vals[i];
    if (i < vals.size())
      statement << ", ";
  }
  return statement.str();
}

} // namespace Algorithm
} // namespace Mantid
