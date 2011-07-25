#include "MantidMDEvents/CoordTransform.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor.
   * Construct the affine matrix to and initialize to an identity matrix.
   * @param inD :: input number of dimensions, >= 1
   * @param outD :: output number of dimensions, <= inD
   * @throw std::runtime_error if outD > inD
   */
  CoordTransform::CoordTransform(const size_t inD, const size_t outD)
  : inD(inD), outD(outD), affineMatrixParameter(outD, inD)
  {
    if (outD > inD)
      throw std::runtime_error("CoordTransform: Cannot have more output dimensions than input dimensions!");
    if (outD == 0)
      throw std::runtime_error("CoordTransform: invalid number of output dimensions!");
    if (inD == 0)
      throw std::runtime_error("CoordTransform: invalid number of input dimensions!");
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CoordTransform::~CoordTransform()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Directly set the affine matrix to use.
   *
   * @param newMatrix :: (outD+1 * inD+1) matrix to set.
   * @throw runtime_error if the matrix dimensions are incompatible.
   */
  void CoordTransform::setMatrix(const Mantid::Kernel::Matrix<coord_t> newMatrix)
  {
    affineMatrixParameter.setMatrix(newMatrix);
  }


  //----------------------------------------------------------------------------------------------
  /** Return the affine matrix in the transform.
   */
  Mantid::Kernel::Matrix<coord_t> CoordTransform::getMatrix() const
  {
    return affineMatrixParameter.getAffineMatrix();
  }

  //----------------------------------------------------------------------------------------------
  /** Add a translation (in the output coordinates) to the transform.
   *
   * @param translationVector :: fixed-size array of the translation vector, of size outD
   */
  void CoordTransform::addTranslation(const coord_t * translationVector)
  {
    Matrix<coord_t> translationMatrix(outD.getValue()+1, inD.getValue()+1);
    // Start with identity
    translationMatrix.identityMatrix();
    // Fill the last column with the translation value
    for (size_t i=0; i < outD.getValue(); i++)
      translationMatrix[i][inD.getValue()] = translationVector[i];

    // Multiply the affine matrix by the translation affine matrix to combine them
    Matrix<coord_t> currentAffine = affineMatrixParameter.getAffineMatrix();
    currentAffine *= translationMatrix;

    affineMatrixParameter.setMatrix(currentAffine);
  }

  //----------------------------------------------------------------------------------------------
  /** Serialize the coordinate transform
  *
  * @return The coordinate transform in its serialized form.
  */
  std::string CoordTransform::toXMLString() const
  {
     using namespace Poco::XML;

      AutoPtr<Document> pDoc = new Document;
      AutoPtr<Element> coordTransformElement = pDoc->createElement("CoordTransform");
      pDoc->appendChild(coordTransformElement);

      AutoPtr<Element> coordTransformTypeElement = pDoc->createElement("Type");
      coordTransformTypeElement->appendChild(pDoc->createTextNode("CoordTransform"));
      coordTransformElement->appendChild(coordTransformTypeElement);

      AutoPtr<Element> paramListElement = pDoc->createElement("ParameterList");

      AutoPtr<Text> formatText = pDoc->createTextNode("%s%s%s");
      paramListElement->appendChild(formatText);

      coordTransformElement->appendChild(paramListElement);

      std::stringstream xmlstream;

      DOMWriter writer;
      writer.writeNode(xmlstream, pDoc);

      std::string formattedXMLString = boost::str(boost::format(xmlstream.str().c_str())
        % inD.toXMLString().c_str() % outD.toXMLString().c_str() % affineMatrixParameter.toXMLString().c_str());
      return formattedXMLString;
  }

//  //----------------------------------------------------------------------------------------------
//  /** Set the transformation to be a simple rotation.
//   *
//   * @param translationVector :: fixed-size array of the translation vector, of size inD
//   * @throw runtime_error if inD != outD
//   */
//  TCT
//  void CoordTransform::setRotation(const coord_t * translationVector)
//  {
//    if (inD != outD) throw std::runtime_error("Translation required inD == outD.");
//  }





} // namespace Mantid
} // namespace MDEvents

