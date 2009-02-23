#ifndef MANTID_KERNEL_FAST_EXPONENTIAL_H_
#define MANTID_KERNEL_FAST_EXPONENTIAL_H_

namespace Mantid{
namespace Kernel{


/** Based on the article: A Fast, Compact Approximation of the Exponential Function
* by Nicol N. Schraudolph and on the slighly modified version:
* On a Fast, Compact Approximation of the Exponential Function
* Neural Computation archive
* Volume 12 ,  Issue 9  (September 2000)
* Pages 2009-2012
* Author: Gavin C. Cawley 	 University of East Anglia, Norwich, Norfolk NR4 7TJ, England
* This is actually a bit faster than a LookupTable with linear interpolation, however it seems less accurate
* with error as big as a few % for some values of y.
 **/

#define EXP_A (1048576/M_LN2)
#define EXP_C 60801

inline double fast_exp(double y)
{
    static union
    {
        double d;
#ifdef LITTLE_ENDIAN
        struct { int j, i; } n;
#else
        struct { int i, j; } n;
#endif
    }
    _eco;

    _eco.n.i = (int)(EXP_A*(y)) + (1072693248 - EXP_C);
    _eco.n.j = 0;

    return _eco.d;
}

} // Namespace Kernel
} // Namespace Mantid

#endif /* FAST_EXPOENTIAL_H_ */
