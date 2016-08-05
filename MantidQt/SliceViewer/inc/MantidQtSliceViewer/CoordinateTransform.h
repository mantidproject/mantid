#ifndef MANTIDQT_SLICEVIEWER_COORDINATETRANSFORM_H
#define MANTIDQT_SLICEVIEWER_COORDINATETRANSFORM_H

#include "MantidKernel/VMD.h"
#include "MantidAPI/IMDWorkspace.h"
#include <memory>

namespace MantidQt {
namespace SliceViewer {
	class CoordinateTransform {
	public:
		virtual ~CoordinateTransform() {};
		virtual void transform(Mantid::Kernel::VMD &coords, size_t dimX, size_t dimY) = 0;
		virtual void checkDimensionsForHKL(Mantid::API::IMDWorkspace_sptr ws, size_t dimX, size_t dimY) = 0;

	};

	class NullTransform : public CoordinateTransform {
	public:
		~NullTransform();
		void transform(Mantid::Kernel::VMD &coords, size_t dimX, size_t dimY) override;
		void checkDimensionsForHKL(Mantid::API::IMDWorkspace_sptr ws, size_t dimX, size_t dimY) override;
	};

	class NonOrthogonalTransform : public CoordinateTransform {
	public:
		~NonOrthogonalTransform();
		NonOrthogonalTransform(Mantid::API::IMDWorkspace_sptr ws, size_t dimX, size_t dimY);
		void transform(Mantid::Kernel::VMD &coords, size_t dimX, size_t dimY) override;
		void checkDimensionsForHKL(Mantid::API::IMDWorkspace_sptr ws, size_t dimX, size_t dimY) override;

	private:
		Mantid::coord_t m_skewMatrix[9];
		bool m_dimensionsHKL;
	};

	std::unique_ptr<CoordinateTransform> createCoordinateTransform(Mantid::API::IMDWorkspace_sptr ws, size_t dimX, size_t dimY);
}
}
#endif 
