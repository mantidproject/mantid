#ifndef MANTID_DATAHANDLING_SAVEVTP_H_
#define MANTID_DATAHANDLING_SAVEVTP_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid 
{
namespace DataHandling
{
  /**
     Saves a 1D or 2D workspace using the vtk file format described in the "file formats"
     document at http://www.vtk.org/documents.php .
     This version saves the information in the newer XML format using a <I>.vtp</I> 
     file extension. The class is implemented as an algorithm and overrides the init() 
     and exec() functions.

     Required properties:
     <UL>
     <LI> Filename - The name of the used to store the workspace, without an extension</LI>
     <LI> InputWorkspace - The name of the workspace to save.
     </UL>

     Optional properties:
     <UL>
     <LI> Xminimum - The minimum value of the histogram x-axis to print
     <LI> Xmaximum - The maximum value of the histogram x-axis to print
     </UL>

     @author Martyn Gigg, Tessella Support Services plc
     @date 05/11/2008
     
     Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
     
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
  class DLLExport SaveVTK : public API::Algorithm 
  {
    
  public:
    /// Default constructor
    SaveVTK();

    /// Algorithm's name 
    virtual const std::string name() const { return "SaveVTK";};
    /// Algorithm's version 
    virtual int version() const { return 1;};
    /// Algorithm's category 
    virtual const std::string category() const { return "DataHandling";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    
    /// Override virtual init function
    void init();

    /// Override virtual exec function
    void exec();

    /// Check the optional properties
    void checkOptionalProperties();

    /// Write a histogram to the file
    void writeVTKPiece(std::ostream & outXML, const std::vector<double>& dataX, const std::vector<double>& dataY, const std::vector<double>& dataE, int index) const; 

    /// The x-axis minimum
    double m_Xmin;

    /// The x-axis minimum
    double m_Xmax;
    
  };

}

}

#endif //MANTID_DATAHANDLING_SAVEVTP_H_
