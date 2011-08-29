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
  namespace MDEvents
  {
    /// Forward declaration
    class CoordTransform;

    /** A parser for processing coordinate transform xml
   *
   * @author Owen Arnold
   * @date 22/july/2011
   */
    class DLLExport CoordTransformAffineParser
    {
    public:
      CoordTransformAffineParser();
      virtual CoordTransform* createTransform(Poco::XML::Element* coordTransElement) const;
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
