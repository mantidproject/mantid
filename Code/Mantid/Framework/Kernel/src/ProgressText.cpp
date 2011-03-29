#include "MantidKernel/ProgressText.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <fstream>
#include <iomanip>

namespace Mantid
{
namespace Kernel
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param start :: float, fraction at the 0th step
   * @param end :: float, fraction at the last steps
   * @param nsteps :: number of (integer) steps to take between start and end
   * @param newLines :: put each report on a new line.
   *        Otherwise, the report gets updated on the same line with stars. A new line occurs at destruction.
   * @return
   */
  ProgressText::ProgressText(double start, double end, int nsteps, bool newLines)
  : m_newLines(newLines), m_lastMsgLength(0)
  {
    this->m_start = start;
    this->m_end = end;
    this->setNumSteps(nsteps);
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ProgressText::~ProgressText()
  {
    if (!m_newLines)
      std::cout << std::endl;
  }

  //----------------------------------------------------------------------------------------------
  /** Report progress, as text. This is called by the base class Progress->report() method.
   *
   * @param msg :: optional text message to add
   */
  void ProgressText::doReport(const std::string & msg)
  {
    double p = m_start + m_step*(m_i - m_ifirst);
    if (p > m_end) p = m_end;
    // Print out
    int pct = (p * 100);

    coutMutex.lock();

    // Return at the start of the line if not doing new lines
    if (!m_newLines)
      std::cout << "\r";

    // Put a line of up to 40 *; spaces otherwise.
    for (int i=0; i < 40; i++)
      std::cout << ((i*2.5 < pct) ? "*" : " ");
    std::cout << " " << std::setw(3) << pct << " % " << msg << "  ";

    if (!m_newLines)
    {
      // Overwrite the rest of the last message
      for (size_t i=msg.size(); i<m_lastMsgLength; i++)
        std::cout << " ";
      // Update the console
      std::cout.flush();
    }
    else
      std::cout << std::endl;

    m_lastMsgLength = msg.size();

    // Save where we last reported to avoid notifying too often.
    this->m_last_reported = m_i;

    coutMutex.unlock();
  }


} // namespace Mantid
} // namespace API

