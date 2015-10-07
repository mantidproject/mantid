#include "GLActor.h"
#include "GLActorVisitor.h"

GLActor::~GLActor()
{
}

bool GLActor::accept(GLActorVisitor& visitor, VisitorAcceptRule)
{
    return visitor.visit(this);
}

bool GLActor::accept(GLActorConstVisitor &visitor, GLActor::VisitorAcceptRule) const
{
    return visitor.visit(this);
}

GLColor GLActor::makePickColor(size_t pickID)
{
    pickID += 1;
    unsigned char r,g,b;
    r=(unsigned char)(pickID/65536);
    g=(unsigned char)((pickID%65536)/256);
    b=(unsigned char)((pickID%65536)%256);
    return GLColor(r,g,b);
}

size_t GLActor::decodePickColor(const QRgb& c)
{
  return decodePickColor((unsigned char)qRed(c),(unsigned char)qGreen(c),(unsigned char)qBlue(c));
}

size_t GLActor::decodePickColor(unsigned char r,unsigned char g,unsigned char b)
{
  unsigned int index = r;
  index *= 256;
  index += g;
  index *= 256;
  index += b - 1;
  return index;
}

GLColor GLActor::defaultDetectorColor()
{
  return GLColor(200,200,200);
}
