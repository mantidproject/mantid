#include "MantidDataObjects/CoordTransformAligned.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Matrix.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataObjects {

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param inD :: input number of dimensions, >= 1
 * @param outD :: output number of dimensions, <= inD
 * @param dimensionToBinFrom :: For each dimension in the output, index in the
 *input workspace of which dimension it is
 * @param origin :: Offset (minimum) position in each of the output dimensions,
 *sized [outD]
 * @param scaling :: Scaling from the input to the output dimension, sized
 *[outD]
 * @throw std::runtime_error if outD > inD or if an invalid index is in
 *dimensionToBinFrom
 *
 */
CoordTransformAligned::CoordTransformAligned(const size_t inD,
                                             const size_t outD,
                                             const size_t *dimensionToBinFrom,
                                             const coord_t *origin,
                                             const coord_t *scaling)
    : CoordTransform(inD, outD) {
  if (!origin || !scaling || !dimensionToBinFrom)
    throw std::runtime_error("CoordTransformAligned::ctor(): at least one of "
                             "the input arrays is a NULL pointer.");
  m_dimensionToBinFrom = new size_t[outD];
  m_origin = new coord_t[outD];
  m_scaling = new coord_t[outD];
  for (size_t d = 0; d < outD; d++) {
    m_dimensionToBinFrom[d] = dimensionToBinFrom[d];
    if (m_dimensionToBinFrom[d] >= inD) {
      delete [] m_dimensionToBinFrom;
      delete [] m_origin;
      delete [] m_scaling;
      throw std::runtime_error(
          "CoordTransformAligned::ctor(): invalid entry in "
          "dimensionToBinFrom[" +
          Mantid::Kernel::Strings::toString(d) +
          "]. Cannot build the coordinate transformation.");
    }
    m_origin[d] = origin[d];
    m_scaling[d] = scaling[d];
  }
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param inD :: input number of dimensions, >= 1
 * @param outD :: output number of dimensions, <= inD
 * @param dimensionToBinFrom :: For each dimension in the output, index in the
 *input workspace of which dimension it is
 * @param origin :: Offset (minimum) position in each of the output dimensions,
 *sized [outD]
 * @param scaling :: Scaling from the input to the output dimension, sized
 *[outD]
 * @throw std::runtime_error if outD > inD or if an invalid index is in
 *dimensionToBinFrom
 *
 */
CoordTransformAligned::CoordTransformAligned(
    const size_t inD, const size_t outD,
    const std::vector<size_t> dimensionToBinFrom,
    const std::vector<coord_t> origin, const std::vector<coord_t> scaling)
    : CoordTransform(inD, outD) {
  if (dimensionToBinFrom.size() != outD || origin.size() != outD ||
      scaling.size() != outD)
    throw std::runtime_error("CoordTransformAligned::ctor(): at least one of "
                             "the input vectors is the wrong size.");
  m_dimensionToBinFrom = new size_t[outD];
  m_origin = new coord_t[outD];
  m_scaling = new coord_t[outD];
  for (size_t d = 0; d < outD; d++) {
    m_dimensionToBinFrom[d] = dimensionToBinFrom[d];
    if (m_dimensionToBinFrom[d] >= inD) {
      delete [] m_dimensionToBinFrom;
      delete [] m_origin;
      delete [] m_scaling;
      throw std::runtime_error(
          "CoordTransformAligned::ctor(): invalid entry in "
          "dimensionToBinFrom[" +
          Mantid::Kernel::Strings::toString(d) +
          "]. Cannot build the coordinate transformation.");
    }
    m_origin[d] = origin[d];
    m_scaling[d] = scaling[d];
  }
}

//----------------------------------------------------------------------------------------------
/** Virtual cloner
 * @return a copy of this object  */
CoordTransform *CoordTransformAligned::clone() const {
  CoordTransformAligned *out = new CoordTransformAligned(
      inD, outD, m_dimensionToBinFrom, m_origin, m_scaling);
  return out;
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CoordTransformAligned::~CoordTransformAligned() {
  delete[] m_dimensionToBinFrom;
  delete[] m_origin;
  delete[] m_scaling;
}

//----------------------------------------------------------------------------------------------
/** Apply the coordinate transformation
 *
 * @param inputVector :: fixed-size array of input coordinates, of size inD
 * @param outVector :: fixed-size array of output coordinates, of size outD
 */
void CoordTransformAligned::apply(const coord_t *inputVector,
                                  coord_t *outVector) const {
  // For each output dimension
  for (size_t out = 0; out < outD; ++out) {
    // Get the coordinate at the dimension in the INPUT workspace corresponding
    // to this OUTPUT
    coord_t x = inputVector[m_dimensionToBinFrom[out]];
    // Convert by taking the origin out then scaling.
    outVector[out] = (x - m_origin[out]) * m_scaling[out];
  }
}

//----------------------------------------------------------------------------------------------
/** Create an equivalent affine transformation matrix out of the
 * parameters of this axis-aligned transformation.
 *
 * The output can be used to merge CoordTransforms together.
 *
 * @return An affine matrix matrix with inD+1 columns, outD+1 rows.
 */
Mantid::Kernel::Matrix<coord_t>
CoordTransformAligned::makeAffineMatrix() const {
  // Zeroed-out affine matrix
  Matrix<coord_t> mat(outD + 1, inD + 1);
  // Bottom-right corner of the matrix is always 1.
  mat[outD][inD] = 1.0;
  // For each output dimension
  for (size_t out = 0; out < outD; ++out) {
    // The ROW is the out dimension.
    size_t row = out;
    // The COLUMN is the input dimension
    size_t col = m_dimensionToBinFrom[out];
    // So place the scaling factor at that spot
    mat[row][col] = m_scaling[out];
    // And place the origin (translation amount) at the last column
    mat[row][inD] = -m_origin[out] * m_scaling[out];
  }
  return mat;
}

//----------------------------------------------------------------------------------------------
/** Serialize the coordinate transform
*
* @return The coordinate transform in its serialized form.
*/
std::string CoordTransformAligned::toXMLString() const {
  using namespace Poco::XML;

  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> coordTransformElement =
      pDoc->createElement("CoordTransform");
  pDoc->appendChild(coordTransformElement);

  AutoPtr<Element> coordTransformTypeElement = pDoc->createElement("Type");
  coordTransformTypeElement->appendChild(
      pDoc->createTextNode("CoordTransformAffine"));
  coordTransformElement->appendChild(coordTransformTypeElement);

  AutoPtr<Element> paramListElement = pDoc->createElement("ParameterList");

  AutoPtr<Text> formatText = pDoc->createTextNode("%s%s%s");
  paramListElement->appendChild(formatText);

  coordTransformElement->appendChild(paramListElement);

  std::stringstream xmlstream;

  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  // Convert the members to parameters
  InDimParameter inD_param(inD);
  OutDimParameter outD_param(outD);

  std::string formattedXMLString = boost::str(
      boost::format(xmlstream.str().c_str()) % inD_param.toXMLString().c_str() %
      outD_param.toXMLString().c_str());
  return formattedXMLString;
}

/**
 * Coordinate transform id
 * @return the type of coordinate transform
 */
std::string CoordTransformAligned::id() const {
  return "CoordTransformAligned";
}

} // namespace Mantid
} // namespace DataObjects
