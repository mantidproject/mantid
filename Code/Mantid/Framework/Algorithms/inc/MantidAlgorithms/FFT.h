#ifndef MANTID_ALGORITHM_FFT_H_
#define MANTID_ALGORITHM_FFT_H_
/*WIKI* 


The FFT algorithm performs discrete Fourier transform of complex data using the Fast Fourier Transform algorithm. It uses the GSL Fourier transform functions to do the calculations. Due to the nature of the fast fourier transform the input spectra must have linear x axes. If the imaginary part is not set the data is considered real. The "Transform" property defines the direction of the transform: direct ("Forward") or inverse ("Backward").

Note that the input data is shifted before the transform along the x axis to place the origin in the middle of the x-value range. It means that for the data defined on an interval [A,B] the output <math>F(\xi_k)</math> must be multiplied by <math> e^{-2\pi ix_0\xi_k} </math>, where <math>x_0=\tfrac{1}{2}(A+B)</math>, <math>\xi_k</math> is the frequency.

==Details==

The Fourier transform of a complex function <math>f(x)</math> is defined by equation:

:<math>F(\xi)=\int_{-\infty}^\infty f(x)e^{-2\pi ix\xi} dx</math>

For discrete data with equally spaced <math>x_n</math> and only non-zero on an interval <math>[A,B]</math> the integral can be approximated by a sum:

:<math>F(\xi)=\Delta x\sum_{n=0}^{N-1}f(x_n)e^{-2\pi ix_n\xi}</math>

Here <math>\Delta x</math> is the bin width, <math>x_n=A+\Delta xn</math>. If we are looking for values of the transformed function <math>F</math> at discrete frequencies <math>\xi_k=\Delta\xi k</math> with 

:<math>\Delta\xi=\frac{1}{B-A}=\frac{1}{L}=\frac{1}{N\Delta x}</math>

the equation can be rewritten:

:<math>F_k=e^{-2\pi iA\xi_k}\Delta x\sum_{n=0}^{N-1}f_ne^{-\tfrac{2\pi i}{N}nk}</math>

Here <math>f_n=f(x_n)</math> and <math>F_k=F(\xi_k)</math>. The fromula 

:<math>\tilde{F}_k=\Delta x\sum_{n=0}^{N-1}f_ne^{-\tfrac{2\pi i}{N}nk}</math>

is the Discrete Fourier Transform (DFT) and can be efficiently evaluated using the Fast Fourier Transform algorithm. The DFT formula calculates the Fourier transform of data on the interval <math>[0,L]</math>. It should be noted that it is periodic in <math>k</math> with period <math>N</math>. If we also assume that <math>f_n</math> is aslo periodic with period <math>N</math> the DFT can be used to transform data on the interval <math>[-L/2,L/2]</math>. To do this we swap the two halves of the data array <math>f_n</math>. If we denote the modified array as <math>\bar{f}_n</math>, its transform will be

:<math>\bar{F}_k=\Delta x\sum_{n=0}^{N-1}\bar{f}_ne^{-\tfrac{2\pi i}{N}nk}</math>

The Mantid FFT algorithm returns the complex array <math>\bar{F}_K</math> as Y values of two spectra in the output workspace, one for the real and the other for the imaginary part of the transform. The X values are set to the transform frequencies and have the range approximately equal to <math>[-N/L,N/L]</math>. The actual limits depend sllightly on whether <math>N</math> is even or odd and whether the input spectra are histograms or point data. The variations are of the order of <math>\Delta\xi</math>. The zero frequency is always in the bin with index <math>k=N/2</math>.

===Example 1===

In this example the input data were calculated using function <math>\exp(-(x-1)^2)</math>.

[[Image:FFTGaussian1.png|Gaussian]]

[[Image:FFTGaussian1FFT.png|FFT of a Gaussian]]

Because the <math>x=0</math> is in the middle of the data array the transform shown is the exact DFT of the input data.

===Example 2===

In this example the input data were calculated using function <math>\exp(-x^2)</math>.

[[Image:FFTGaussian2.png|Gaussian]]

[[Image:FFTGaussian1FFT.png|FFT of a Gaussian]]

Because the <math>x=0</math> is not in the middle of the data array the transform shown includes a shifting factor of <math>\exp(2\pi i\xi)</math>. To remove it the output must be mulitplied by <math>\exp(-2\pi i\xi)</math>. The corrected transform will be:

[[Image:FFTGaussian2FFT.png|FFT of a Gaussian]]

It should be noted that in a case like this, i.e. when the input is a real positive even function, the correction can be done by finding the transform's modulus <math>(Re^2+Im^2)^{1/2}</math>. The output workspace includes the modulus of the transform.

==Output==

The output workspace for a direct ("Forward") transform contains six spectra. The actual transfor is written to spectra with indeces 3 and 4 which are the real and imaginary parts correspondingly. The last spectrum (index 5) has the modulus of the transform: <math> \sqrt(Re^2+Im^2) </math>. The spectra from 0 to 2 repeat these results for positive frequencies only. This corresponds to Fourier transform of real data and is only meaningful if the input data are real. 

{| border="1" cellpadding="5" cellspacing="0"
!Workspace index
!Description
|-
|0
|Real part, positive frequencies
|-
|1
|Imaginary part, positive frequencies
|-
|2
|Modulus, positive frequencies
|-
|3
|Complete real part
|-
|4
|Complete imaginary part
|-
|5
|Complete transform modulus
|}

The output workspace for an inverse ("Backward") transform has 3 spectra for the real (0), imaginary (1) parts, and the modulus (2).

{| border="1" cellpadding="5" cellspacing="0"
!Workspace index
!Description
|-
|0
|Real part
|-
|1
|Imaginary part
|-
|2
|Modulus
|}


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace Algorithms
{

/** Performs a Fast Fourier Transform of data

    @author Roman Tolchenov
    @date 07/07/2009

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport FFT : public API::Algorithm
{
public:
  /// Default constructor
  FFT() : API::Algorithm() {};
  /// Destructor
  virtual ~FFT() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FFT";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FFT_H_*/
