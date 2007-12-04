#ifndef DataSpace_h
#define DataSpace_h

// Impliments a simple data storage space

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

  DataSpace();                                 ///< Constructor
  DataSpace(const DataSpace&);                 ///< Copy constructor
  DataSpace& operator=(const DataSpace&);      ///< Copy assignment operator
  ~DataSpace();                                ///< Destructor

  DataSpace operator+(const DataSpace&) const;   ///< Addition operator
  DataSpace& operator+=(const DataSpace&);       ///< += operator
  DataSpace& operator+=(const double);           ///< += operator

  DataSpace operator-(const DataSpace&) const;   ///< Subtraction operator
  DataSpace& operator-=(const DataSpace&);       ///< -= operator
  DataSpace& operator-=(const double);           ///< -= operator

  DataSpace& operator*=(const DataSpace&);       ///< *= operator
  DataSpace& operator*=(const double);           ///< *= operator

  DataSpace& operator/=(const DataSpace&);       ///< /= operator
  DataSpace& operator/=(const double);           ///< /= operator

  DataSpace operator*(const DataSpace&) const;   ///< * operator

  double& operator[](const int A) { return Y[A]; }            ///< Returns the indexed data point
  const double operator[](const int A) const { return Y[A]; } ///< Returns the indexed data point (const version)
  double XPt(const int A) const { return X[A]; }              ///< Returns the indexed X data point
  double YPt(const int A) const { return Y[A]; }              ///< Returns the indexed data point
  double ErrPt(const int A) const { return Err[A]; }          ///< Returns the indexed error point
  
  std::vector<double>& XValues() { return X; }       ///< Returns the vector of X values
  std::vector<double>& YValues() { return Y; }       ///< Returns the vector of Y values
  std::vector<double>& ErrValues() { return Err; }   ///< Returns the vector of error values

  const std::vector<double>& XValues() const { return X; }     ///< Returns the vector of X values (const version)
  const std::vector<double>& YValues() const { return Y; }     ///< Returns the vector of Y values (const version)
  const std::vector<double>& ErrValues() const { return Err; } ///< Returns the vector of error values (const version)


  DataSpace& rebin(const double,const double,const double);    ///< Rebin the data
  void setLevel(const int,const double,const double=0.0,const double =-1.0);   ///< Set the level
  int removeZeroX();                    ///< remove X <= 0.0
  void zero();                           ///< zero Y value

  double calcEntropy() const;                     ///< Full entropy calculation
  double calcEntropy(std::vector<double>&) const; ///< Full entropy calculation
  double calcFitBasic(const DataSpace&) const;    ///< Simple Chi^2 calc
  double calcFitBasic(const DataSpace&,
		      std::vector<double>&) const;  ///< Simple Chi^2 calc

  int size() const { return Npts; }                ///< Returns the number of points in the dataset
  void setTitle(const std::string& A) { Title=A;}  ///< Set the title of the dataset
  std::string getTitle() const { return Title; }   ///< Get the title of the dataset

  int readFour(const std::string&);                 ///< Read 4-vector?
  int write(const std::string&,const int =0) const; ///< Write out
  int write(std::ostream&) const;                   ///< Write out to a stream
  
  void convAngleToQ(const double);      ///< convert an angle to Q
  void calcTrans(const DataSpace&,const double);   ///< full transform 
};

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif



