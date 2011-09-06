#ifndef MANTID_MDEVENTS_COORDTRANSFORMPARSER_H_
#define MANTID_MDEVENTS_COORDTRANSFORMPARSER_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Poco
{
  namespace XML
  {
    //Forward declaration
    class Element;
  }
}

namespace Mantid
{
  namespace API
  {
    /// Forward declaration
    class CoordTransform;
  }

  namespace MDEvents
  {

    /** A parser for processing coordinate transform xml
   *
   * @author Owen Arnold
   * @date 22/july/2011
   */
    class DLLExport CoordTransformAffineParser
    {
    public:
      CoordTransformAffineParser();
      virtual Mantid::API::CoordTransform* createTransform(Poco::XML::Element* coordTransElement) const;
      virtual void setSuccessor(CoordTransformAffineParser* other);
      virtual ~CoordTransformAffineParser();
      typedef boost::shared_ptr<CoordTransformAffineParser> SuccessorType_sptr;
    protected:
      SuccessorType_sptr m_successor;
    private:
      CoordTransformAffineParser(const CoordTransformAffineParser&);
      CoordTransformAffineParser& operator=(const CoordTransformAffineParser&);
    };
  }
}

#endif
