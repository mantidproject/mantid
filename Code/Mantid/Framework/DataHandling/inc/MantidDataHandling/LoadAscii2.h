#ifndef MANTID_DATAHANDLING_LoadAscii2_H_
#define MANTID_DATAHANDLING_LoadAscii2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
  namespace DataHandling
  {
    /**
    Loads a workspace from an ascii file. Spectra must be stored in columns.

    Properties:
    <ul>
    <li>Filename  - the name of the file to read from.</li>
    <li>Workspace - the workspace name that will be created and hold the loaded data.</li>
    <li>Separator - the column separation character: comma (default),tab,space,colon,semi-colon.</li>
    <li>Unit      - the unit to assign to the X axis (default: Energy).</li>
    </ul>

    @author Keith Brown, ISIS, Placement student from the University of Derby
    @date 10/10/13

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadAscii2 :public API::IFileLoader<Kernel::FileDescriptor>
    {
    public:
      /// Default constructor
      LoadAscii2();
      /// The name of the algorithm
      virtual const std::string name() const { return "LoadAscii"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Loads data from a text file and stores it in a 2D workspace (Workspace2D class).";}

      /// The version number
      virtual int version() const { return 2; }
      /// The category
      virtual const std::string category() const { return "DataHandling\\Text"; }
      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(Kernel::FileDescriptor & descriptor) const;

    protected:
      /// Read the data from the file
      virtual API::Workspace_sptr readData(std::ifstream & file);
      /// Return true if the line is to be skipped
      bool skipLine(const std::string & line, bool header = false) const;
      /// Return true if the line doesn't start with a valid character
      bool badLine(const std::string & line) const;
      /// check and configure flags and values relating to starting a new spectra
      void newSpectra();
      /// Check if the file has been found to incosistantly include spectra IDs
      void inconsistantIDCheck() const;
      /// Split the data into columns.
      int splitIntoColumns(std::list<std::string> & columns, const std::string & str) const;
      /// Fill the given vector with the data values
      void fillInputValues(std::vector<double> &values, const std::list<std::string>& columns) const;
      //write the values in the current line to teh end fo teh current spectra
      void addToCurrentSpectra(std::list<std::string> & columns);
      //check that the nubmer of columns in the current line match the number found previously
      void checkLineColumns(const size_t & cols) const;
      //interpret a line that has been deemed valid enough to look at.
      void parseLine(const std::string & line, std::list<std::string> & columns);
      //find the number of collums we should expect from now on
      void setcolumns(std::ifstream & file, std::string & line, std::list<std::string> & columns);
      //wirte the spectra to the workspace
      void writeToWorkspace(API::MatrixWorkspace_sptr & localWorkspace, const size_t & numSpectra) const;
      //Process the header information. This implementation just skips it entirely.
      void processHeader(std::ifstream & file);
      /// The column separator
      std::string m_columnSep;

    private:
      
      /// Declare properties
      void init();
      /// Execute the algorithm
      void exec();

      /// Map the separator options to their string equivalents
      std::map<std::string,std::string> m_separatorIndex;
      std::string m_comment;
      size_t m_baseCols;
      size_t m_specNo;
      size_t m_lastBins;
      size_t m_curBins;
      bool m_spectraStart;
      size_t m_spectrumIDcount;
      size_t m_lineNo;
      std::vector<DataObjects::Histogram1D> m_spectra;
      DataObjects::Histogram1D *m_curSpectra;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LoadAscii2_H_  */
