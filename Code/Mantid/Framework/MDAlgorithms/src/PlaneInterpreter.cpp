#include "MantidMDAlgorithms/PlaneInterpreter.h"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"

namespace Mantid
{
namespace MDAlgorithms
{

planeVector PlaneInterpreter::walkTree(CompositeImplicitFunction* compFunc) const
{
  using namespace Mantid::API;
  planeVector flattenedboxes;
  functionVector nestedFuncs = compFunc->getFunctions();
  for (unsigned int i = 0; i < nestedFuncs.size(); i++)
  {
    if (CompositeImplicitFunction::functionName() == nestedFuncs[i]->getName())
    {
      CompositeImplicitFunction* composite = dynamic_cast<CompositeImplicitFunction*>(nestedFuncs[i].get());
      planeVector boxes = walkTree(composite); //recursive walk
      flattenedboxes.insert(flattenedboxes.end(), boxes.begin(), boxes.end());
    }
    else if (PlaneImplicitFunction::functionName() == nestedFuncs[i]->getName())
    {
      boost::shared_ptr<PlaneImplicitFunction> spPlane =
          boost::static_pointer_cast<PlaneImplicitFunction, Mantid::API::ImplicitFunction>(nestedFuncs[i]);
      flattenedboxes.push_back(spPlane);
    }
  }
  return flattenedboxes;
}

std::vector<double> PlaneInterpreter::defaultRotationMatrix() const
{
  std::vector<double> identityMatrix(9, 0);
  identityMatrix[0] = 1;
  identityMatrix[4] = 1;
  identityMatrix[8] = 1;
  return identityMatrix;
}

std::vector<double> PlaneInterpreter::operator()(Mantid::API::ImplicitFunction* implicitFunction) const
{
  return Execute(implicitFunction);
}

std::vector<double> PlaneInterpreter::Execute(Mantid::API::ImplicitFunction* implicitFunction) const
{
  //A rotation matrix is by default an identity matrix.
  std::vector<double> rotationMatrix = defaultRotationMatrix();

  CompositeImplicitFunction* compFunction =
      dynamic_cast<Mantid::MDAlgorithms::CompositeImplicitFunction*> (implicitFunction);

  if (compFunction != NULL)
  {
    //Flatten out box functions
    planeVector flattenedPlanes = walkTree(compFunction);

    int size = flattenedPlanes.size();

    //Only if there are any plane functions at all, get the last plane's rotation matrix.
    if (size > 0)
    {
      //Use the last defined plane.
      boost::shared_ptr<PlaneImplicitFunction> planeFunction = flattenedPlanes[size - 1];
      //Get the rotation matrix from the last defined plane.
      rotationMatrix = planeFunction->asRotationMatrixVector();
    }
  }

  return rotationMatrix;
}

planeVector PlaneInterpreter::getAllPlanes(Mantid::API::ImplicitFunction* implicitFunction) const
{
  CompositeImplicitFunction* compFunction =
      dynamic_cast<Mantid::MDAlgorithms::CompositeImplicitFunction*> (implicitFunction);

  planeVector flattenedPlanes;
  if (compFunction != NULL)
  {
    //Flatten out box functions
    flattenedPlanes = walkTree(compFunction);
  }
  return flattenedPlanes;
}


}
}
