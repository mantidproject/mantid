#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT_H_

#include "MantidKernel/System.h"
#include "MantidAlgorithms/SpecularReflectionAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
  namespace Algorithms
  {

    /** SpecularReflectionPositionCorrect : Algorithm to perform vertical position corrections based on the specular reflection condition.

     Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport SpecularReflectionPositionCorrect: public SpecularReflectionAlgorithm
    {
    public:
      SpecularReflectionPositionCorrect();
      virtual ~SpecularReflectionPositionCorrect();

      virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Correct detector positions vertically based on the specular reflection condition.";}

      virtual int version() const;
      virtual const std::string category() const;

    private:
  
      void init();
      void exec();

      /// Correct detector positions.
      void correctPosition(API::MatrixWorkspace_sptr toCorrect, const double& twoThetaInDeg,
          Geometry::IComponent_const_sptr sample, Geometry::IComponent_const_sptr detector);

      /// Move detectors.
      void moveDetectors(API::MatrixWorkspace_sptr toCorrect, Geometry::IComponent_const_sptr detector, Geometry::IComponent_const_sptr sample, const double& upOffset, const double& acrossOffset, const Mantid::Kernel::V3D& detectorPosition);
    };

  } // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT_H_ */
