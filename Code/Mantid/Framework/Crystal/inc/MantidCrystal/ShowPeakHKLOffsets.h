/*


namespace Crystal * PeakHKLOffsets.h
 *
 *  Created on: May 13, 2013
 *      Author: ruth
 */

#ifndef SHOWPEAKHKLOFFSETS_H_
#define SHOWPEAKHKLOFFSETS_H_


#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Crystal
{
  class DLLExport ShowPeakHKLOffsets  : public API::Algorithm
   {
   public:
     ShowPeakHKLOffsets();
     virtual ~ShowPeakHKLOffsets();

     virtual const std::string name() const
       {
        return "ShowPeakHKLOffsets";
       };

     virtual  int version() const
     {
       return 1;
     };

     const std::string category() const
     { return "Crystal";
     };

   private:

     void initDocs();

     void init();

     void exec();

     static Kernel::Logger& g_log ;
   };


}
}

#endif /* ShowPeakHKLOffsets_H_ */
