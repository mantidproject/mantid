#ifndef DATAHANDING_SAVEGSS_H_
#define DATAHANDING_SAVEGSS_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid
{
namespace DataHandling
{
/**
     Saves a focused data set
     into a three column GSAS format containing X_i, Y_i*step, and E_i*step. Exclusively for
     the crystallography package GSAS and data needs to be in time-of-flight
     For data where the focusing routine has generated several spectra (for example, multi-bank instruments),
     the option is provided for saving all spectra into a single file, separated by headers, or into
     several files that will be named "workspaceName_"+spectra_number.

     Required properties:
     <UL>
     <LI> InputWorkspace - The workspace name to save. </LI>
     <LI> Filename       - The filename for output </LI>
     </UL>

     Optional properties:
     <UL>
     <LI> SplitFiles - Option for splitting into N files for workspace with N-spectra</LI>
     <LI> Append     - Append to Filename, if it already exists (default: true).</LI>
     <LI> Bank       - The bank number of the first spectrum (default: 1)</LI>
     </UL>

     @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
     @date 04/03/2009

     Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport SaveGSS : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  SaveGSS() : Mantid::API::Algorithm(){}
  /// Virtual destructor
  virtual ~SaveGSS() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SaveGSS"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Saves a focused data set into a three column GSAS format.";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Diffraction;DataHandling\\Text"; }

private:
  
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  /// Write GSAS file
  void writeGSASFile(const std::string& outfilename, bool append, int basebanknumber, bool multiplybybinwidth,  bool split,
                     const std::string& outputFormat);

  ///Write the header information
  void writeHeaders(const std::string &format, std::stringstream &os,double primaryflightpath) const;
  ///Write out the data in RALF format
  void writeRALFdata(const int bank, const bool MultiplyByBinWidth, std::stringstream &out,
                     const MantidVec& X, const MantidVec& Y, const MantidVec& E) const;
  ///Write out the data in SLOG format
  void writeSLOGdata(const int bank, const bool MultiplyByBinWidth, std::stringstream &out,
                     const MantidVec& X, const MantidVec& Y, const MantidVec& E) const;
  /// sets non workspace properties for the algorithm
  void setOtherProperties(IAlgorithm* alg,const std::string & propertyName,const std::string &propertyValue,int periodNum);

  bool m_useSpecAsBank;

  /// Workspace
  API::MatrixWorkspace_const_sptr inputWS;

};

}
}
#endif //DATAHANDING_SAVEGSS_H_
