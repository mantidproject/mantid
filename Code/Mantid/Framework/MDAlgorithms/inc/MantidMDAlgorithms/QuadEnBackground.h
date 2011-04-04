#ifndef MANTID_MDALGORITHMS_QUADENBACKGROUND_H_
#define MANTID_MDALGORITHMS_QUADENBACKGROUND_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDIterator.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        /**
        Provide Quadratic background in energy: Bg = c+eps*(l+eps*q)

        Required properties:
        <UL>
        <LI> Constant  -  Constant  coefficient </LI>
        <LI> Linear    -  Linear    coefficient </LI>
        <LI> Quadratic -  Quadratic coefficient </LI>
        </UL>

        @author Ron Fowler
        @date 31/03/2011

        Copyright &copy; 2007-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
        class DLLExport QuadEnBackground : public API::ParamFunction, public API::IFunctionMD
        {
        public:
            /// Constructor
            QuadEnBackground();
            /// Destructor
            virtual ~QuadEnBackground() {}

            /// overwrite IFunction base class methods
            std::string name()const{return "QuadEnBackground";}
        protected:
            /// function to return the calculated signal at cell r, given the energy dependent model applied to points
            virtual double function(Mantid::API::IMDIterator& r) const;
        };

    } // namespace MDAlgorithms
} // namespace Mantid

#endif /*MANTID_MDALGORITHMS_QUADENBACKGROUND_H_*/
