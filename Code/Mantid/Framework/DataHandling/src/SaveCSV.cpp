/*
 Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
 */

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveCSV.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>  // used to get ofstream
#include <iomanip>  // setw() used below

/* @class SaveCSV

 @author Anders J. Markvardsen, ISIS, RAL
 @date 15/10/2007
 */

namespace Mantid
{
namespace DataHandling
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SaveCSV)

/// Sets documentation strings for this algorithm
void SaveCSV::initDocs()
{
  this->setWikiSummary("Saves a 1D or 2D [[workspace]] to a CSV file. ");
  this->setOptionalMessage("Saves a 1D or 2D workspace to a CSV file.");
}


using namespace Kernel;
using namespace API;
using API::WorkspaceProperty;
using API::MatrixWorkspace;
using API::MatrixWorkspace_sptr;
using DataObjects::Workspace1D;
using DataObjects::Workspace2D;
using DataObjects::Workspace1D_sptr;
using DataObjects::Workspace2D_sptr;

/// Empty default constructor
SaveCSV::SaveCSV()
{
}

/** Initialisation method. Does nothing at present.
 *
 */
void SaveCSV::init()
{
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "The filename of the output CSV file" );
  declareProperty(new FileProperty("Filename", "",FileProperty::Save, ".csv"),
    "The name of the workspace containing the data you want to save to\n"
    "a CSV file" );
  declareProperty("Separator", ",",
    "The separator that will go between the numbers on a line in the\n"
    "output file (default ',')" );
  declareProperty("LineSeparator", "\n",
    "The string to place at the end of lines (default new line\n"
    "character)" );
}

/** Executes the algorithm. Retrieve the Filename, separator and Lineseparator
 *  properties and save workspace to Filename.
 *
 *  @throw NotImplementedError Thrown if workspace to save is not a 1D or 2D workspace
 *  @throw Mantid::Kernel::Exception::FileError  Thrown if errors with file opening and existence
 */
void SaveCSV::exec()
{
  // Gets the name of the file to save the workspace to; and the
  // separator and Lineseparator properties if they are provided by the user.

  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  // Get the values of the optional parameters
  m_separator = getPropertyValue("Separator");
  m_lineSeparator = getPropertyValue("LineSeparator");
  g_log.debug() << "Parameters: Filename='" << m_filename << "' " <<
    "Seperator='" << m_separator << "' " <<
    "LineSeparator='" << m_lineSeparator << "' " << std::endl;

  // prepare to save to file

  std::ofstream outCSV_File(m_filename.c_str());

  if (!outCSV_File)
  {
    g_log.error("Failed to open file:" + m_filename);
    throw Exception::FileError("Failed to open file:" , m_filename);
  }

  // Get the input workspace
  const MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // get workspace ID string. Used to differentiate between
  // workspace1D and workspace2D in the if statement below

  const std::string workspaceID = inputWorkspace->id();

  // seperating out code depending on the workspace ID


  if (workspaceID == "Workspace1D")
  {

    const Workspace1D_sptr localworkspace = boost::dynamic_pointer_cast<Workspace1D>(inputWorkspace);

    // Get info from 1D workspace

    const MantidVec& xValue = localworkspace->dataX();
    const MantidVec& yValue = localworkspace->dataY();
    const MantidVec& eValue = localworkspace->dataE();
    Progress p(this, 0, 1, static_cast<int>(xValue.size()));
    // write to file
    
    for (int i = 0; i < (int)xValue.size(); i++)
    {
      outCSV_File << std::setw(15) << xValue[i] << m_separator << std::setw(15)
          << yValue[i] << m_separator << std::setw(15) << eValue[i]
          << m_lineSeparator;
		  p.report();
    }
  }
  else if ( workspaceID.find("Workspace2D") != std::string::npos )
  {
	 
   
    const Workspace2D_sptr localworkspace = boost::dynamic_pointer_cast<Workspace2D>(inputWorkspace);

    // Get info from 2D workspace

    const int numberOfHist = localworkspace->getNumberHistograms();
    
	
    // Add first x-axis line to output file

    {
      const MantidVec& xValue = localworkspace->dataX(0);

      outCSV_File << "A";

      for (int j = 0; j < (int)xValue.size(); j++)
      {
        outCSV_File << std::setw(15) << xValue[j] << m_separator;
      }

      outCSV_File << m_lineSeparator;
	  progress(0.2);
    }
    Progress p(this,0.2,1.0,2*numberOfHist);
    for (int i = 0; i < numberOfHist; i++)
    {
      // check if x-axis has changed. If yes print out new x-axis line

      if (i > 0)
      {
        const MantidVec& xValue = localworkspace->dataX(i);
        const MantidVec& xValuePrevious = localworkspace->dataX(i-1);

        if (xValue != xValuePrevious)
        {
          outCSV_File << "A";

          for (int j = 0; j < (int)xValue.size(); j++)
          {
            outCSV_File << std::setw(15) << xValue[j] << m_separator;
          }

          outCSV_File << m_lineSeparator;
        }
      }

      // add y-axis line for histogram (detector) i

      const MantidVec& yValue = localworkspace->dataY(i);

      outCSV_File << i;

      for (int j = 0; j < (int)yValue.size(); j++)
      {
        outCSV_File << std::setw(15) << yValue[j] << m_separator;
      }

      outCSV_File << m_lineSeparator;
	  p.report();
    }
    // print out errors

    outCSV_File << "\nERRORS\n";

    for (int i = 0; i < numberOfHist; i++)
    {
      const MantidVec& eValue = localworkspace->dataE(i);

      outCSV_File << i;

      for (int j = 0; j < (int)eValue.size(); j++)
      {
        outCSV_File << std::setw(15) << eValue[j] << m_separator;
      }
      outCSV_File << m_lineSeparator;
	   p.report();
    }

  }
  else
  {
    outCSV_File.close(); // and should probably delete file from disk as well
    throw Exception::NotImplementedError("SaveCSV currently only works for 1D and 2D workspaces.");
  }
  outCSV_File.close();
  //only gets here if everything happened normally
  return;
}

} // namespace DataHandling
} // namespace Mantid
