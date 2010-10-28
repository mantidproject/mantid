#include "MantidDataHandling/SavePHX.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Objects/Object.h"

#include <cstdio>
#include <fstream>
#include <iomanip>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM( SavePHX);

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void SavePHX::init() {
	declareProperty(new WorkspaceProperty<> ("InputWorkspace", "",
			Direction::Input), "The input workspace");
	declareProperty(new FileProperty("Filename", "", FileProperty::Save),
			"The filename to use for the saved data");

}

void SavePHX::exec() {
	const double rad2deg = 180.0 / M_PI;

	// Get the input workspace
	MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

	// Get the sample position
	const Geometry::V3D samplePos =
			inputWorkspace->getInstrument()->getSample()->getPos();

	// Retrieve the filename from the properties
	const std::string filename = getProperty("Filename");

	// Get the number of spectra
	const int nHist = inputWorkspace->getNumberHistograms();

	// Get a pointer to the sample
	IObjComponent_const_sptr sample =
			inputWorkspace->getInstrument()->getSample();

	std::ofstream outPHX_file(filename.c_str());

	if (!outPHX_file) {
		g_log.error("Failed to open (PHX) file:" + filename);
		throw Kernel::Exception::FileError("Failed to open (PHX) file:",
				filename);
	}

	double twoTheta = 0.0;
	double phi = 0.0;
	double delta_twoTheta = 0.0;
	double delta_phi = 0.0;
	int nDetectors = 0;

	for (int i = 0; i < nHist; ++i) {
		// Now get the detector object for this histogram
		IDetector_sptr det = inputWorkspace->getDetector(i);

		//    boost::shared_ptr<GeometryHandler> handler;
		//    handler = const_cast<Object>(shape)->getGeometryHandler();
		//
		//    handler->GetObjectGeom(type, geometry_vectors, radius, height);


		if (!det->isMonitor()) {

			++nDetectors;

			// Get a pointer to the object.
			//      const boost::shared_ptr<const Object> shape = det->Shape();

			// What is the distance from the sample to the detector (L2) ?
			double distance = det->getDistance(*sample);

			// Get the scattering angle (Polar) for this detector (in radians).
			twoTheta = inputWorkspace->detectorTwoTheta(det);
			// And also the Azimuthal angle (also in radians)
			phi = det->getPhi();

			// Convert to degrees
			twoTheta = twoTheta * rad2deg;
			phi = phi * rad2deg;

			// int type(0);
//			std::vector<Mantid::Geometry::V3D> geometry_vectors(0);
//			double height(0.0), radius(0.0);

			// shape->GetObjectGeom(type, geometry_vectors, radius, height);

			//        std::cout << "Pixel 1: " << std::endl;
			//        std::cout << "Height = " << height << std::endl;
			//        std::cout << "Radius = " << radius << std::endl;

			// Initialise to large values
			double xmin = -1000.0;
			double xmax = 1000.0;
			double ymin = -1000.0;
			double ymax = 1000.0;
			double zmin = -1000.0;
			double zmax = 1000.0;

			// Get the bounding box
			det->getBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);

			//            g_log.information("X:(" + boost::lexical_cast<std::string>(xmin) + "," + boost::lexical_cast<
			//                std::string>(xmax) + ")");
			//            g_log.information("Y:(" + boost::lexical_cast<std::string>(ymin) + "," + boost::lexical_cast<
			//                std::string>(ymax) + ")");
			//            g_log.information("Z:(" + boost::lexical_cast<std::string>(zmin) + "," + boost::lexical_cast<
			//                std::string>(zmax) + ")");

			double xsize = xmax - xmin;
			double ysize = ymax - ymin;
			double zsize = zmax - zmin;

			g_log.debug() << "L2     : " << distance << std::endl;
			g_log.debug() << "Width  : " << xsize << std::endl;
			g_log.debug() << "Height : " << ysize << std::endl;
			g_log.debug() << "Depth  : " << zsize << std::endl;

			//      IObjComponent_sptr detcomp = det->getComponent();
			//      Object shape = detcomp->Shape();

			//            double theta_min = acos(zmin/sqrt((xmin*xmin)+(ymin*ymin)+(zmin*zmin)));
			//            double theta_max = acos(zmax/sqrt((xmax*xmax)+(ymax*ymax)+(zmax*zmax)));
			//
			//            double phi_min = atan2(ymin,xmin);
			//            double phi_max = atan2(ymax,xmax);
			//
			//            delta_twoTheta = ((theta_max-theta_min)*(180.0/PI));
			//            delta_phi = ((phi_max-phi_min)*(180.0/PI));
			//
			//      delta_twoTheta = atan2(xsize,distance)*(180.0/PI);
			//      delta_phi = atan2(ysize,distance)*(180.0/PI);

			//    ang=(y>0)?atan2(y,x):2.*M_PI+atan2(y,x);

			//      delta_twoTheta = comp->getPos();

			delta_phi = atan2((ysize / 2.0), distance) * rad2deg;
			delta_twoTheta = atan2((xsize / 2.0), distance) * rad2deg;

			// Write the number of detectors to the file.
			outPHX_file << nDetectors << std::endl;
			// Now write all the detector info.
			outPHX_file << std::fixed << std::setprecision(3);
			outPHX_file << "1\t0\t" << twoTheta << "\t" << phi << "\t"
					<< delta_twoTheta << "\t" << delta_phi << "\t0\t"
					<< det->getID() << std::endl;
		}
	}

	// Close the file
	outPHX_file.close();
}

} // namespace DataHandling
} // namespace Mantid
