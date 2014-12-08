#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidAlgorithms/ReflectometryWorkflowBase.h"

namespace Mantid
{
  namespace Algorithms
  {

    /** ReflectometryReductionOne : Reflectometry reduction of a single input TOF workspace to an IvsQ workspace.

     Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport ReflectometryReductionOne: public ReflectometryWorkflowBase
    {
    public:

      /// Constructor
      ReflectometryReductionOne();
      /// Destructor
      virtual ~ReflectometryReductionOne();

      virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Reduces a single TOF/Lambda reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections.";}

      virtual int version() const;
      virtual const std::string category() const;

      /// Convert to an IvsQ workspace. Performs detector positional corrections based on the component name and the theta value.
      Mantid::API::MatrixWorkspace_sptr toIvsQ(API::MatrixWorkspace_sptr& toConvert, const bool correctPosition,
           OptionalDouble& thetaInDeg, const bool isPointDetector);

    private:

      /** Overridden Algorithm methods **/
  

      void init();

      void exec();

      /// Get the surface sample component
      Mantid::Geometry::IComponent_const_sptr getSurfaceSampleComponent(Mantid::Geometry::Instrument_const_sptr inst);

      /// Get the detector component
      Mantid::Geometry::IComponent_const_sptr getDetectorComponent(Mantid::Geometry::Instrument_const_sptr inst, const bool isPointDetector);

      /// Correct detector positions.
      API::MatrixWorkspace_sptr correctPosition(API::MatrixWorkspace_sptr& toCorrect, const double& thetaInDeg, const bool isPointDetector);

      /// Sum spectra.
      Mantid::API::MatrixWorkspace_sptr sumSpectraOverRange(API::MatrixWorkspace_sptr inWS, const int startIndex, const int endIndex);

      /// Perform a transmission correction on the input IvsLam workspace
      API::MatrixWorkspace_sptr transmissonCorrection(API::MatrixWorkspace_sptr IvsLam,
          const MinMax& wavelengthInterval, const MinMax& wavelengthMonitorBackgroundInterval,
          const MinMax& wavelengthMonitorIntegrationInterval, const int& i0MonitorIndex,
          API::MatrixWorkspace_sptr firstTransmissionRun,
          OptionalMatrixWorkspace_sptr secondTransmissionRun, const OptionalDouble& stitchingStartQ,
          const OptionalDouble& stitchingDeltaQ, const OptionalDouble& stitchingEndQ,
          const OptionalDouble& stitchingStartOverlapQ, const OptionalDouble& stitchingEndOverlapQ,
          const double& wavelengthStep, const std::string& processingCommands);

      /// Verify spectrum maps
      void verifySpectrumMaps(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2, const bool severe = false);

    };

  } // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE_H_ */
