/*
 * SCDCalibratePanels.h
 *
 *  Created on: Mar 12, 2012
 *      Author: ruth
 */

#ifndef SCDCALIBRATEPANELS_H_
#define SCDCALIBRATEPANELS_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Crystal
{

  class SCDCalibratePanels: public Mantid::API::Algorithm
  {
  public:
    SCDCalibratePanels();
    virtual ~SCDCalibratePanels();

    virtual const std::string name() const
    {
       return "SCDCalibratePanels";
    }

     /// Algorithm's version for identification overriding a virtual method
     virtual int version() const
    {
        return 1;
    }

     /// Algorithm's category for identification overriding a virtual method
     virtual const std::string category() const
     {
       return "Crystal";
     }


  private:
    void exec ();

    void  init ();

     void initDocs ();

     static Kernel::Logger & g_log;

  };

}//namespace Crystal
}//namespace Mantid

#endif /* SCDCALIBRATEPANELS_H_ */
