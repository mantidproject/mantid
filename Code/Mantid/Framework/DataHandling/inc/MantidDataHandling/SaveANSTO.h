#ifndef MANTID_DATAHANDLING_SAVEANSTO_H_
#define MANTID_DATAHANDLING_SAVEANSTO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace DataHandling
  {
    class DLLExport SaveANSTO : public API::Algorithm
    {
    public:
      /// Default constructor
      SaveANSTO();
      /// Destructor
      ~SaveANSTO() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveANSTO"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Text"; }

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
      ///static reference to the logger class
      static Kernel::Logger& g_log;

      API::MatrixWorkspace_const_sptr m_ws;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_SAVEANSTO_H_  */
