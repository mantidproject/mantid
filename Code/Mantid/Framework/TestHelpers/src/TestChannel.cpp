//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidTestHelpers/TestChannel.h"

namespace Mantid
{

  namespace TestHelpers
  {

    TestChannel::TestChannel()
    {
    }

    TestChannel::~TestChannel()
    {
    }

    void TestChannel::log(const Poco::Message& msg)
    {
      _msgList.push_back(msg);
    }

    TestChannel::MsgList& TestChannel::list()
    {
      return _msgList;
    }

    void TestChannel::clear()
    {
      _msgList.clear();
    }

  }
}
