#ifndef Chebpoly_h
#define Chebpoly_h

/*!
  \class ChebPoly
  \brief Adds the ablity to put a cheb-poly on a class
  \version 1.0
  \date August 2006
  \author S. Ansell
*/

class ChebPoly  
{
  
 private:
  
  typedef std::vector<double> storeType;
  typedef std::vector<double>::const_iterator storeIter;

  int polyN;    ///< number of polynominal units  
  int Npts;     ///< number of points
  const std::vector<double>& X;      ///< X-coord
  const std::vector<double>& Y;      ///< Y-coord
  const std::vector<double>& Err;    ///< Error values 
  std::vector<double> Coef;          ///< Coefficients

						 
  void chebft();
  double fit(double) const;
  double polint(const storeIter,const storeIter,const int,double) const;
  void resizeYfit(const int);
 
 public:
  
  ChebPoly(const storeType&,const storeType&,const storeType&);
  ChebPoly(const ChebPoly&);
  ChebPoly& operator=(const ChebPoly&);
  ~ChebPoly();

  void chebypol();
  double chi();
  double value(const double) const;

};


#endif





