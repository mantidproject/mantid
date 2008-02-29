#ifndef MANTIDAPI_POINTDATAREF_H
#define MANTIDAPI_POINTDATAREF_H

#include "MantidKernel/System.h"
#include "MantidAPI/IPointData.h"

namespace Mantid
{

namespace API
{

  //forward declaration
  class IErrorHelper;

/**
  The PointDataRef class holds references to all of the data required to describe a single data item of a point dataset.
  All of the data items are held as pointers to data owned by classes other than itself.  
  This is done for performance reasons when iterating over workspaces.

  \author N. Draper
    
  Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
  class DLLExport PointDataRef : public IPointData
{
  public:
  const double& X() const;
  const double& E() const;
  const double& E2() const;
  const IErrorHelper* ErrorHelper() const;
  int SpectraNo() const;

  double& X();
  double& E();
  double& E2();
  virtual const bool isE2() const {return (e2Pointer!=0);}

  const double& Y() const;
  double& Y();

  double* xPointer;        ///< Pointer to the value of X
  double* yPointer;        ///< Pointer to the value of Y
  double* ePointer;        ///< Pointer to the value of E
  double* e2Pointer;       ///< Pointer to the value of E2
  const IErrorHelper* errorHelper;  
  int spectraNo;


  PointDataRef* clone() const;

  PointDataRef();
  PointDataRef(const PointDataRef&);
  PointDataRef& operator=(const PointDataRef&);
  PointDataRef& operator=(const IPointData&);
  virtual ~PointDataRef();

  int operator<(const PointDataRef&) const;
  int operator>(const PointDataRef&) const;
  int operator==(const PointDataRef&) const;
  int operator!=(const PointDataRef&) const;
};

}  // NAMESPACE API

}  // NAMESPACE Mantid

#endif //MANTIDAPI_POINTDATAREF_H
