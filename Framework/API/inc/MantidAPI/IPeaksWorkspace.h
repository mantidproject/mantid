// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <optional>

namespace Mantid {
namespace Geometry {
class IPeak;
}
namespace API {

//==========================================================================================
/** Interface to the class Mantid::DataObjects::PeaksWorkspace

    The class PeaksWorkspace stores information about a set of SCD peaks.

    @author Ruth Mikkelson, SNS ORNL
    @date 3/10/2010
 */
class MANTID_API_DLL IPeaksWorkspace : public ITableWorkspace, public Mantid::API::ExperimentInfo {
public:
  /// Ctor
  IPeaksWorkspace()
      : ITableWorkspace(), ExperimentInfo(), m_convention(Kernel::ConfigService::Instance().getString("Q.convention")) {
  }

  /// Returns a clone of the workspace
  IPeaksWorkspace_uptr clone() const { return IPeaksWorkspace_uptr(doClone()); }
  IPeaksWorkspace &operator=(const IPeaksWorkspace &other) = delete;
  //---------------------------------------------------------------------------------------------
  /** @return the number of peaks
   */
  virtual int getNumberPeaks() const = 0;

  //---------------------------------------------------------------------------------------------
  /** @return the number of peaks
   */
  virtual std::string getConvention() const = 0;

  //---------------------------------------------------------------------------------------------
  /** Removes the indicated peak
   * @param peakNum  the peak to remove. peakNum starts at 0
   */
  virtual void removePeak(int peakNum) = 0;

  virtual void removePeaks(std::vector<int> badPeaks) = 0;

  //---------------------------------------------------------------------------------------------
  /** Add a peak to the list
   * @param ipeak :: Peak object to add (copy) into this.
   */
  virtual void addPeak(const Mantid::Geometry::IPeak &ipeak) = 0;

  //---------------------------------------------------------------------------------------------
  /** Add a peak to the list
   * @param position :: V3D positon of the peak.
   * @param frame :: Coordinate system frame of the peak position.
   */
  virtual void addPeak(const Kernel::V3D &position, const Kernel::SpecialCoordinateSystem &frame) = 0;

  //---------------------------------------------------------------------------------------------
  /** Return a reference to the Peak
   * @param peakNum :: index of the peak to get.
   * @return a reference to a Peak object.
   */
  virtual Mantid::Geometry::IPeak &getPeak(size_t const peakNum) = 0;

  //---------------------------------------------------------------------------------------------
  /** Return a reference to the Peak (const version)
   * @param peakNum :: index of the peak to get.
   * @return a reference to a Peak object.
   */
  virtual const Mantid::Geometry::IPeak &getPeak(size_t const peakNum) const = 0;

  //---------------------------------------------------------------------------------------------
  /** Return a pointer to the Peak
   * @param peakNum :: index of the peak to get.
   * @return a pointer to a Peak object.
   */
  Mantid::Geometry::IPeak *getPeakPtr(size_t const peakNum) { return &this->getPeak(peakNum); }

  //---------------------------------------------------------------------------------------------
  /** Create an instance of a Peak
   * @param QLabFrame :: Q of the center of the peak in the lab frame, in
   * reciprocal space
   * @param detectorDistance :: Optional distance between the sample and the
   * detector. Calculated if not provided.
   * @return a pointer to a new Peak object.
   */
  virtual std::unique_ptr<Geometry::IPeak> createPeak(const Mantid::Kernel::V3D &QLabFrame,
                                                      std::optional<double> detectorDistance = std::nullopt) const = 0;

  //---------------------------------------------------------------------------------------------
  /** Create an instance of a Peak
   * @param position :: enter of the peak in the specified frame
   * @param frame :: the coordinate frame that the position is specified in.
   * @return a pointer to a new Peak object.
   */
  virtual std::unique_ptr<Mantid::Geometry::IPeak>
  createPeak(const Mantid::Kernel::V3D &position, const Mantid::Kernel::SpecialCoordinateSystem &frame) const = 0;

  //---------------------------------------------------------------------------------------------
  /** Create an instance of a Peak
   * @param position :: enter of the peak in the sample frame
   * @return a pointer to a new Peak object.
   */
  virtual std::unique_ptr<Mantid::Geometry::IPeak> createPeakQSample(const Mantid::Kernel::V3D &position) const = 0;

  /**
   * Create an instance of a peak using a V3D
   * @param HKL V3D
   * @return a pointer to a new Peak object.
   */
  virtual std::unique_ptr<Geometry::IPeak> createPeakHKL(const Mantid::Kernel::V3D &HKL) const = 0;

  /**
   * Create an instance of a peak using default constructor
   * @return a pointer to a new Peak object.
   */
  virtual std::unique_ptr<Geometry::IPeak> createPeak() const = 0;

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
  virtual void setCoordinateSystem(const Kernel::SpecialCoordinateSystem coordinateSystem) = 0;
  //---------------------------------------------------------------------------------------------
  /**
   * Get the special coordinate system.
   * @returns special Q3D coordinate system to use being used by this
   * PeaksWorkspace object. Probably the one the workspace was generated with.
   */
  virtual Kernel::SpecialCoordinateSystem getSpecialCoordinateSystem() const = 0;

  virtual std::vector<std::pair<std::string, std::string>> peakInfo(const Kernel::V3D &QFrame,
                                                                    bool labCoords) const = 0;
  virtual int peakInfoNumber(const Kernel::V3D &qLabFrame, bool labCoords) const = 0;

  //---------------------------------------------------------------------------------------------
  virtual void saveNexus(::NeXus::File *file) const = 0;

  std::string m_convention;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  IPeaksWorkspace(const IPeaksWorkspace &) = default;

  const std::string toString() const override;

private:
  IPeaksWorkspace *doClone() const override = 0;
};
} // namespace API
} // namespace Mantid
