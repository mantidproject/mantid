/*
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

#include <iostream> 
#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace DataObjects
{


Histogram1D::Histogram1D() 
  /*!
    Constructor
   */
{ }



Histogram1D::Histogram1D(const Histogram1D& A) :
  refX(A.refX),refY(A.refY),refE(A.refE)
  /*!
    Copy constructor
    \param A :: Historgram to copy
   */
{ }

Histogram1D& 
Histogram1D::operator=(const Histogram1D& A)
  /*!
    Assignment operator
    \param A :: Historgram to copy
    \return *this
   */
{
  if (this!=&A)
    {
      refX=A.refX;
      refY=A.refY;
      refE=A.refE;
    }
  return *this;
}
  

Histogram1D::~Histogram1D() 
 /// Destructor : Nothing to do since refX, refY, and refE are managed ptr
{}


} // namespace DataObjects
} // namespace Mantid
