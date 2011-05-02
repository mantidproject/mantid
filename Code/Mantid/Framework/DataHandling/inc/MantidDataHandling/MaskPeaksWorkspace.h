#ifndef MANTID_DATAHANDLING_MaskPeaksWorkspace_H_
#define MANTID_DATAHANDLING_MaskPeaksWorkspace_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace DataHandling
{
/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS, ORNL
 @date 02/08/2011

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport MaskPeaksWorkspace: public API::Algorithm
{
public:
  /// Default constructor
  MaskPeaksWorkspace();
  /// Destructor
  virtual ~MaskPeaksWorkspace();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "MaskPeaksWorkspace"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }

private:
  API::MatrixWorkspace_sptr inputW;  ///< A pointer to the input workspace
  DataObjects::EventWorkspace_sptr eventW; ///< A pointer to the event workspace
  API::MatrixWorkspace_sptr outputW; ///< A pointer to the output workspace
  // Overridden Algorithm methods
  void init();
  void exec();
  /// Read in all the input parameters
  void retrieveProperties();
  int Xmin;        ///< The start of the X range for fitting
  int Xmax;        ///< The end of the X range for fitting
  int Ymin;        ///< The start of the Y range for fitting
  int Ymax;        ///< The end of the Y range for fitting
  int Binmin;        ///< The start of the Bin range for fitting
  int Binmax;        ///< The end of the TOF range for fitting
  int TOFmin;        ///< The start of the TOF range for fitting
  int TOFmax;        ///< The end of the TOF range for fitting
  int TOFPeak;       ///< The peak in the TOF range for fitting
  int tofISAW;       /// check if bin agrees with peak from ISAW
  
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MaskPeaksWorkspace_H_*/
