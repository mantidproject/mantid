#ifndef MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILE_H_
#define MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{

class ChopperConfiguration
{
  // FIXME :: Add std:: to string and vectors
public:
  ChopperConfiguration(double freq, string bankidstr, string cwlstr, string mndspstr, string mxdspstr, string maxtofstr, string splitdstr, string vrunstr);
  ~ChopperConfiguration();

private:
  parseString();

  double m_frequency;
  std::vector<double> m_CWL;
  vector<double> m_mindsps;
  vector<double> m_maxdsps;
  vector<double> m_maxtofs;
  vector<int> m_bankIDs;
  vector<double> m_splitds;
  vector<int> m_vruns;
};

  /** SaveGSASInstrumentFile : TODO: DESCRIPTION
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport SaveGSASInstrumentFile : public API::Algorithm
  {
  public:
    SaveGSASInstrumentFile();
    virtual ~SaveGSASInstrumentFile();
    /// Algorithm's name
    virtual const std::string name() const { return "SaveGSASInstrumentFile"; }
    /// Algorithm's version
    virtual int version() const { return (1); }
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Diffraction;Algorithm\\Text"; }

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialisation code
    void init();
    /// Execution code
    void exec();


    /// Instrument
    std::string m_instrumetn;
    /// L1
    double m_L1;
    /// L2
    double m_L2;
    /// 2Theta
    double m_2theta;
    /// Frequency
    double m_frequency;

    
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILE_H_ */
