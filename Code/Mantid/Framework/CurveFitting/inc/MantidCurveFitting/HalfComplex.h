#ifndef MANTID_CURVEFITTING_HALFCOMPLEX_H_
#define MANTID_CURVEFITTING_HALFCOMPLEX_H_

namespace Mantid {
namespace CurveFitting {

/**
 * Class for helping to read the transformed data. It represent an output of
 * the GSL real fast fourier transform routine. The routine transforms an
 * array of n real numbers into an array of about n/2 complex numbers which
 * are the amplitudes of the positive frequencies of the full complex fourier
 * transform.
 */
class HalfComplex {
  size_t m_size;  ///< size of the transformed data
  double *m_data; ///< pointer to the transformed data
  bool m_even;    ///< true if the size of the original data is even
public:

   /// Constructor.
   /// @param data :: A pointer to the transformed complex data
   /// @param n :: The size of untransformed real data
  HalfComplex(double *data, const size_t &n)
      : m_size(n / 2 + 1), m_data(data), m_even(n / 2 * 2 == n) {}
  /// Returns the size of the transform
  size_t size() const { return m_size; }

  /// The real part of i-th transform coefficient
  /// @param i :: The index of the complex transform coefficient
  /// @return The real part
  double real(size_t i) const {
    if (i >= m_size)
      return 0.;
    if (i == 0)
      return m_data[0];
    return m_data[2 * i - 1];
  }
  /// The imaginary part of i-th transform coefficient
  /// @param i :: The index of the complex transform coefficient
  /// @return The imaginary part
  double imag(size_t i) const {
    if (i >= m_size)
      return 0.;
    if (i == 0)
      return 0;
    if (m_even && i == m_size - 1)
      return 0;
    return m_data[2 * i];
  }
  
  /// Set a new value for i-th complex coefficient
  /// @param i :: The index of the coefficient
  /// @param re :: The real part of the new value
  /// @param im :: The imaginary part of the new value
  void set(size_t i, const double &re, const double &im) {
    if (i >= m_size)
      return;
    if (i == 0) // this is purely real
    {
      m_data[0] = re;
    } else if (m_even && i == m_size - 1) // this is also purely real
    {
      m_data[2 * i - 1] = re;
    } else {
      m_data[2 * i - 1] = re;
      m_data[2 * i] = im;
    }
  }
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_HALFCOMPLEX_H_*/
