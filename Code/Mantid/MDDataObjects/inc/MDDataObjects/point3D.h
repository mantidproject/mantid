#ifndef H_POINT3D
#define H_POINT3D

namespace Mantid{
namespace MDDataObjects{

//
struct MD_image_point
{
  double s;      // signal
  double err;   // error
  unsigned long npix;  // numer of data points (pixels) contributed into this point;
  size_t  chunk_location;
};

//
class DLLExport point3D
{

public:
  point3D(void):x(0),y(0),z(0){};
  point3D(double x0, double y0, double z0):x(x0),y(y0),z(z0),s(0){};

  ~point3D(){};

  inline double GetX() const{return x;}
  inline double GetY() const{return y;}
  inline double GetZ() const{return z;}
  double GetS()const{return s;}
  //   double GetErr()const{return err;}
  //   unsigned int GetNpix()const{return npix;}

  double &X(){return x;}
  double &Y(){return y;}
  double &Z(){return z;}
  double &S(){return s;}
  //   double &Err(){return err;}
  //   unsigned long &Npix(){return npix;}

  point3D &operator=(const  MD_image_point &data)
  {
    this->s = data.s;
    return *this;
  }

private:
  double x,y,z;
  double s;   // signal field;

};

}
}
#endif
