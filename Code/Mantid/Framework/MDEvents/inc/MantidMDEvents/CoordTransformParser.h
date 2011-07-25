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
    class DLLExport CoordTransformParser
    {
    public:
      CoordTransformParser();
      virtual CoordTransform* createTransform(Poco::XML::Element* coordTransElement) const;
      virtual void setSuccessor(CoordTransformParser* other);
      virtual ~CoordTransformParser();
      typedef boost::shared_ptr<CoordTransformParser> SuccessorType_sptr;
    protected:
      SuccessorType_sptr m_successor;
    private:
      CoordTransformParser(const CoordTransformParser&);
      CoordTransformParser& operator=(const CoordTransformParser&);
    };
  }
}

#endif