#include "MantidDataObjects/CompressedWorkspace2D.h"
#include "MantidAPI/RefAxis.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <zlib.h>
#include <cstring>

// Visual studio 2010 can't deal with NULL (for a pointer) being passed to the constructor of an std::pair
// You have to use the c++0x keyword 'nullptr' instead.
// _MSC_VER=1600 is Visual Studio 2010, so in all other cases I create a define to turn nullptr into NULL
#if (_MSC_VER!=1600)
  #ifndef nullptr
    #define nullptr NULL
  #endif
#endif

namespace Mantid
{
namespace DataObjects
{

DECLARE_WORKSPACE(CompressedWorkspace2D)

// Get a reference to the logger
Kernel::Logger& CompressedWorkspace2D::g_log = Kernel::Logger::get("CompressedWorkspace2D");

/// Constructor
CompressedWorkspace2D::CompressedWorkspace2D() :
AbsManagedWorkspace2D(100)
{
}

/** Sets the size of the workspace and sets up the temporary file
*  @param NVectors The number of vectors/histograms/detectors in the workspace
*  @param XLength The number of X data points/bin boundaries in each vector (must all be the same)
*  @param YLength The number of data/error points in each vector (must all be the same)
*  @throw std::runtime_error if unable to open a temporary file
*/
void CompressedWorkspace2D::init(const int &NVectors, const int &XLength, const int &YLength)
{
  g_log.information("Creating a CompressedWorkspace2D");

  AbsManagedWorkspace2D::init(NVectors,XLength,YLength);

  if (! Kernel::ConfigService::Instance().getValue("CompressedWorkspace.VectorsPerBlock", m_vectorsPerBlock) )
    m_vectorsPerBlock = 4;
  // Should this ever come out to be zero, then actually set it to 1
  if ( m_vectorsPerBlock == 0 ) m_vectorsPerBlock = 1;

  m_blockSize = m_vectorSize * m_vectorsPerBlock;
  m_inBuffer.resize( ( m_XLength + 2*m_YLength ) * m_vectorsPerBlock );

  // make the compressed buffer as big as to be able to hold uncompressed data +
  size_t bufferSize = (m_vectorSize + m_vectorSize/1000 + 12) * m_vectorsPerBlock;
  m_outBuffer.resize(bufferSize);

  //std::cerr<<"Compressed buffer size "<<bufferSize<<'\n';
  //std::cerr<<"m_vectorSize: "<<m_vectorSize<<"\n";
  //std::cerr<<"m_vectorsPerBlock: "<<m_vectorsPerBlock<<"\n";
  //std::cerr<<"Memeory: "<<getMemorySize()<<"\n";

  ManagedDataBlock2D *newBlock = new ManagedDataBlock2D(0, m_vectorsPerBlock, m_XLength, m_YLength);
  CompressedPointer tmpBuff = compressBlock(newBlock,0);
  m_compressedData[0] = tmpBuff;

  for(int i=0;i<NVectors;i+=m_vectorsPerBlock)
  {
    CompressedPointer p(nullptr,tmpBuff.second);
    p.first = new Bytef[p.second];
    memcpy(p.first,tmpBuff.first,p.second);
    m_compressedData[i] = p;
  }
  delete newBlock;

}

/// Destructor. Clears the buffer and deletes the temporary file.
CompressedWorkspace2D::~CompressedWorkspace2D()
{
  for(CompressedMap::const_iterator it=m_compressedData.begin();it!= m_compressedData.end();it++)
  {
    delete [] it->second.first;
  }
}

/**  This function decides if ManagedDataBlock2D with given startIndex needs to 
be loaded from storage and loads it.
@param newBlock Returned data block address
@param startIndex Starting spectrum index in the block
*/
void CompressedWorkspace2D::readDataBlock(ManagedDataBlock2D *newBlock,int startIndex)const
{
  uncompressBlock(newBlock,startIndex);
}

void CompressedWorkspace2D::writeDataBlock(ManagedDataBlock2D *toWrite) const
{
  CompressedPointer p = compressBlock(toWrite,toWrite->minIndex());
  CompressedPointer old_p = m_compressedData[toWrite->minIndex()];
  if (old_p.first) delete [] old_p.first;
  m_compressedData[toWrite->minIndex()] = p;
}

long int CompressedWorkspace2D::getMemorySize() const
{
  double sz = 0.;
  for(CompressedMap::const_iterator it=m_compressedData.begin();it!= m_compressedData.end();it++)
    sz += it->second.second;
  //std::cerr<<"Memory: "<<sz/1e6<<" + "<<double(getNumberBlocks()) * double(m_blockSize)/1e6
  //  << " + " << double(m_inBuffer.size())*sizeof(double)/1e6<< " + " << double(m_outBuffer.size())/1e6<<'\n';
  sz += double(getNumberBlocks()) * m_blockSize + double(m_inBuffer.size())*sizeof(double) + double(m_outBuffer.size());
  return (long int)(sz/1024);
}

/**
 *  @param block Pointer to the source block for compression
 *  @param startIndex The starting index of the block
 */
CompressedWorkspace2D::CompressedPointer CompressedWorkspace2D::compressBlock(ManagedDataBlock2D* block,int startIndex) const
{
  //std::cerr<<"compress "<<startIndex<<'\n';

  // --- old way -----
  //int vSize = m_XLength + 2 * m_YLength;
  //for(int i=0;i<m_vectorsPerBlock;i++)
  //{
  //    std::vector<double>& X = block->dataX(startIndex + i);
  //    std::vector<double>& Y = block->dataY(startIndex + i);
  //    std::vector<double>& E = block->dataE(startIndex + i);
  //    std::vector<double>::iterator it = std::copy(X.begin(),X.end(),m_inBuffer.begin() + i * vSize);
  //    it = std::copy(Y.begin(),Y.end(),it);
  //    it = std::copy(E.begin(),E.end(),it);

  //}
  // --- new way -----
  int j = 0;
  for(int i=0;i<m_vectorsPerBlock;i++)
  {
    MantidVec& X = block->dataX(startIndex + i);
    MantidVec::iterator it = std::copy(X.begin(),X.end(),m_inBuffer.begin() + j);
    j += m_XLength;
  }
  for(int i=0;i<m_vectorsPerBlock;i++)
  {
    MantidVec& Y = block->dataY(startIndex + i);
    MantidVec::iterator it = std::copy(Y.begin(),Y.end(),m_inBuffer.begin() + j);
    j += m_YLength;
  }
  for(int i=0;i<m_vectorsPerBlock;i++)
  {
    MantidVec& E = block->dataE(startIndex + i);
    MantidVec::iterator it = std::copy(E.begin(),E.end(),m_inBuffer.begin() + j);
    j += m_YLength;
  }

  uLongf nBuff = m_outBuffer.size();

  compress2(&m_outBuffer[0],&nBuff,reinterpret_cast<Bytef*>(&m_inBuffer[0]),m_vectorSize*m_vectorsPerBlock,1);

  Bytef* tmp = new Bytef[nBuff];
  memcpy(tmp,&m_outBuffer[0],nBuff);

  return CompressedPointer(tmp,nBuff);
}

/**
 *  @param block Pointer to the destination decompressed block
 *  @param startIndex The starting index of the block
 */
void CompressedWorkspace2D::uncompressBlock(ManagedDataBlock2D* block,int startIndex)const
{
  //std::cerr<<"uncompress "<<startIndex<<'\n';
  uLongf nBuff = m_outBuffer.size();
  CompressedPointer p = m_compressedData[startIndex];
  int status = uncompress (&m_outBuffer[0], &nBuff, p.first, p.second);

  if (status == Z_MEM_ERROR)
    throw std::runtime_error("There is not enough memory to complete the uncompress operation.");
  if (status == Z_BUF_ERROR)
    throw std::runtime_error("There is not enough room in the buffer to complete the uncompress operation.");
  if (status == Z_DATA_ERROR)
    throw std::runtime_error("Compressed data has been corrupted.");
  //std::cerr<<"uncompressed "<<nBuff<<" bytes\n";

  if (nBuff != m_vectorSize * m_vectorsPerBlock)
  {
    g_log.error()<<"Unequal sizes: " <<nBuff<<" != "<<m_vectorSize * m_vectorsPerBlock<<'\n';
    throw std::runtime_error("");
  }

  double* out = reinterpret_cast<double*>(&m_outBuffer[0]);
  //-------- old way -----------------
  //int vSize = m_XLength + 2 * m_YLength;
  //for(int i=0;i<m_vectorsPerBlock;i++)
  //{
  //    int iX = i*vSize;
  //    int iY = iX + m_XLength;
  //    int iE = iY + m_YLength;
  //    int iEnd = iE + m_YLength;
  //    std::vector<double>& X = block->dataX(startIndex + i);
  //    std::vector<double>& Y = block->dataY(startIndex + i);
  //    std::vector<double>& E = block->dataE(startIndex + i);
  //    X.assign(out + iX,out + iY);
  //    Y.assign(out + iY,out + iE);
  //    E.assign(out + iE,out + iEnd);
  //}
  //---------- new way -----------------
  int j = 0;
  for(int i=0;i<m_vectorsPerBlock;i++)
  {
    MantidVec& X = block->dataX(startIndex + i);
    X.assign(out + j,out + j + m_XLength);
    j += m_XLength;
  }
  for(int i=0;i<m_vectorsPerBlock;i++)
  {
    MantidVec& Y = block->dataY(startIndex + i);
    Y.assign(out + j,out + j + m_YLength);
    j += m_YLength;
  }
  for(int i=0;i<m_vectorsPerBlock;i++)
  {
    MantidVec& E = block->dataE(startIndex + i);
    E.assign(out + j,out + j + m_YLength);
    j += m_YLength;
  }

}

} // namespace DataObjects
} // namespace Mantid
