#ifndef MANTID_ALGORITHMS_XDATACONVERTER_H_
#define MANTID_ALGORITHMS_XDATACONVERTER_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
      This is an abstract base class for sharing methods between algorithms that operate only
      on X data. Inheriting classes should overide the isRequired, checkInputWorkspace, getNewXSize and 
      setXData methods to return the appropriate values.

      @author Martyn Gigg, Tessella plc
      @date 2010-12-14
      
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
    class DLLExport XDataConverter : public API::Algorithm
    {
    public:
      /// Default constructor
      XDataConverter();
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "General";}

    protected:
      /// Returns true if the algorithm needs to be run. 
      virtual bool isProcessingRequired(const API::MatrixWorkspace_sptr inputWS) const = 0;
      /// Checks the input workspace is consistent, throwing if not
      virtual bool isWorkspaceLogical(const API::MatrixWorkspace_sptr inputWS) const = 0;
      /// Returns the size of the new X vector
      virtual std::size_t getNewXSize(const API::MatrixWorkspace_sptr inputWS) const = 0;
      /// Calculate the X point values. Implement in an inheriting class.
      virtual void calculateXPoints(const MantidVec & inputX, MantidVec &outputX) const = 0;
      
    private:
      /// Override init
      virtual void init();
      /// Override exec
      virtual void exec();

      /// Set the X data on given spectra
      void setXData(API::MatrixWorkspace_sptr outputWS, 
		    const API::MatrixWorkspace_sptr inputWS,
		    const int index);
      
      /// Flag if the X data is shared
      bool m_sharedX;
      /// Cached data for shared X values
      MantidVecPtr m_cachedX;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_XDATACONVERTER_H_*/
