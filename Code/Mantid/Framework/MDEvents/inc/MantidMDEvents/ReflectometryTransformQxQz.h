#ifndef MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZ_H_
#define MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZ_H_

#include "MantidKernel/System.h"
#include "MantidKernel/ClassMacros.h"
#include "MantidMDEvents/ReflectometryTransform.h"
#include "MantidMDEvents/CalculateReflectometryQBase.h"

namespace Mantid
{

  namespace MDEvents
  {

    /**
     Converts from inputs of wavelength, incident theta and final theta to Qx for reflectometry experiments
     */
    class CalculateReflectometryQx: public CalculateReflectometryQBase
    {
    private:
      double m_cos_theta_i;
      double m_dirQx;
    public:
      /**
       Constructor
       @param thetaIncident: incident theta value in degrees
       */
      CalculateReflectometryQx(const double& thetaIncident) :
          m_cos_theta_i(cos(thetaIncident * to_radians_factor))
      {
      }
      /**
       Setter for the final theta value require for the calculation. Internally pre-calculates and caches to cos theta for speed.
       @param thetaFinal: final theta value in degrees
       */
      void setThetaFinal(const double& thetaFinal)
      {
        const double c_cos_theta_f = cos(thetaFinal * to_radians_factor);
        m_dirQx = (c_cos_theta_f - m_cos_theta_i);
      }
      /**
       Executes the calculation to determine Qz
       @param wavelength : wavelenght in Anstroms
       */
      double execute(const double& wavelength) const
      {
        double wavenumber = 2 * M_PI / wavelength;
        return wavenumber * m_dirQx;
      }
    };

    /**
     Converts from inputs of wavelength, incident theta and final theta to Qz for reflectometry experiments
     */
    class CalculateReflectometryQz: public CalculateReflectometryQBase
    {
    private:
      double m_sin_theta_i;
      double m_dirQz;
    public:
      /**
       Constructor
       @param thetaIncident: incident theta value in degrees
       */
      CalculateReflectometryQz(const double& thetaIncident) :
          m_sin_theta_i(sin(thetaIncident * to_radians_factor))
      {
      }
      /**
       Setter for the final theta value require for the calculation. Internally pre-calculates and caches to sine theta for speed.
       @param thetaFinal: final theta value in degrees
       */
      void setThetaFinal(const double& thetaFinal)
      {
        const double c_sin_theta_f = sin(thetaFinal * to_radians_factor);
        m_dirQz = (c_sin_theta_f + m_sin_theta_i);
      }
      /**
       Executes the calculation to determine Qz
       @param wavelength : wavelenght in Anstroms
       */
      double execute(const double& wavelength) const
      {
        double wavenumber = 2 * M_PI / wavelength;
        return wavenumber * m_dirQz;
      }
    };

    /** ReflectometryTranformQxQz : Type of ReflectometyTransform. Used to convert from an input R vs Wavelength workspace to a 2D MDEvent workspace with dimensions of QxQy.
     Transformation is specific for reflectometry purposes.

     @date 2012-05-29

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
    class DLLExport ReflectometryTransformQxQz: public ReflectometryTransform
    {
    private:
      const double m_qxMin;
      const double m_qxMax;
      const double m_qzMin;
      const double m_qzMax;
      /// Object performing raw calculation to determine Qx
      mutable CalculateReflectometryQx m_QxCalculation;
      /// Object performing raw calculation to determine Qx
      mutable CalculateReflectometryQz m_QzCalculation;
    public:

      /// Constructor
      ReflectometryTransformQxQz(double qxMin, double qxMax, double qzMin, double qzMax,
          double incidentTheta, int numberOfBinsQx = 100, int numberOfBinsQz = 100);
      /// Destructor
      virtual ~ReflectometryTransformQxQz();
      /// Execute transformation
      virtual Mantid::API::MatrixWorkspace_sptr execute(
          Mantid::API::MatrixWorkspace_const_sptr inputWs) const;
      /// Execute MD transformation
      virtual Mantid::API::IMDEventWorkspace_sptr executeMD(
          Mantid::API::MatrixWorkspace_const_sptr inputWs,
          Mantid::API::BoxController_sptr boxController) const;

    private:

      DISABLE_DEFAULT_CONSTRUCT(ReflectometryTransformQxQz)
      DISABLE_COPY_AND_ASSIGN(ReflectometryTransformQxQz)

    };

  } // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZ_H_ */
