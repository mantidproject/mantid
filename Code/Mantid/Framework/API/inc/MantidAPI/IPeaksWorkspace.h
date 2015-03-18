#ifndef MANTID_API_IPEAKSPACE_H_
#define MANTID_API_IPEAKSPACE_H_ 1

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/SpecialCoordinateSystem.h"

namespace Mantid {

namespace API {
class IPeak;

//==========================================================================================
/** Interface to the class Mantid::DataObjects::PeaksWorkspace

    The class PeaksWorkspace stores information about a set of SCD peaks.

    @author Ruth Mikkelson, SNS ORNL
    @date 3/10/2010

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
 */
class MANTID_API_DLL IPeaksWorkspace : public ITableWorkspace,
                                       public Mantid::API::ExperimentInfo {
public:
  /// Ctor
  IPeaksWorkspace() : ITableWorkspace(), ExperimentInfo() {}

  /// Copy constructor
  IPeaksWorkspace(const IPeaksWorkspace &other)
      : ITableWorkspace(other), ExperimentInfo(other) {}

  /// Destructor
  virtual ~IPeaksWorkspace();

  // boost::shared_ptr<IPeaksWorkspace> clone() = 0;
  // void appendFile( std::string filename, Mantid::Geometry::Instrument_sptr
  // inst) = 0;

  //---------------------------------------------------------------------------------------------
  /** @return the number of peaks
   */
  virtual int getNumberPeaks() const = 0;

  //---------------------------------------------------------------------------------------------
  /** Removes the indicated peak
   * @param peakNum  the peak to remove. peakNum starts at 0
   */
  virtual void removePeak(int peakNum) = 0;

  //---------------------------------------------------------------------------------------------
  /** Add a peak to the list
   * @param ipeak :: Peak object to add (copy) into this.
   */
  virtual void addPeak(const IPeak &ipeak) = 0;

  //---------------------------------------------------------------------------------------------
  /** Return a reference to the Peak
   * @param peakNum :: index of the peak to get.
   * @return a reference to a Peak object.
   */
  virtual IPeak &getPeak(int peakNum) = 0;

  //---------------------------------------------------------------------------------------------
  /** Return a reference to the Peak (const version)
   * @param peakNum :: index of the peak to get.
   * @return a reference to a Peak object.
   */
  virtual const IPeak &getPeak(int peakNum) const = 0;

  //---------------------------------------------------------------------------------------------
  /** Return a pointer to the Peak
   * @param peakNum :: index of the peak to get.
   * @return a pointer to a Peak object.
   */
  IPeak *getPeakPtr(const int peakNum) { return &this->getPeak(peakNum); }

  //---------------------------------------------------------------------------------------------
  /** Create an instance of a Peak
   * @param QLabFrame :: Q of the center of the peak, in reciprocal space
   * @param detectorDistance :: distance between the sample and the detector.
   * @return a pointer to a new Peak object.
   */
  virtual IPeak *createPeak(Mantid::Kernel::V3D QLabFrame,
                            double detectorDistance = 1.0) const = 0;

  //---------------------------------------------------------------------------------------------
  /** Determine if the workspace has been integrated using a peaks integration
   * algorithm.
   * @return TRUE if the workspace has been integrated.
   */
  virtual bool hasIntegratedPeaks() const = 0;

  //---------------------------------------------------------------------------------------------
  /**
   * Creates a new TableWorkspace giving the IDs of the detectors that
   * contribute to the
   * peak.
   * @returns A shared pointer to a TableWorkspace containing the information
   */
  virtual API::ITableWorkspace_sptr createDetectorTable() const = 0;

  //---------------------------------------------------------------------------------------------
  /**
   * Set the special coordinate system.
   * @param coordinateSystem : Special Q3D coordinate system to use.
   */
  virtual void setCoordinateSystem(
      const Mantid::Kernel::SpecialCoordinateSystem coordinateSystem) = 0;

  //---------------------------------------------------------------------------------------------
  /**
   * Get the special coordinate system.
   * @returns special Q3D coordinate system to use being used by this
   * PeaksWorkspace object. Probably the one the workspace was generated with.
   */
  virtual Mantid::Kernel::SpecialCoordinateSystem
  getSpecialCoordinateSystem() const = 0;
  virtual std::vector<std::pair<std::string, std::string>>
  peakInfo(Kernel::V3D QFrame, bool labCoords) const = 0;
  virtual int peakInfoNumber(Kernel::V3D qLabFrame, bool labCoords) const = 0;

protected:
  virtual const std::string toString() const;
};

/// Typedef for a shared pointer to a peaks workspace.
typedef boost::shared_ptr<IPeaksWorkspace> IPeaksWorkspace_sptr;

/// Typedef for a shared pointer to a const peaks workspace.
typedef boost::shared_ptr<const IPeaksWorkspace> IPeaksWorkspace_const_sptr;
}
}
#endif
