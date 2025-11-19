// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/DownloadFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

#include "Poco/URI.h"

#include <stdexcept>
#include <string>

namespace Mantid::DataHandling {

using Mantid::API::FileProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::MandatoryValidator;
using Mantid::Kernel::StringListValidator;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DownloadFile)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
DownloadFile::DownloadFile() : m_internetHelper(new Kernel::InternetHelper()) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
DownloadFile::~DownloadFile() { delete m_internetHelper; }

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string DownloadFile::name() const { return "DownloadFile"; }

/// Algorithm's version for identification. @see Algorithm::version
int DownloadFile::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DownloadFile::category() const { return "DataHandling"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string DownloadFile::summary() const { return "Downloads a file from a url to the file system"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DownloadFile::init() {
  declareProperty("Address", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "The address of the network resource to download.", Direction::InOut);
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save),
                  "The filename to save the download to.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DownloadFile::exec() {
  std::string address = getProperty("Address");
  if ((!address.starts_with("http://")) && (!address.starts_with("https://"))) {
    address = "http://" + address;
    g_log.information("Address must start http:// or https://, http has been "
                      "assumed to continue: " +
                      address);
  }
  std::string filename = getProperty("Filename");

  Poco::URI url(address);
  m_internetHelper->downloadFile(url.toString(), filename);
  setProperty("Address", address);
}

} // namespace Mantid::DataHandling
