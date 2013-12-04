#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>

namespace Mantid
{
  namespace Algorithms
  {

    /** ReflectometryReductionOne : Reflectometry reduction of a single input TOF workspace to an IvsQ workspace.

     Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport ReflectometryReductionOne: public API::DataProcessorAlgorithm
    {
    public:

      // Class typedefs
      typedef boost::tuple<double, double> MinMax;
      typedef boost::optional<double> OptionalDouble;
      typedef boost::optional<Mantid::API::MatrixWorkspace_sptr> OptionalMatrixWorkspace_sptr;
      typedef std::vector<int> WorkspaceIndexList;
      typedef boost::optional<std::vector<int> > OptionalWorkspaceIndexes;
      typedef boost::tuple<Mantid::API::MatrixWorkspace_sptr, Mantid::API::MatrixWorkspace_sptr> DetectorMonitorWorkspacePair;

      /// Constructor
      ReflectometryReductionOne();
      /// Destructor
      virtual ~ReflectometryReductionOne();

      virtual const std::string name() const;
      virtual int version() const;
      virtual const std::string category() const;

      /// Convert the input workspace to wavelength, splitting according to the properties provided.
      DetectorMonitorWorkspacePair toLam(Mantid::API::MatrixWorkspace_sptr toConvert,
          const WorkspaceIndexList& detectorIndexRange, const int monitorIndex,
          const MinMax& wavelengthMinMax, const MinMax& backgroundMinMax);

    private:

      /** Overridden Algorithm methods **/
      virtual void initDocs();

      void init();

      void exec();

      /** Auxillary getters and validators **/
      bool isPropertyDefault(const std::string& propertyName) const;

      /// Get a workspace index list.
      WorkspaceIndexList getWorkspaceIndexList() const;

      /// Get min max indexes.
      void fetchOptionalLowerUpperPropertyValue(const std::string& propertyName, bool isPointDetector,
          OptionalWorkspaceIndexes& optionalUpperLower) const;

      /// Get the min/max property values
      MinMax getMinMax(const std::string& minProperty, const std::string& maxProperty) const;

      /// Get the transmission correction properties
      void getTransmissionRunInfo(OptionalMatrixWorkspace_sptr& firstTransmissionRun,
          OptionalMatrixWorkspace_sptr& secondTransmissionRun, OptionalDouble& stitchingStartQ,
          OptionalDouble& stitchingDeltaQ, OptionalDouble& stitchingEndQ,
          OptionalDouble& stitchingStartOverlapQ, OptionalDouble& stitchingEndOverlapQ) const;

      /// Validate the transmission correction property inputs
      void validateTransmissionInputs() const;

      /** Algorithm running methods **/

      /// Convert the monitor parts of the input workspace to wavelength
      API::MatrixWorkspace_sptr toLamMonitor(const API::MatrixWorkspace_sptr& toConvert,
          const int monitorIndex, const MinMax& backgroundMinMax);

      /// Convert the detector spectrum of the input workspace to wavelength
      API::MatrixWorkspace_sptr toLamDetector(const WorkspaceIndexList& detectorIndexRange,
          const API::MatrixWorkspace_sptr& toConvert, const MinMax& wavelengthMinMax);

      /// Perform a transmission correction on the input IvsLam workspace
      API::MatrixWorkspace_sptr transmissonCorrection(API::MatrixWorkspace_sptr IvsLam,
          const MinMax& wavelengthInterval, const MinMax& wavelengthMonitorBackgroundInterval,
          const MinMax& wavelengthMonitorIntegrationInterval, const int& i0MonitorIndex,
          OptionalMatrixWorkspace_sptr firstTransmissionRun,
          OptionalMatrixWorkspace_sptr secondTransmissionRun, const OptionalDouble& stitchingStartQ,
          const OptionalDouble& stitchingDeltaQ, const OptionalDouble& stitchingEndQ,
          const OptionalDouble& stitchingStartOverlapQ, const OptionalDouble& stitchingEndOverlapQ);

    };

  } // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE_H_ */
