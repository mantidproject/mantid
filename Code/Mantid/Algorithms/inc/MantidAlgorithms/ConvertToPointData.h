#ifndef MANTID_ALGORITHMS_CONVERTTOPOINTDATA_H_
#define MANTID_ALGORITHMS_CONVERTTOPOINTDATA_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
      Converts a histogram workspace to point data by simply taking the centre point of the bin
      as the new point on the X axis
      
      @author Martyn Gigg, Tessella plc
      @date 2010-12-14
      
      Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
      
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
    class DLLExport ConvertToPointData: public API::Algorithm
    {
    public:
      /// Default constructor
      ConvertToPointData();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "ConvertToPointData"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "General";}

    private:
      /// Override init
      void init();
      /// Override exec
      void exec();

      /// Set the X data on given spectra
      void setXData(API::MatrixWorkspace_sptr outputWS, const API::MatrixWorkspace_sptr inputWS,
		    const int index);
      /// Calculate the X point values
      void calculateXPoints(const MantidVec & inputX, MantidVec &outputX) const;

      /// Flag if the X data is shared
      bool m_sharedX;
      /// Cached data for shared X values
      MantidVecPtr m_cachedX;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTTOPOINTDATA_H_*/
