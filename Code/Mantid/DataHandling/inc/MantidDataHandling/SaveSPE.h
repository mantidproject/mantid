#ifndef MANTID_DATAHANDLING_SAVESPE_H_
#define MANTID_DATAHANDLING_SAVESPE_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/**
     Saves a workspace into an ASCII SPE file.

     Required properties:
     <UL>
     <LI> InputWorkspace - The workspace name to save. </LI>
     <LI> Filename - The filename for output </LI>
     </UL>

     @author Stuart Campbell, NScD, Oak Ridge National Laboratory
     @date 08/09/2009

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
class DLLExport SaveSPE : public API::Algorithm
{
public:
  /// Constructor
  SaveSPE();
  /// Virtual destructor
  virtual ~SaveSPE() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SaveSPE"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

  /// the mask flag (=-1e30) from the SPE specification http://www.mantidproject.org/images/3/3d/Spe_file_format.pdf
  static const double MASK_FLAG;

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  void writeHists(const API::MatrixWorkspace_const_sptr WS, FILE * const outFile);
  void writeHist(const API::MatrixWorkspace_const_sptr WS, FILE * const outFile, const int specIn) const;
  void writeMaskFlags(FILE * const outFile) const;
  void writeBins(const MantidVec &Vs, FILE * const outFile) const;
  void writeValue(const double value, FILE * const outFile) const;
  void logMissingMasked(const std::vector<int> &inds, const int nonMasked, const int masked) const;
  
  ///the SPE files have a constant number of numbers writen on each line, but depending on the number of bins there will be some "spare" numbers at the end of the block, this holds that number of spares
  int m_remainder;
  ///the number of bins in each histogram, as the histogram must have common bins this shouldn't change
  int m_nBins;

  /// the error value (=0.0) for spectra whose detectors are all masked, from the SPE specification http://www.mantidproject.org/images/3/3d/Spe_file_format.pdf
  static const double MASK_ERROR;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_SAVESPE_H_

