#include "MantidKernel/VMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace Kernel
{

/**
  Prints a text representation of itself
  @param os :: the Stream to output to
  @param v :: the vector to output
  @return the output stream
  */
std::ostream& operator<<(std::ostream& os, const VMD& v)
{
  os << v.toString();
  return os;
}

/** Make an orthogonal system with 2 input 3D vectors.
 * Currently only works in 3D!
 *
 * @param vectors :: list of 2 vectors with 3D
 * @return list of 3 vectors
 */
std::vector<VMD> VMD::makeVectorsOrthogonal(std::vector<VMD> & vectors)
{
  if (vectors.size() != 2)
    throw std::runtime_error("VMD::makeVectorsOrthogonal(): Need 2 input vectors.");
  if (vectors[0].getNumDims() != 3 || vectors[1].getNumDims() != 3)
    throw std::runtime_error("VMD::makeVectorsOrthogonal(): Need 3D input vectors.");
  std::vector<V3D> in, out;
  for (size_t i=0; i<vectors.size(); i++)
    in.push_back( V3D( vectors[i][0], vectors[i][1], vectors[i][2]) );
  out = V3D::makeVectorsOrthogonal(in);

  std::vector<VMD> retVal;
  for (size_t i=0; i<out.size(); i++)
    retVal.push_back( VMD(out[i]) );
  return retVal;
}

} // namespace Mantid
} // namespace Kernel

