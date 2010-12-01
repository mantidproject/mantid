#ifndef MANTID_CURVEFITTING_GSLFUNCTIONS_H_
#define MANTID_CURVEFITTING_GSLFUNCTIONS_H_


namespace Mantid
{
  double gsl_costFunction(const gsl_vector * x, void *params);
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLFUNCTIONS_H_*/
