#ifndef MANTID_DIFFSPHERE_LOGNORMAL_H_
#define MANTID_DIFFSPHERE_LOGNORMAL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunctionMW.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    @author Jose Borreguero, NScD
    @date 11/14/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

<<<<<<< HEAD
  	// simple structure for the constant xnl coefficients
    struct xnlc{
      double x;
      int n;
      int l;
    };

    // list of 99 coefficients sorted by increasing value (F.Volino,Mol. Phys. 41,271-279,1980)
    std::vector<std::string> xnlist=(
      " 0.000000  0  0", " 2.081576  1  0", " 3.342094  2  0", " 4.493409  0  1",
      " 4.514100  3  0", " 5.646704  4  0", " 5.940370  1  1", " 6.756456  5  0",
      " 7.289932  2  1", " 7.725252  0  2", " 7.851078  6  0", " 8.583755  3  1",
      " 8.934839  7  0", " 9.205840  1  2", " 9.840446  4  1", "10.010371  8  0",
      "10.613855  2  2", "10.904122  0  3", "11.070207  5  1", "11.079418  9  0",
      "11.972730  3  2", "12.143204 10  0", "12.279334  6  1", "12.404445  1  3",
      "13.202620 11  0", "13.295564  4  2", "13.472030  7  1", "13.846112  2  3",
      "14.066194  0  4", "14.258341 12  0", "14.590552  5  2", "14.651263  8  1",
      "15.244514  3  3", "15.310887 13  0", "15.579236  1  4", "15.819216  9  1",
      "15.863222  6  2", "16.360674 14  0", "16.609346  4  3", "16.977550 10  1",
      "17.042902  2  4", "17.117506  7  2", "17.220755  0  5", "17.408034 15  0",
      "17.947180  5  3", "18.127564 11  1", "18.356318  8  2", "18.453241 16  0",
      "18.468148  3  4", "18.742646  1  5", "19.262710  6  3", "19.270294 12  1",
      "19.496524 17  0", "19.581889  9  2", "19.862424  4  4", "20.221857  2  5",
      "20.371303  0  6", "20.406581 13  1", "20.538074 18  0", "20.559428  7  3",
      "20.795967 10  2", "21.231068  5  4", "21.537120 14  1", "21.578053 19  0",
      "21.666607  3  5", "21.840012  8  3", "21.899697  1  6", "21.999955 11  2",
      "22.578058  6  4", "22.616601 20  0", "22.662493 15  1", "23.082796  4  5",
      "23.106568  9  3", "23.194996 12  2", "23.390490  2  6", "23.519453  0  7",
      "23.653839 21  0", "23.783192 16  1", "23.906450  7  4", "24.360789 10  3",
      "24.382038 13  2", "24.474825  5  5", "24.689873 22  0", "24.850085  3  6",
      "24.899636 17  1", "25.052825  1  7", "25.218652  8  4", "25.561873 14  2",
      "25.604057 11  3", "25.724794 23  0", "25.846084  6  5", "26.012188 18  1",
      "26.283265  4  6", "26.516603  9  4", "26.552589  2  7", "26.666054  0  8",
      "26.735177 15  2", "26.758685 24  0", "26.837518 12  3");

=======
  //simple structure for the constant xnl coefficients

  struct xnlc{
    double x;
    unsigned n;
    unsigned l;
  };

    // List of 98 coefficients sorted by increasing value (F.Volino,Mol. Phys. 41,271-279,1980)
    // For each coefficient, the triad (coeff, n, l) is defined
    double xnlist[]={
       2.081576,  1,  0,  3.342094,  2,  0,  4.493409,  0,  1,  4.514100,  3,  0,
       5.646704,  4,  0,  5.940370,  1,  1,  6.756456,  5,  0,  7.289932,  2,  1,
       7.725252,  0,  2,  7.851078,  6,  0,  8.583755,  3,  1,  8.934839,  7,  0,
       9.205840,  1,  2,  9.840446,  4,  1, 10.010371,  8,  0, 10.613855,  2,  2,
      10.904122,  0,  3, 11.070207,  5,  1, 11.079418,  9,  0, 11.972730,  3,  2,
      12.143204, 10,  0, 12.279334,  6,  1, 12.404445,  1,  3, 13.202620, 11,  0,
      13.295564,  4,  2, 13.472030,  7,  1, 13.846112,  2,  3, 14.066194,  0,  4,
      14.258341, 12,  0, 14.590552,  5,  2, 14.651263,  8,  1, 15.244514,  3,  3,
      15.310887, 13,  0, 15.579236,  1,  4, 15.819216,  9,  1, 15.863222,  6,  2,
      16.360674, 14,  0, 16.609346,  4,  3, 16.977550, 10,  1, 17.042902,  2,  4,
      17.117506,  7,  2, 17.220755,  0,  5, 17.408034, 15,  0, 17.947180,  5,  3,
      18.127564, 11,  1, 18.356318,  8,  2, 18.453241, 16,  0, 18.468148,  3,  4,
      18.742646,  1,  5, 19.262710,  6,  3, 19.270294, 12,  1, 19.496524, 17,  0,
      19.581889,  9,  2, 19.862424,  4,  4, 20.221857,  2,  5, 20.371303,  0,  6,
      20.406581, 13,  1, 20.538074, 18,  0, 20.559428,  7,  3, 20.795967, 10,  2,
      21.231068,  5,  4, 21.537120, 14,  1, 21.578053, 19,  0, 21.666607,  3,  5,
      21.840012,  8,  3, 21.899697,  1,  6, 21.999955, 11,  2, 22.578058,  6,  4,
      22.616601, 20,  0, 22.662493, 15,  1, 23.082796,  4,  5, 23.106568,  9,  3,
      23.194996, 12,  2, 23.390490,  2,  6, 23.519453,  0,  7, 23.653839, 21,  0,
      23.783192, 16,  1, 23.906450,  7,  4, 24.360789, 10,  3, 24.382038, 13,  2,
      24.474825,  5,  5, 24.689873, 22,  0, 24.850085,  3,  6, 24.899636, 17,  1,
      25.052825,  1,  7, 25.218652,  8,  4, 25.561873, 14,  2, 25.604057, 11,  3,
      25.724794, 23,  0, 25.846084,  6,  5, 26.012188, 18,  1, 26.283265,  4,  6,
      26.516603,  9,  4, 26.552589,  2,  7, 26.666054,  0,  8, 26.735177, 15,  2,
      26.758685, 24,  0, 26.837518, 12,  3};

    // simple structure to hold a linear interpolation of factor J around its numerical divergence point
    struct linearJ{
      double slope;
      double intercept;
    };

>>>>>>> Refs #4394 Updated main fitting funtion
    class DLLExport DiffSphere : public API::ParamFunction, public API::IFunctionMW
    {
    public:
      /// Constructor
      DiffSphere();
      /// Destructor
      virtual ~DiffSphere() {}

      /// overwrite IFunction base class methods
      std::string name()const{return "DiffSphere";}

    protected:
      virtual void functionMW(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDerivMW(API::Jacobian* out, const double* xValues, const size_t nData);
      std::vector<xnlc> xnl; //xnl coefficients
<<<<<<< HEAD

    private:
      void initXnlCoeff(std::vector<std::string> coeffList);
=======
      std::vector<double> alpha; //certain coefficients invariant during fitting
      size_t lmax=8;         //maximum value of l in xnlist
      size_t ncoeff=98;      //number of coefficients
      double divZone=0.05;   //linear interpolatio zone around the numerical divergence of factor J
      std::vector<linearJ> linearJlist;

    private:
      void initXnlCoeff(double *coeffList);
      void initAlphaCoeff();
      void initLinJlist();
>>>>>>> Refs #4394 Updated main fitting funtion

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_DIFFSPHERE_LOGNORMAL_H_*/
