#ifndef MANTID_DATAHANDLING_SAVEANSTOASCII_H_
#define MANTID_DATAHANDLING_SAVEANSTOASCII_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace DataHandling
  {
    class DLLExport SaveANSTOAscii : public API::Algorithm
    {
    public:
      /// Default constructor
      SaveANSTOAscii();
      /// Destructor
      ~SaveANSTOAscii() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveANSTOAscii"; }
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
      /// returns true if the value is NaN
      bool checkIfNan(const double& value) const;
      /// returns true if the value if + or - infinity
      bool checkIfInfinite(const double& value) const;
      /// print the appropriate value to file
      void outputval (double val, std::ofstream & file, bool leadingSep = true);
      char m_sep;
      API::MatrixWorkspace_const_sptr m_ws;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_SAVEANSTO_H_  */
