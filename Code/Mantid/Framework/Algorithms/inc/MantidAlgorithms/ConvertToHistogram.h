#ifndef MANTID_ALGORITHMS_CONVERTTOHISTOGRAM_H_
#define MANTID_ALGORITHMS_CONVERTTOHISTOGRAM_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/XDataConverter.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
      Converts a histogram workspace to point data by simply taking the centre point of the bin
      as the new point on the X axis
      
      @author Martyn Gigg, Tessella plc
      @date 2010-12-21
      
      Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
      
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
    class DLLExport ConvertToHistogram : public XDataConverter
    {
    public:
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "ConvertToHistogram"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Converts a workspace containing point data into one containing histograms.";}

      
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Transforms\\Axes";}

    private:
      
      /// Returns true if the algorithm needs to be run. 
      bool isProcessingRequired(const API::MatrixWorkspace_sptr inputWS) const;
      /// Checks the input workspace is consistent, throwing if not
      bool isWorkspaceLogical(const API::MatrixWorkspace_sptr inputWS) const;
      /// Returns the size of the new X vector
      std::size_t getNewXSize(const API::MatrixWorkspace_sptr inputWS) const;
      /// Calculate the X point values. Implement in an inheriting class.
      void calculateXPoints(const MantidVec & inputX, MantidVec &outputX) const;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTTOHISTOGRAM_H_*/
