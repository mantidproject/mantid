#ifndef DataSpace_h
#define DataSpace_h

//! Impliments a simple data storage space

namespace Mantid
{

namespace Geometry
{

  /*!
    \class DataSpace 
    \brief Basic X:Y:err read/process class
    \version 1.0
    \author S. Ansell
    \date October 2007
    
    This is to be superceeded by DataLine/DataMap and others
    in src/include 
   */

class DataSpace
{
 private:

  static Logger& PLog;           ///< The official logger

  std::string Title;              ///< Title of the workspace

  int Npts;                       ///< Number of points in X,Y,Err
  std::vector<double> X;          ///< X points
  std::vector<double> Y;          ///< Y (data) Points
  std::vector<double> Err;        ///< Error on Y

 public:

  DataSpace();
  DataSpace(const DataSpace&);
  DataSpace& operator=(const DataSpace&);
  ~DataSpace();

  DataSpace operator+(const DataSpace&) const;
  DataSpace& operator+=(const DataSpace&);
  DataSpace& operator+=(const double);         

  DataSpace operator-(const DataSpace&) const;
  DataSpace& operator-=(const DataSpace&);
  DataSpace& operator-=(const double);         

  DataSpace& operator*=(const DataSpace&);
  DataSpace& operator*=(const double);

  DataSpace& operator/=(const DataSpace&);
  DataSpace& operator/=(const double);

  DataSpace operator*(const DataSpace&) const;

  double& operator[](const int A) { return Y[A]; } 
  const double operator[](const int A) const { return Y[A]; } 
  double XPt(const int A) const { return X[A]; } 
  double YPt(const int A) const { return Y[A]; } 
  double ErrPt(const int A) const { return Err[A]; } 
  
  std::vector<double>& XValues() { return X; }
  std::vector<double>& YValues() { return Y; }
  std::vector<double>& ErrValues() { return Err; }

  const std::vector<double>& XValues() const { return X; }
  const std::vector<double>& YValues() const { return Y; }
  const std::vector<double>& ErrValues() const { return Err; }


  DataSpace& rebin(const double,const double,const double);
  void setLevel(const int,const double,const double=0.0,const double =-1.0);
  int removeZeroX();                    ///< remove X <= 0.0
  void zero();                           ///< zero Y value

  double calcEntropy() const;                     ///< Full entropy calculation
  double calcEntropy(std::vector<double>&) const; ///< Full entropy calculation
  double calcFitBasic(const DataSpace&) const;    ///< Simple Chi^2 calc
  double calcFitBasic(const DataSpace&,
		      std::vector<double>&) const;  ///< Simple Chi^2 calc

  int size() const { return Npts; }
  void setTitle(const std::string& A) { Title=A;} 
  std::string getTitle() const { return Title; }

  int readFour(const std::string&);
  int write(const std::string&,const int =0) const;
  int write(std::ostream&) const;
  
  void convAngleToQ(const double);      ///< convert an angle to Q
  void calcTrans(const DataSpace&,const double);   ///< full transform 
};

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif



