// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/Logger.h"

#include "MantidMDAlgorithms/MDEventWSWrapper.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDTransfInterface.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

namespace Mantid {
namespace MDAlgorithms {
/** Class describes the interface to the methods, which perform conversion from
    usual workspaces to MDEventWorkspace

    See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation
    for detailed description of this
    class place in the algorithms hierarchy.


    @date 07-01-2012
*/

class DLLExport ConvToMDBase {
public:
  // constructor;
  ConvToMDBase();

  /// method which initiates all main class variables
  virtual size_t initialize(const MDWSDescription &WSD, std::shared_ptr<MDEventWSWrapper> inWSWrapper,
                            bool ignoreZeros);
  /// method which starts the conversion procedure
  virtual void runConversion(API::Progress *) = 0;
  /// virtual destructor
  virtual ~ConvToMDBase() = default;
  /// Set the normalization options
  virtual void setDisplayNormalization(Mantid::API::IMDEventWorkspace_sptr mdWorkspace,
                                       const Mantid::API::MatrixWorkspace_sptr &underlyingWorkspace);

  /** method returns unit conversion helper, used to convert input workspace
     units to the units, used by appropriate MD transformation
     (if such conversion is necessary) */
  UnitsConversionHelper &getUnitConversionHelper() { return m_UnitConversion; }

protected:
  // pointer to input matrix workspace;
  API::MatrixWorkspace_const_sptr m_InWS2D;
  // pointer to the class, which keeps target workspace and provides functions
  // adding additional MD events to it.
  std::shared_ptr<MDEventWSWrapper> m_OutWSWrapper;
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
  Mantid::Kernel::SpecialCoordinateSystem m_coordinateSystem;

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