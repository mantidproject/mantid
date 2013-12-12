#ifndef MANTID_DATAHANDLING_LOADFULLPROFRESOLUTION_H_
#define MANTID_DATAHANDLING_LOADFULLPROFRESOLUTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace DataHandling
{

  /** LoadFullprofResolution : Load Fullprof resolution (.irf) file to TableWorkspace(s)
    
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
  class DLLExport LoadFullprofResolution : public API::Algorithm
  {
  public:
    LoadFullprofResolution();
    virtual ~LoadFullprofResolution();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "LoadFullprofResolution";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Implement abstract Algorithm methods
    void init();
    /// Implement abstract Algorithm methods
    void exec();

    /// Load file to a vector of strings
    void loadFile(std::string filename, std::vector<std::string>& lines);

    /// Get the NPROF number
    int getProfNumber(const std::vector<std::string>& lines);

    /// Scan imported file for bank information
    void scanBanks(const std::vector<std::string>& lines, std::vector<int>& banks,
                   std::map<int, int> &bankstartindexmap, std::map<int, int> &bankendindexmap);

    /// Parse .irf file to a map
    void parseResolutionStrings(std::map<std::string, double>& parammap, const std::vector<std::string>& lines, int bankid, int startlineindex, int endlineindex, int nProf);
    
    void parseBankLine(std::string line, double& cwl, int& bankid);

    /// Search token for profile number
    int searchProfile();

    /// Parse 1 bank of lines of profile 9
    void parseProfile9();

    /// Parse 1 bank of lines of profile 10
    void parseProfile10();

    /// Generate output workspace
    DataObjects::TableWorkspace_sptr genTableWorkspace(std::map<int, std::map<std::string, double> > bankparammap);

    /// Generate bank information workspace
    DataObjects::TableWorkspace_sptr genInfoTableWorkspace(std::vector<int> banks);

  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADFULLPROFRESOLUTION_H_ */
