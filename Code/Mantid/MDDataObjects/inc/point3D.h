#ifndef H_POINT3D
#define H_POINT3D

struct data_point{
    double s;   // signal field;
    double err; // error field
    long   npix; // number of the pixels which contribute to this particular data point;
};
struct coordinate{
  double x,y,z;
};

class point3D
{
public:
    point3D(void);
    ~point3D(void);
private:
    coordinate r;
    data_point data;

friend class DND;
};
#endif
