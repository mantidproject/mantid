#ifndef MANTID_ALGORITHMS_HOLLOWCANMONTECARLOABSORPTION_H_
#define MANTID_ALGORITHMS_HOLLOWCANMONTECARLOABSORPTION_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  //-----------------------------------------------------------------------------------------------
  // Forward declarations
  //-----------------------------------------------------------------------------------------------
  namespace Kernel
  {
    class Material;
    class V3D;
  }
  namespace Geometry
  {
    class Object;
  }

  namespace Algorithms
  {

    /**

      Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport HollowCanMonteCarloAbsorption  : public API::Algorithm
    {
    public:
      HollowCanMonteCarloAbsorption();
      virtual ~HollowCanMonteCarloAbsorption();

      virtual const std::string name() const;
      virtual int version() const;
      virtual const std::string category() const;
      virtual const std::string summary() const;

    private:
      void init();
      void exec();

      void attachEnvironment(API::MatrixWorkspace_sptr & workspace);
      boost::shared_ptr<Geometry::Object> createEnvironmentShape() const;
      boost::shared_ptr<Kernel::Material> createEnvironmentMaterial() const;

      void attachSample(API::MatrixWorkspace_sptr & workspace);
      boost::shared_ptr<Geometry::Object> createSampleShape() const;
      void runSetSampleMaterial(API::MatrixWorkspace_sptr & workspace);

      API::MatrixWorkspace_sptr runMonteCarloAbsorptionCorrection(const API::MatrixWorkspace_sptr & workspace);

      const std::string cylinderXML(const std::string &id, const Kernel::V3D & bottomCentre, const double radius,
                                    const Kernel::V3D & axis, const double height) const;
      const std::string cuboidXML(const std::string &id, const Kernel::V3D & leftFrontBottom, const Kernel::V3D & leftBackBottom,
                                  const Kernel::V3D & leftFrontTop, const Kernel::V3D & rightFrontBottom) const;
    };


  } // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_HOLLOWCANMONTECARLOABSORPTION_H_ */
