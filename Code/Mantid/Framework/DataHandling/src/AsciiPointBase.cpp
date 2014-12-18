/*
AsciiPointBase is an abstract class holding the functionality for the
SaveILLCosmosAscii and SaveANSTOAscii export-only Acii-based save formats. It is
based on a python script by Maximilian Skoda, written for the ISIS Reflectometry
GUI
*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/AsciiPointBase.h"
#include "MantidAPI/FileProperty.h"
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

namespace Mantid {
namespace DataHandling {
using namespace Kernel;
using namespace API;

/// Initialisation method.
void AsciiPointBase::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "The name of the workspace containing the data you want to save.");

  std::vector<std::string> exts;
  exts.push_back(ext());
  declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
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
  const std::vector<double> &xTemp = m_ws->readX(0);
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

  const std::vector<double> &yData = m_ws->readY(0);
  const std::vector<double> &eData = m_ws->readE(0);
  if (exportDeltaQ) {
    for (size_t i = 0; i < m_xlength; ++i) {
      double dq = XData[i] * m_qres;
      outputval(XData[i], file, leadingSep());
      outputval(yData[i], file);
      outputval(eData[i], file);
      outputval(dq, file);
      file << std::endl;
    }
  } else {
    for (size_t i = 0; i < m_xlength; ++i) {
      outputval(XData[i], file, leadingSep());
      outputval(yData[i], file);
      outputval(eData[i], file);
      file << std::endl;
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
  bool nancheck = checkIfNan(val);
  bool infcheck = checkIfInfinite(val);
  if (leadingSep) {
    file << sep();
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

/** checks if a value is Not A Number
 *  @returns boolean true if the supplied value was Not a Number
 */
bool AsciiPointBase::checkIfNan(const double &value) const {
  return (boost::math::isnan(value));
}

/** checks if a value is Infinite
 *  @returns boolean true if the supplied value was Infinite
 */
bool AsciiPointBase::checkIfInfinite(const double &value) const {
  return (std::abs(value) == std::numeric_limits<double>::infinity());
}
} // namespace DataHandling
} // namespace Mantid
