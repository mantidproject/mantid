/*
AsciiPointBase is an abstract class holding the functionality for the
SaveILLCosmosAscii and SaveANSTOAscii export-only Acii-based save formats. It is
based on a python script by Maximilian Skoda, written for the ISIS Reflectometry
GUI
*/
#include "MantidDataHandling/AsciiPointBase.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ListValidator.h"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <fstream>

namespace Mantid {
namespace DataHandling {
using namespace Kernel;
using namespace API;

/// Initialisation method.
void AsciiPointBase::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "The name of the workspace containing the data you want to save.");

  declareProperty(Kernel::make_unique<FileProperty>("Filename", "",
                                                    FileProperty::Save, ext()),
                  "The filename of the output file.");

  extraProps();
}

/**
 *   Executes the algorithm. In this case it provides the process for any child
 * classes as this class is abstract
 */
void AsciiPointBase::exec() {
  std::string filename = getProperty("Filename");
  std::ofstream file(filename.c_str());
  if (!file) {
    g_log.error("Unable to create file: " + filename);
    throw Exception::FileError("Unable to create file: ", filename);
  }
  m_ws = getProperty("InputWorkspace");
  g_log.information("FILENAME: " + filename);
  std::string sepOption = getProperty("Separator");
  if (sepOption == "comma") {
    m_sep = ',';
  } else if (sepOption == "space") {
    m_sep = ' ';
  } else {
    m_sep = '\t';
  }
  std::vector<double> XData = header(file);
  extraHeaders(file);
  data(file, XData);
  file.close();
}

/** Adds extra data to the top of the file.
 *  @param file :: pointer to output file stream
 *  @returns std::vector<double> of point data for the X column
 */
std::vector<double> AsciiPointBase::header(std::ofstream &file) {
  auto title = '#' + m_ws->getTitle();
  const auto &xTemp = m_ws->x(0);
  m_xlength = xTemp.size() - 1;
  std::vector<double> XData(m_xlength, 0);
  for (size_t i = 0; i < m_xlength; ++i) {
    XData[i] = (xTemp[i] + xTemp[i + 1]) / 2.0;
  }

  m_qres = (XData[1] - XData[0]) / XData[1];
  g_log.information("Constant dq/q from file: " +
                    boost::lexical_cast<std::string>(m_qres));
  file << std::scientific;
  return XData;
}

/** virtual method to add information to the file before the data
 *  @param file :: pointer to output file stream
 *  @param XData :: pointer to a std::vector<double> containing the point data
 * to be printed
 *  @param exportDeltaQ :: bool on whether deltaQ column to be printed
 */
void AsciiPointBase::data(std::ofstream &file, const std::vector<double> &XData,
                          bool exportDeltaQ) {

  const auto &yData = m_ws->y(0);
  const auto &eData = m_ws->e(0);
  if (exportDeltaQ) {
    for (size_t i = 0; i < m_xlength; ++i) {
      double dq = XData[i] * m_qres;
      outputval(XData[i], file, leadingSep());
      outputval(yData[i], file);
      outputval(eData[i], file);
      outputval(dq, file);
      file << '\n';
    }
  } else {
    for (size_t i = 0; i < m_xlength; ++i) {
      outputval(XData[i], file, leadingSep());
      outputval(yData[i], file);
      outputval(eData[i], file);
      file << '\n';
    }
  }
}

/** writes a properly formatted line of data
 *  @param val :: the double value to be written
 *  @param file :: pointer to output file stream
 *  @param leadingSep :: boolean to determine if there should be a separator
 * before this value, default true
 */
void AsciiPointBase::outputval(double val, std::ofstream &file,
                               bool leadingSep) {
  bool nancheck = std::isnan(val);
  bool infcheck = std::isinf(val);
  if (leadingSep) {
    file << m_sep;
  }
  if (!nancheck && !infcheck) {
    file << val;
  } else if (infcheck) {
    // infinite - output 'inf'
    file << "inf";
  } else {
    // not a number - output nan
    file << "nan";
  }
}

/** appends the separator property to the algorithm
 */
void AsciiPointBase::appendSeparatorProperty() {
  std::vector<std::string> propOptions;
  propOptions.push_back("comma");
  propOptions.push_back("space");
  propOptions.push_back("tab");
  declareProperty("Separator", "tab",
                  boost::make_shared<StringListValidator>(propOptions),
                  "The separator used for splitting data columns.");
}
} // namespace DataHandling
} // namespace Mantid
