#define _USE_MATH_DEFINES
#include "MantidVatesAPI/vtkEllipsoidTransformer.h"
#include "MantidKernel/V3D.h"

#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <cmath>

namespace Mantid
{
namespace VATES
{
  vtkEllipsoidTransformer::vtkEllipsoidTransformer(){}

  vtkEllipsoidTransformer::~vtkEllipsoidTransformer(){}

  /**
  * Generates a transform based on the directions of the ellipsoid
  * @param directions The directions of the ellipsoid.
  * @returns A transform for the ellipsoid.
  */
  vtkSmartPointer<vtkTransform> vtkEllipsoidTransformer::generateTransform(std::vector<Mantid::Kernel::V3D> directions)
  {
    // The original ellipsoid is set to have its principal axis along the x axis and the first minor axis along the y axis.
    Mantid::Kernel::V3D principalAxisOriginal(1.0, 0.0, 0.0);
    Mantid::Kernel::V3D principalAxisTransformed(directions[0]);
    Mantid::Kernel::V3D minorAxisOriginal(0.0, 1.0, 0.0);
    Mantid::Kernel::V3D minorAxisTransformed(directions[1]);

    // Compute the axis of rotation. This is the normal between the original and the transformed direction
    Mantid::Kernel::V3D rotationAxis1 = principalAxisOriginal.cross_prod(principalAxisTransformed);
    rotationAxis1 = rotationAxis1/rotationAxis1.norm();

    // Compute the angle of rotation, i.e. the angle between the original and the transformed axis.
    double angle1 = acos(principalAxisOriginal.scalar_prod(principalAxisTransformed)
                    /principalAxisOriginal.norm()/principalAxisTransformed.norm());

    // After the prinicpal axis is rotated into its right position we need to rotate the (rotated) minor axis
    // into its right position. The rotation axis is given by the new prinicipal rotation axis
    Mantid::Kernel::V3D minorAxisOriginalRotated(rotateVector(minorAxisOriginal, rotationAxis1, angle1));

    Mantid::Kernel::V3D rotationAxis2(minorAxisOriginalRotated.cross_prod(minorAxisTransformed));
    rotationAxis2 = rotationAxis2/rotationAxis2.norm();
    double angle2 = acos(minorAxisOriginalRotated.scalar_prod(minorAxisTransformed)
                     /minorAxisOriginalRotated.norm()/minorAxisTransformed.norm());

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();

    double angle1Degree = angle1*180/M_PI;
    double angle2Degree = angle2*180/M_PI;

    // The total transform is T = T_rot2*T_rot1. Note that we need to add the last operation first!
    transform->RotateWXYZ(angle2Degree, rotationAxis2[0], rotationAxis2[1], rotationAxis2[2]);
    transform->RotateWXYZ(angle1Degree, rotationAxis1[0], rotationAxis1[1], rotationAxis1[2]);

    return transform;
  }

  /**
  * Rotate the a given vector around a specified axis by a specified angle. See http://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
  * @param original The original vector
  * @param rotationAxis The axis around which to rotate.
  * @param angle The rotation angle.
  * @returns The rotated vector.
  */
  Mantid::Kernel::V3D vtkEllipsoidTransformer::rotateVector(Mantid::Kernel::V3D original, Mantid::Kernel::V3D rotationAxis, double angle)
  {
    Mantid::Kernel::V3D cross(rotationAxis.cross_prod(original));
    double scalar = rotationAxis.scalar_prod(original);
    double cos = std::cos(angle);
    double sin = std::sin(angle);

    Mantid::Kernel::V3D rotated = original*cos + cross*sin + rotationAxis*(scalar)*(1-cos);

    return rotated;
  }
}
}