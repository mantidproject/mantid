// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY NOT be included in any test from a package below Geometry
 *    (e.g. Kernel).
 *  Conversely, this file (and its cpp) MAY NOT be modified to use anything from
 *a
 *  package higher than Geometry (e.g. API, DataObjects, ...)
 *********************************************************************************/
#ifndef COMPONENTCREATIONHELPER_H_
#define COMPONENTCREATIONHELPER_H_

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/V3D.h"
#include <memory>

// Forward declarations
namespace Mantid {
namespace Kernel {
class Quat;
}
namespace Geometry {
class CompAssembly;
class ObjComponent;
class DetectorGroup;
class DetectorsRing;
class IDetector;
class Instrument;
} // namespace Geometry
} // namespace Mantid

namespace ComponentCreationHelper {

/**
A set of helper functions for creating various component structures for the unit
tests.
*/

//----------------------------------------------------------------------------------------------
/**
 * Return the appropriate XML for the requested cylinder
 */
std::string cappedCylinderXML(double radius, double height,
                              const Mantid::Kernel::V3D &baseCentre,
                              const Mantid::Kernel::V3D &axis,
                              const std::string &id);

/**
 * Create a capped cylinder object
 */
boost::shared_ptr<Mantid::Geometry::CSGObject>
createCappedCylinder(double radius, double height,
                     const Mantid::Kernel::V3D &baseCentre,
                     const Mantid::Kernel::V3D &axis, const std::string &id);

/// Add a spherical sample at samplePos to given instrument.
void addSampleToInstrument(Mantid::Geometry::Instrument_sptr &instrument,
                           const Mantid::Kernel::V3D &samplePos);

/// Add a source with given name and sourcePos to given instrument
void addSourceToInstrument(Mantid::Geometry::Instrument_sptr &instrument,
                           const Mantid::Kernel::V3D &sourcePos,
                           const std::string &name = "moderator");

/**
 * Return the XML for a sphere.
 */
std::string sphereXML(double radius, const Mantid::Kernel::V3D &centre,
                      const std::string &id);
/**
 * Create a sphere object
 */
boost::shared_ptr<Mantid::Geometry::CSGObject>
createSphere(double radius,
             const Mantid::Kernel::V3D &centre = Mantid::Kernel::V3D(),
             const std::string &id = "sp-1");
/** Create a cuboid shape for your pixels */
boost::shared_ptr<Mantid::Geometry::CSGObject>
createCuboid(double x_side_length, double y_side_length = -1.0,
             double z_side_length = -1.0);
/**
 * Create a component assembly at the origin made up of 4 cylindrical detectors
 */
boost::shared_ptr<Mantid::Geometry::CompAssembly>
createTestAssemblyOfFourCylinders();
/**
 * Create an object component that has a defined shape
 */
Mantid::Geometry::ObjComponent *createSingleObjectComponent();
/**
 * Create a hollow shell, i.e. the intersection of two spheres or radius r1 and
 * r2
 */
boost::shared_ptr<Mantid::Geometry::CSGObject>
createHollowShell(double innerRadius, double outerRadius,
                  const Mantid::Kernel::V3D &centre = Mantid::Kernel::V3D());
/**
 * Create a detector group containing 5 detectors
 */
boost::shared_ptr<Mantid::Geometry::DetectorGroup>
createDetectorGroupWith5CylindricalDetectors();
/**
 * Create a detector group containing n detectors with gaps
 */
boost::shared_ptr<Mantid::Geometry::DetectorGroup>
createDetectorGroupWithNCylindricalDetectorsWithGaps(unsigned int nDet = 4,
                                                     double gap = 0.01);
/**
 * Create a detector group containing detectors ring
 * R_min -- min radius of the ring
 * R_max -- max radius of the ring, center has to be in 0 position,
 * z     -- axial z-coordinate of the detectors position;
  The detectors are the cylinders with 1.5cm height and 0.5 cm radius
 */
boost::shared_ptr<Mantid::Geometry::DetectorGroup>
createRingOfCylindricalDetectors(const double R_min = 4.5,
                                 const double R_max = 5,
                                 const double z000000000000000 = 4);

/**
 * Create a detector vector containing detectors ring
 * R_min -- min radius of the ring
 * R_max -- max radius of the ring, center has to be in 0 position,
 * z     -- axial z-coordinate of the detectors position;
  The detectors are the cylinders with 1.5cm height and 0.5 cm radius
 */
std::vector<std::unique_ptr<Mantid::Geometry::IDetector>>
createVectorOfCylindricalDetectors(const double R_min = 4.5,
                                   const double R_max = 5,
                                   const double z000000000000000 = 4);

/**
 * Creates a single flat bank with cylindrical (tubes) of detectors. Vertical y
 * offsets can be used to vertically offset individual tubes.
 * @param nTubes :: number of tubes in the bank
 * @param verticalOffsets :: vertical offsets vector one element per tube. Fixed
 * size to the number of tubes.
 * @param nDetsPerTube :: number of detector pixels per tube
 * @param xMin :: x-min for bank
 * @param xMax :: x-max for bank
 * @param yMin :: y-min for bank (use offsets to shift individual tubes)
 * @param yMax :: y-max for bank (use offsets to shift individual tubes)
 * @return Instrument with single bank as described by parameters.
 */
Mantid::Geometry::Instrument_sptr
createCylInstrumentWithVerticalOffsetsSpecified(
    size_t nTubes, std::vector<double> verticalOffsets, size_t nDetsPerTube,
    double xMin, double xMax, double yMin, double yMax);

/** create instrument with cylindrical detectors located in specific angular
 * positions */
Mantid::Geometry::Instrument_sptr
createCylInstrumentWithDetInGivenPositions(const std::vector<double> &L2,
                                           const std::vector<double> &polar,
                                           const std::vector<double> &azim);
/**
 * Create an test instrument with n panels of 9 cylindrical detectors, a source
 * and spherical sample shape.
 * Detectors have IDs assigned as follows:
 * 7 8 9
 * 4 5 6
 * 1 2 3
 * @param num_banks :: number of 9-cylinder banks to create
 * @param sourcePos :: Position of the source component
 * @param samplePos :: Position of the sample component.
 * @param cylRadius :: radius of each detector
 * @param cylHeight :: height of each detector
 * @return Created instrument
 */
Mantid::Geometry::Instrument_sptr createTestInstrumentCylindrical(
    int num_banks,
    const Mantid::Kernel::V3D &sourcePos = Mantid::Kernel::V3D(0.0, 0.0, -10.),
    const Mantid::Kernel::V3D &samplePos = Mantid::Kernel::V3D(),
    const double cylRadius = 0.004, const double cylHeight = 0.0002);

void addRectangularBank(Mantid::Geometry::Instrument &testInstrument,
                        int idStart, int pixels, double pixelSpacing,
                        std::string bankName,
                        const Mantid::Kernel::V3D &bankPos,
                        const Mantid::Kernel::Quat &bankRot);

/// Create a test instrument with n panels of rectangular detectors,
/// pixels*pixels in size, a source and spherical sample shape.
Mantid::Geometry::Instrument_sptr
createTestInstrumentRectangular(int num_banks, int pixels,
                                double pixelSpacing = 0.008,
                                double bankDistanceFromSample = 5.0);

Mantid::Geometry::Instrument_sptr
createTestInstrumentRectangular2(int num_banks, int pixels,
                                 double pixelSpacing = 0.008);

/// Creates a mimimal valid virtual instrument.
Mantid::Geometry::Instrument_sptr
createMinimalInstrument(const Mantid::Kernel::V3D &sourcePos,
                        const Mantid::Kernel::V3D &samplePos,
                        const Mantid::Kernel::V3D &detectorPos);

Mantid::Geometry::Instrument_sptr
sansInstrument(const Mantid::Kernel::V3D &sourcePos,
               const Mantid::Kernel::V3D &samplePos,
               const Mantid::Kernel::V3D &trolley1Pos,
               const Mantid::Kernel::V3D &trolley2Pos);

Mantid::Geometry::Instrument_sptr
createInstrumentWithPSDTubes(const size_t nTubes = 3,
                             const size_t nPixelsPerTube = 50,
                             const bool mirrorTubes = false);
} // namespace ComponentCreationHelper

#endif // COMPONENTCREATIONHELPERS_H_
