#ifndef MANTID_DATAHANDLING_ASCIIPOINTBASE_H_
#define MANTID_DATAHANDLING_ASCIIPOINTBASE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace DataHandling
  {
    class DLLExport AsciiPointBase : public API::Algorithm
    {
    public:
      /// Default constructor
      AsciiPointBase() {}
      /// Destructor
      ~AsciiPointBase() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const = 0;
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const = 0;
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Text"; }
      /*
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "AsciiPointBase"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Text"; }
      */
    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs() = 0;
      /// Return the file extension this algorthm should output.
      virtual std::string ext() = 0;
      /// Return the separator character
      virtual char sep() {return '\t';}
      /// return if the line should start with a separator
      virtual bool leadingSep() {return true;}
      /// Add extra properties
      virtual void extraProps() = 0;
      /// write any extra information required
      virtual void extraHeaders(std::ofstream & file) = 0;
      /// write the main content of the data
      virtual void data(std::ofstream & file, const std::vector<double> & XData);
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
      /// write the top of the file
      virtual std::vector<double> header(std::ofstream & file);
      ///static reference to the logger class
      static Kernel::Logger& g_log;
    protected:
      char m_sep;
      double m_qres;
      size_t m_xlength;
      API::MatrixWorkspace_const_sptr m_ws;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_SAVEANSTO_H_  */
