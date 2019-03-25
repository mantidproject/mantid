// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/Logger.h"
#include <atomic>
#include <iomanip>

namespace Mantid {
namespace Kernel {

namespace {
Logger logger("ProgressText");
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param start :: float, fraction at the 0th step
 * @param end :: float, fraction at the last steps
 * @param nsteps :: number of (integer) steps to take between start and end
 * @param newLines :: put each report on a new line.
 *        Otherwise, the report gets updated on the same line with stars. A new
 *line occurs at destruction.
 * @return
 */
ProgressText::ProgressText(double start, double end, int nsteps, bool newLines)
    : m_newLines(newLines), m_lastMsgLength(0) {
  this->m_start = start;
  this->m_end = end;
  this->setNumSteps(nsteps);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ProgressText::~ProgressText() {
  if (!m_newLines)
    logger.debug() << '\n';
}

//----------------------------------------------------------------------------------------------
/** Report progress, as text. This is called by the base class
 *Progress->report() method.
 *
 * @param msg :: optional text message to add
 */
void ProgressText::doReport(const std::string &msg) {
  double p = m_start + m_step * double(m_i - m_ifirst);
  if (p > m_end)
    p = m_end;
  // Print out
  int pct = static_cast<int>(p * 100.0);

  std::lock_guard<std::mutex> lock(coutMutex);

  // Return at the start of the line if not doing new lines
  if (!m_newLines)
    logger.debug() << "\r";

  // Put a line of up to 40 *; spaces otherwise.
  for (int i = 0; i < 40; i++)
    logger.debug() << ((i * 2.5 < pct) ? "*" : " ");
  logger.debug() << " " << std::setw(3) << pct << " % " << msg << "  ";

  if (!m_newLines) {
    // Overwrite the rest of the last message
    for (size_t i = msg.size(); i < m_lastMsgLength; i++)
      logger.debug() << " ";
    // Update the console
    logger.debug().flush();
  } else
    logger.debug() << '\n';

  m_lastMsgLength = msg.size();

  // Save where we last reported to avoid notifying too often.
  this->m_last_reported.store(m_i.load());
}

} // namespace Kernel
} // namespace Mantid
