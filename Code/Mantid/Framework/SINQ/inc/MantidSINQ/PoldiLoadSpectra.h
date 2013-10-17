#ifndef MANTID_DATAHANDLING_PoldiLoadChopperSlits_H_
#define MANTID_DATAHANDLING_PoldiLoadChopperSlits_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/System.h"

#include <napi.h>
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid
{
  namespace DataHandling
  {

    class DLLExport PoldiLoadSpectra : public API::Algorithm
    {
    public:
      /// Default constructor
    	PoldiLoadSpectra(){};
      /// Destructor
      virtual ~PoldiLoadSpectra() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "PoldiLoadSpectra"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Poldi\\PoldiSet"; }



    protected:
      /// Overwrites Algorithm method
      void exec();





    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Overwrites Algorithm method.
      void init();

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadPoldiNexus_H_*/
