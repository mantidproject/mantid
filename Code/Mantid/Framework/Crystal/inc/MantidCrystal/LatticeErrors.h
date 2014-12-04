/*
 * LatticeErrors.h
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */

#ifndef PEAKHKLERRORS_H_
#define PEAKHKLERRORS_H_
#include "MantidKernel/System.h"

#include "MantidAPI/IFunction.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
  namespace Crystal
  {

    /**

      @author Ruth Mikkelson, SNS,ORNL
      @date 01/26/2013
     Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport LatticeErrors: public API::ParamFunction, public API::IFunction1D
    {
    public:
      LatticeErrors();
      virtual ~LatticeErrors();

      std::string name() const
      {
        return std::string("LatticeErrors");
      }
      ;

      virtual int version() const
      {
        return 1;
      }
      ;

      const std::string category() const
      {
        return "Crystal";
      }
      ;

      void function1D(double *out, const double *xValues, const size_t nData) const;

      //void functionDeriv1D(Mantid::API::Jacobian* out, const double *xValues, const size_t nData);

      void functionDerivLocal(API::Jacobian* , const double* , const size_t );
      void functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian);


      void init();
    };
  }
}

#endif /* PEAKHKLERRORS_H_ */
