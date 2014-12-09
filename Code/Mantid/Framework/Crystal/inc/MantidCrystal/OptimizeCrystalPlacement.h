/*
* OptimizeCrystalPlacement.h
*
*  Created on: Jan 26, 2013
*      Author: ruth
*/

#ifndef OPTIMIZECRYSTALPLACEMENT_H_
#define OPTIMIZECRYSTALPLACEMENT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Crystal
  {

    /** OptimizeCrystalPlacement

    Description:
    This algorithm basically indexes peaks with the crystal orientation matrix stored in the peaks workspace.
    The optimization is on the goniometer settings for the runs in the peaks workspace and also the sample
    orientation .
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
    class DLLExport OptimizeCrystalPlacement  : public API::Algorithm
    {
    public:
      OptimizeCrystalPlacement();
      virtual ~OptimizeCrystalPlacement();

      virtual const std::string name() const
      {
        return "OptimizeCrystalPlacement";
      };
      ///Summary of algorithms purpose
      virtual const std::string summary() const {return "This algorithm  optimizes goniometer settings  and sample orientation to better index the peaks.";}


      virtual  int version() const
      {
        return 1;
      };

      const std::string category() const
      { return "Crystal";
      };

    private:


      void init();

      void exec();
    };
  }
}

#endif /* OPTIMIZECRYSTALPLACEMENT_H_ */
