#ifndef MANTID_DATAHANDLING_LOADREFLTBL_H_
#define MANTID_DATAHANDLING_LOADREFLTBL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
  namespace DataHandling
  {
    /**
    Loads a table workspace from an ascii file in reflectometry tbl format. Rows must be no longer than 17 cells.

    Copyright &copy; 2007-2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport LoadReflTBL :public API::IFileLoader<Kernel::FileDescriptor>
    {
    public:
      /// Default constructor
      LoadReflTBL();
      /// The name of the algorithm
      virtual const std::string name() const { return "LoadReflTBL"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Loads data from a reflectometry table file and stores it in a table workspace (TableWorkspace class).";}

      /// The version number
      virtual int version() const { return 1; }
      /// The category
      virtual const std::string category() const { return "DataHandling\\Text"; }
      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(Kernel::FileDescriptor & descriptor) const;

    private:
      
      /// Declare properties
      void init();
      /// Execute the algorithm
      void exec();
      /// Split into columns with respect to the comma delimiters
      size_t getCells(std::string line, std::vector<std::string> & cols) const;
      /// count the number of commas in the line
      size_t countCommas (std::string line) const;
      /// find all pairs of quotes in the line
      size_t findQuotePairs (std::string line, std::vector<std::vector<size_t>> & quoteBounds) const;
      /// Parse more complex CSV, used when the data involves commas in the data and quoted values
      void csvParse(std::string line, std::vector<std::string> & cols, std::vector<std::vector<size_t>> & quoteBounds) const;
      /// the perfect number of commas expected in a single line. more is fine, less is not (set to 16)
      const size_t m_expectedCommas;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LOADREFLTBL_H_  */
