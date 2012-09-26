#ifndef MANTID_DATAHANDLING_LOADPDFGETNFILE_H_
#define MANTID_DATAHANDLING_LOADPDFGETNFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace DataHandling
{

  /** LoadPDFgetNFile : TODO: DESCRIPTION
    
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport LoadPDFgetNFile : public API::Algorithm
  {
  public:
    LoadPDFgetNFile();
    virtual ~LoadPDFgetNFile();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "LoadPDFgetNFile";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction;DataHandling\\Text";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Implement abstract Algorithm methods
    void init();
    /// Implement abstract Algorithm methods
    void exec();

    /// Parse PDFgetN data file
    void parseDataFile(std::string filename);

    /// Check whether a string starts from a specified sub-string
    bool startsWith(std::string s, std::string header);

    /// Parse column name line staring with #L
    void parseColumnNameLine(std::string line);

    /// Parse data line
    void parseDataLine(std::string line);

    /// Output data workspace
    DataObjects::Workspace2D_sptr outWS;

    /// Data structure to hold input
    std::vector<std::vector<double> > mData;

    /// Names of the columns of the data
    std::vector<std::string> mColumnNames;

    /// Generate output workspace
    void generateDataWorkspace();
    
  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADPDFGETNFILE_H_ */
