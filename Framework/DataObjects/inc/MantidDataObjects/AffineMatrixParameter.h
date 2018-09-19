#ifndef MANTID_DATAOBJECTS_AFFINE_MATRIX_PARAMETER
#define MANTID_DATAOBJECTS_AFFINE_MATRIX_PARAMETER

#include "MantidAPI/ImplicitFunctionParameter.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {
/// Convenience typedef for a specific matrix type.
using AffineMatrixType = Mantid::Kernel::Matrix<coord_t>;

/** Type to wrap an affine matrix and allow serialization via xml.
 *
 * @author Owen Arnold
 * @date 20/07/2011
 */
class DLLExport AffineMatrixParameter
    : public Mantid::API::ImplicitFunctionParameter {
public:
  // ImplcitFunctionParameter Methods.
  std::string getName() const override;
  bool isValid() const override;
  std::string toXMLString() const override;
  AffineMatrixParameter *clone() const override;
  void setMatrix(const AffineMatrixType newMatrix);
  AffineMatrixParameter(size_t outD, size_t inD);
  AffineMatrixParameter(const AffineMatrixParameter &);
  AffineMatrixParameter &operator=(const AffineMatrixParameter &other);
  ~AffineMatrixParameter() override;

  coord_t **getRawMatrix();
  AffineMatrixType getAffineMatrix() const;

  /**
   * Gets the type parameter name.
   * @return parameter name.
   */
  static std::string parameterName() { return "AffineMatrixParameter"; }

private:
  void copyRawMatrix();

  /// Raw matrix used for speed (array of pointers to columns).
  coord_t **m_rawMatrix;
  /// pointer to large memory block (matrix)
  coord_t *m_rawMem;

  /// Affine matrix.
  AffineMatrixType m_affineMatrix;
};
} // namespace DataObjects
} // namespace Mantid

#endif
