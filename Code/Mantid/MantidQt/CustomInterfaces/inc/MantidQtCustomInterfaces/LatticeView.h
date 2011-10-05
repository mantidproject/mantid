#ifndef MANTIDQTCUSTOMINTERFACES_LATTICE_VIEW_H
#define MANTIDQTCUSTOMINTERFACES_LATTICE_VIEW_H

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class LatticeView 
    {
    public:
      virtual double getA1() const = 0;
      virtual double getA2() const = 0;
      virtual double getA3() const = 0;
      virtual double getB1() const = 0;
      virtual double getB2() const = 0;
      virtual double getB3() const = 0;
      virtual void indicateModified() = 0;
      virtual void indicateDefault() = 0;
      virtual void indicateInvalid() = 0;
      virtual void initalize(double a1, double a2, double a3, double b1, double b2, double b3) = 0;
    };
  }
}

#endif