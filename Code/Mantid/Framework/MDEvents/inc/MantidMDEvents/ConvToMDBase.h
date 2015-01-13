#ifndef MANTID_MDEVENTS_CONVERTMD_BASE_H
#define MANTID_MDEVENTS_CONVERTMD_BASE_H

#include "MantidKernel/Logger.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/ExperimentInfo.h"

#include "MantidAPI/MatrixWorkspace.h"

#include "MantidMDEvents/MDWSDescription.h"
#include "MantidMDEvents/MDEventWSWrapper.h"

#include "MantidMDEvents/MDTransfInterface.h"
#include "MantidMDEvents/MDTransfFactory.h"

// units conversion
#include "MantidMDEvents/UnitsConversionHelper.h"

namespace Mantid {
namespace MDEvents {
/** class describes the interface to the methods, which perform conversion from
  usual workspaces to MDEventWorkspace
  *
  * See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation
  for detailed description of this
  * class place in the algorithms hierarchy.
  *
  *
  * @date 07-01-2012

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

        File/ change history is stored at:
  <https://github.com/mantidproject/mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport ConvToMDBase {
public:
  // constructor;
  ConvToMDBase();

  /// method which initiates all main class variables
  virtual size_t initialize(const MDWSDescription &WSD,
                            boost::shared_ptr<MDEventWSWrapper> inWSWrapper,
                            bool ignoreZeros);
  /// method which starts the conversion procedure
  virtual void runConversion(API::Progress *) = 0;
  /// virtual destructor
  virtual ~ConvToMDBase(){};

  /** method returns unit conversion helper, used to convert input workspace
     units to the units, used by appropriate MD transformation
     (if such conversion is necessary) */
  UnitsConversionHelper &getUnitConversionHelper() { return m_UnitConversion; }

protected:
  // pointer to input matrix workspace;
  API::MatrixWorkspace_const_sptr m_InWS2D;
  // pointer to the class, which keeps target workspace and provides functions
  // adding additional MD events to it.
  boost::shared_ptr<MDEvents::MDEventWSWrapper> m_OutWSWrapper;
  // shared pointer to the converter, which converts WS coordinates to MD
  // coordinates
  MDTransf_sptr m_QConverter;
  /// number of target ws dimensions
  size_t m_NDims;
  // index of current run(workspace). Used for MD WS combining
  uint16_t m_RunIndex;
  //---> preprocessed detectors information
  // number of valid spectra
  uint32_t m_NSpectra;
  std::vector<size_t> m_detIDMap;
  std::vector<int32_t> m_detID;

  //<--- End of preprocessed detectors information
  // logger -> to provide logging, for MD dataset file operations
  static Mantid::Kernel::Logger g_Log;
  // vector to keep MD coordinates of single event
  std::vector<coord_t> m_Coord;
  // class responsible for converting units if necessary;
  UnitsConversionHelper m_UnitConversion;
  // the parameter, which control if algorithm should run multithreaded.
  // On multiprocessor machine the algorithm should run and utilizes all cores
  // (see Kernel::Threadpool),
  // but this can be changed setting this parameter to 0 (no multithreading) or
  // positive number specifying the requested number of threads
  int m_NumThreads;
  // Flag which indicates that data with 0 signal should be ignored
  bool m_ignoreZeros;
  /// Any special coordinate system used.
  Mantid::API::SpecialCoordinateSystem m_coordinateSystem;

private:
  /** internal function which do one peace of work, which should be performed by
    one thread
    *
    *@param job_ID -- the identifier which specifies, what part of the work on
    the workspace this job has to do.
                      Often it is a spectra number
    *
  */
  virtual size_t conversionChunk(size_t job_ID) = 0;
};

} // end namespace MDAlgorithms
} // end namespace Mantid
#endif
