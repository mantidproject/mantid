#include "MantidCurveFitting/ThermalNeutronDtoTOFFunction.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include <gsl/gsl_sf_erf.h>

namespace Mantid
{
namespace CurveFitting
{

//----------------------------------------------------------------------------------------------
DECLARE_FUNCTION(ThermalNeutronDtoTOFFunction)

//----------------------------------------------------------------------------------------------
/** Constructor
   */
ThermalNeutronDtoTOFFunction::ThermalNeutronDtoTOFFunction()
{
}

//----------------------------------------------------------------------------------------------
/** Destructor
  */
ThermalNeutronDtoTOFFunction::~ThermalNeutronDtoTOFFunction()
{
}

/**
  * Define the fittable parameters
  */
void ThermalNeutronDtoTOFFunction::init()
{

    /// Instrument geometry related
    declareParameter("Dtt1", 1.0);
    declareParameter("Dtt1t", 1.0);
    declareParameter("Dtt2t", 1.0);
    declareParameter("Zero", 0.0);
    declareParameter("Zerot", 0.0);

    declareParameter("Width", 1.0);
    declareParameter("Tcross", 1.0);
}

/**
   * Main function
   */
void ThermalNeutronDtoTOFFunction::function1D(double* out, const double* xValues, const size_t nData) const
{
    double dtt1 = getParameter("Dtt1");
    double dtt1t = getParameter("Dtt1t");
    double dtt2t = getParameter("Dtt2t");
    double zero = getParameter("Zero");
    double zerot = getParameter("Zerot");
    double width = getParameter("Width");
    double tcross = getParameter("Tcross");

    for (size_t i = 0; i < nData; ++i)
    {
        out[i] = corefunction(xValues[i], dtt1, dtt1t, dtt2t, zero, zerot, width, tcross);
    }

    return;
}


/** Core function
  */
inline double ThermalNeutronDtoTOFFunction::corefunction(double dh, double dtt1, double dtt1t, double dtt2t,
                                                       double zero, double zerot, double width, double tcross) const
{
    double n = 0.5*gsl_sf_erfc(width*(tcross-1/dh));
    double Th_e = zero + dtt1*dh;
    double Th_t = zerot + dtt1t*dh - dtt2t/dh;
    double tof_h = n*Th_e + (1-n)*Th_t;

    return tof_h;
}

} // namespace CurveFitting
} // namespace Mantid
