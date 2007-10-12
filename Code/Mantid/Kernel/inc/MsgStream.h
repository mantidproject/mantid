// RJT, 17/09/07: This file is a TEMPORARY implementation of the MsgStream class.
// It is simply so that we can leave the code that uses it unchanged, but just send the output
// to standard output. It will disappear once we implement our own message service.

#ifndef MSGSTREAM_H_
#define MSGSTREAM_H_

/// This code comes from the Gaudi file 'IMessageSvc.h' and is imported here so that statements
/// in Algorithm still work until we have a proper Message Service.
namespace MSG   {
/// This code comes from the Gaudi file 'IMessageSvc.h' and is imported here so that statements
/// in Algorithm still work until we have a proper Message Service.
  enum Level   {
        NIL = 0,
        VERBOSE,
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL,
        ALWAYS,
        NUM_LEVELS
  };
}

#define endreq "\n";

#include <iostream>

/** @class MsgStream MsgStream.h Kernel/MsgStream.h
 * 
 *  THIS CLASS IS TEMPORARY
 *  and is simply there so that code from Gaudi can be imported unchanged until we have a proper message service
 */
class MsgStream {
public:
  /// Dummy (empty) constructor
  MsgStream(int i, std::string txt) 
  {
    // Do nothing
  }

  /// Accept MsgStream activation using MsgStreamer operator
  MsgStream& operator<< (MSG::Level level)  {   
    return *this;
  }
  
  /// Accept MsgStream activation using MsgStreamer operator
  MsgStream& operator<< (std::string output)  {
    std::cout << output;
    return *this;
  }  
  
};

#endif /*MSGSTREAM_H_*/
