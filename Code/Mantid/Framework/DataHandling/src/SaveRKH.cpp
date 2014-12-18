//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveRKH.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include <Poco/LocalDateTime.h>
#include <Poco/DateTimeFormatter.h>

#include <iomanip>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveRKH)

using namespace API;

/// Constructor
SaveRKH::SaveRKH() : API::Algorithm(), m_workspace(), m_2d(false), m_outRKH() {}

/// Virtual destructor
SaveRKH::~SaveRKH() {}

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void SaveRKH::init() {
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "",
                                               Kernel::Direction::Input),
                  "The name of the workspace to save");
  std::vector<std::string> exts;
  exts.push_back(".txt");
  exts.push_back(".Q");
  exts.push_back(".dat");
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Save, exts),
      "The name to use when saving the file");
  declareProperty(
      "Append", true,
      "If true and Filename already exists, append, else overwrite");
}

/**
 * Execute the algorithm
 */
void SaveRKH::exec() {
  // Retrieve the input workspace
  m_workspace = getProperty("InputWorkspace");

  m_2d =
      (m_workspace->getNumberHistograms() > 1 && m_workspace->blocksize() > 1)
          ? true
          : false;

  // If a 2D workspace, check that it has two numeric axes - bail out if not
  if (m_2d && !(m_workspace->getAxis(1)->isNumeric())) {
    g_log.error(
        "This algorithm expects a 2d workspace to have been converted away from \
                 spectrum numbers on the vertical axis");
    throw std::invalid_argument("Cannot write out this kind of workspace");
  }

  // Check whether to append to an already existing file or overwrite
  using std::ios_base;
  ios_base::openmode mode =
      (getProperty("Append") ? (ios_base::out | ios_base::app) : ios_base::out);
  // Open/create the file
  const std::string filename = getProperty("Filename");
  m_outRKH.open(filename.c_str(), mode);

  if (!m_outRKH) {
    g_log.error() << "An error occurred while attempting to open the file "
                  << filename << "\n";
    throw std::runtime_error(
        "An error occurred while trying to open the output file for writing");
  }

  // Write out the header
  this->writeHeader();

  // Now write out the data using the appropriate method
  if (m_2d)
    this->write2D();
  else
    this->write1D();

  // Close the file
  m_outRKH.close();
}

/// Writes out the header of the output file
void SaveRKH::writeHeader() {
  // The instrument name
  m_outRKH << " " << m_workspace->getInstrument()->getName() << " ";
  Poco::LocalDateTime timestamp;
  // The sample file has the format of the data/time as in this example Thu
  // 28-OCT-2004 12:23
  m_outRKH << Poco::DateTimeFormatter::format(timestamp, std::string("%w"))
           << " "
           << Poco::DateTimeFormatter::format(timestamp, std::string("%d"))
           << "-";
  std::string month =
      Poco::DateTimeFormatter::format(timestamp, std::string("%b"));
  std::transform(month.begin(), month.end(), month.begin(), toupper);
  m_outRKH << month << "-" << Poco::DateTimeFormatter::format(
                                  timestamp, std::string("%Y %H:%M"))
           << " Workspace: " << getPropertyValue("InputWorkspace") << "\n";

  if (m_2d) {
    // The units that the data is in
    const Kernel::Unit_const_sptr unit1 = m_workspace->getAxis(0)->unit();
    const Kernel::Unit_const_sptr unit2 = m_workspace->getAxis(1)->unit();
    const int unitCode1 = unit1->caption() == "q" ? Q_CODE : 0;
    const int unitCode2 = unit2->caption() == "q" ? Q_CODE : 0;
    m_outRKH << "  " << unitCode1 << " " << unit1->caption() << " ("
             << unit1->label().ascii() << ")\n"
             << "  " << unitCode2 << " " << unit2->caption() << " ("
             << unit2->label().ascii() << ")\n"
             << "  0 " << m_workspace->YUnitLabel() << "\n"
             << "  1\n";
  }
  // The workspace title
  m_outRKH << " " << m_workspace->getTitle() << "\n";

  if (!m_2d) {
    const size_t noDataPoints = m_workspace->size();
    m_outRKH << std::setw(5) << noDataPoints << "    0    0    0    1"
             << std::setw(5) << noDataPoints << "    0\n"
             << "         0         0         0         0\n"
             << " 3 (F12.5,2E16.6)\n";
  }
}

/// Writes out the 1D data
void SaveRKH::write1D() {
  const size_t noDataPoints = m_workspace->size();
  const size_t nhist = m_workspace->getNumberHistograms();
  const bool horizontal = (nhist == 1) ? true : false;
  if (horizontal) {
    g_log.notice() << "Values in first column are the X values\n";
    if (m_workspace->getAxis(0)->unit())
      g_log.notice() << "in units of "
                     << m_workspace->getAxis(0)->unit()->unitID() << "\n";
  } else
    g_log.notice("Values in first column are spectrum numbers");
  const bool histogram = m_workspace->isHistogramData();
  Progress prg(this, 0.0, 1.0, noDataPoints);
  const size_t nbins = m_workspace->blocksize();

  for (size_t i = 0; i < nhist; ++i) {
    const auto &xdata = m_workspace->readX(i);
    const auto &ydata = m_workspace->readY(i);
    const auto &edata = m_workspace->readE(i);

    specid_t specid(0);
    try {
      specid = m_workspace->getSpectrum(i)->getSpectrumNo();
    } catch (...) {
      specid = static_cast<specid_t>(i + 1);
    }

    for (size_t j = 0; j < nbins; ++j) {
      // Calculate/retrieve the value to go in the first column
      double xval(0.0);
      if (horizontal)
        xval = histogram ? 0.5 * (xdata[j] + xdata[j + 1]) : xdata[j];
      else {
        xval = static_cast<double>(specid);
      }

      m_outRKH << std::fixed << std::setw(12) << std::setprecision(5) << xval
               << std::scientific << std::setw(16) << std::setprecision(6)
               << ydata[j] << std::setw(16) << edata[j] << "\n";

      prg.report();
    }
  }
}
/// Writes out the 2D data
void SaveRKH::write2D() {
  // First the axis values
  const Axis *const X = m_workspace->getAxis(0);
  const size_t Xbins = X->length();
  m_outRKH << "  " << Xbins << "\n";
  for (size_t i = 0; i < Xbins; ++i) {
    m_outRKH << std::setw(14) << std::scientific << std::setprecision(6)
             << (*X)(i);
    if ((i + 1) % LINE_LENGTH == 0)
      m_outRKH << "\n";
  }
  const Axis *const Y = m_workspace->getAxis(1);
  const size_t Ybins = Y->length();
  m_outRKH << "\n  " << Ybins << std::endl;
  for (size_t i = 0; i < Ybins; ++i) {
    m_outRKH << std::setw(14) << std::scientific << std::setprecision(6)
             << (*Y)(i);
    if ((i + 1) % LINE_LENGTH == 0)
      m_outRKH << "\n";
  }

  // Now the data
  const size_t xSize = m_workspace->blocksize();
  const size_t ySize = m_workspace->getNumberHistograms();
  m_outRKH << "\n   " << xSize << "   " << ySize << "  " << std::scientific
           << std::setprecision(12) << 1.0 << "\n";
  const int iflag = 3;
  m_outRKH << "  " << iflag << "(8E12.4)\n";

  bool requireNewLine = false;
  int itemCount(0);
  for (size_t i = 0; i < ySize; ++i) {
    const auto &ydata = m_workspace->readY(i);
    for (size_t j = 0; j < xSize; ++j) {
      m_outRKH << std::setw(12) << std::scientific << std::setprecision(4)
               << ydata[j];
      requireNewLine = true;
      if ((itemCount + 1) % LINE_LENGTH == 0) {
        m_outRKH << "\n";
        requireNewLine = false;
      }
      ++itemCount;
    }
  }
  // extra new line is required if number of data written out in last column is
  // less than LINE_LENGTH
  if (requireNewLine)
    m_outRKH << "\n";

  // Then all the error values
  itemCount = 0;
  for (size_t i = 0; i < ySize; ++i) {
    const auto &edata = m_workspace->readE(i);
    for (size_t j = 0; j < xSize; ++j) {
      m_outRKH << std::setw(12) << std::scientific << std::setprecision(4)
               << edata[j];
      if ((itemCount + 1) % LINE_LENGTH == 0)
        m_outRKH << "\n";
      ++itemCount;
    }
  }
}

} // namespace DataHandling
} // namespace Mantid
