#ifndef H_POINT3D
#define H_POINT3D

namespace Mantid{
       namespace MDDataObjects{
///
struct data_point{
    double s;   // signal field;
    double err; // error field
    long   npix; // number of the pixels which contribute to this particular data point;
};
struct coordinate{
  double x,y,z;
};

class DLLExport point3D
{

    friend class MDData;
public:
    point3D(void);
    ~point3D();
private:
    coordinate r;
    data_point data;


};

}
}
#endif
