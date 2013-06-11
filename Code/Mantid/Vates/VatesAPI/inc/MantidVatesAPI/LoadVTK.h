#ifndef LOADVTK_H_
#define LOADVTK_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace VATES
  {
    class DLLExport LoadVTK : public Mantid::API::Algorithm
    {
    public:
      virtual const std::string name() const;

      virtual int version() const;

      virtual const std::string category() const;

    private:

      virtual void init();

      virtual void exec();
    };
  }
}

#endif