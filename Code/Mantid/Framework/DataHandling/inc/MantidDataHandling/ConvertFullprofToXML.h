#ifndef MANTID_DATAHANDLING_CONVERTFULLPROFTOXML_H_
#define MANTID_DATAHANDLING_CONVERTFULLPROFTOXML_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"

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
  class DLLExport ConvertFullprofToXML : public API::Algorithm
  {
  public:
    ConvertFullprofToXML();
    virtual ~ConvertFullprofToXML();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "ConvertFullprofToXML";}

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

    /// Get row numbers of the parameters in the table workspace
    void getTableRowNumbers(const API::ITableWorkspace_sptr & tablews, std::map<std::string, size_t>& parammap);

  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_CONVERTFULLPROFTOXML_H_ */
