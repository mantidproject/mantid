#ifndef MANTID_DATAHANDLING_SAVEANSTOASCII_H_
#define MANTID_DATAHANDLING_SAVEANSTOASCII_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/AsciiPointBase.h"

namespace Mantid
{
  namespace DataHandling
  {
    class DLLExport SaveANSTOAscii : public DataHandling::AsciiPointBase
    {
    public:
      /// Default constructor
      SaveANSTOAscii() {}
      /// Destructor
      ~SaveANSTOAscii() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveANSTOAscii"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Return the file extension this algorthm should output.
      virtual std::string ext() {return ".txt";}
      /// return if the line should start with a separator
      virtual bool leadingSep() {return false;}
      /// no extra properties required so override blank
      virtual void extraProps() {}
      /// no extra information required so override blank
      virtual void extraHeaders(std::ofstream & file) {}
      ///static reference to the logger class
      static Kernel::Logger& g_log;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_SAVEANSTO_H_  */
