#include "MantidDataObjects/CompressedWorkspace2D.h"
#include "MantidAPI/RefAxis.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <zlib.h>
#include <cstring>
#include "MantidDataObjects/ManagedHistogram1D.h"

namespace Mantid
{
namespace DataObjects
{

using std::size_t;

DECLARE_WORKSPACE(CompressedWorkspace2D)

// Get a reference to the logger
Kernel::Logger& CompressedWorkspace2D::g_log = Kernel::Logger::get("CompressedWorkspace2D");

/// Constructor
CompressedWorkspace2D::CompressedWorkspace2D() :
AbsManagedWorkspace2D()
{
}

/** Sets the size of the workspace and sets up the temporary file
*  @param NVectors :: The number of vectors/histograms/detectors in the workspace
*  @param XLength :: The number of X data points/bin boundaries in each vector (must all be the same)
*  @param YLength :: The number of data/error points in each vector (must all be the same)
*  @throw std::runtime_error if unable to open a temporary file
*/
void CompressedWorkspace2D::init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength)
{
  g_log.information("Creating a CompressedWorkspace2D");

  AbsManagedWorkspace2D::init(NVectors,XLength,YLength);
  m_vectorSize = ( m_XLength + ( 2*m_YLength ) ) * sizeof(double);

  if (! Kernel::ConfigService::Instance().getValue("CompressedWorkspace.VectorsPerBlock", m_vectorsPerBlock) )
    m_vectorsPerBlock = 4;
  // Should this ever come out to be zero, then actually set it to 1
  if ( m_vectorsPerBlock == 0 ) m_vectorsPerBlock = 1;
  // Because of iffy design, force to 1.
  m_vectorsPerBlock = 1;

  m_blockSize = m_vectorSize * m_vectorsPerBlock;
  m_inBuffer.resize( ( m_XLength + 2*m_YLength ) * m_vectorsPerBlock );

  // make the compressed buffer as big as to be able to hold uncompressed data +
  size_t bufferSize = (m_vectorSize + m_vectorSize/1000 + 12) * m_vectorsPerBlock;
  m_outBuffer.resize(bufferSize);

  // Create all the blocks
  this->initBlocks();

  //std::cerr<<"Compressed buffer size "<<bufferSize<<'\n';
  //std::cerr<<"m_vectorSize: "<<m_vectorSize<<"\n";
  //std::cerr<<"m_vectorsPerBlock: "<<m_vectorsPerBlock<<"\n";
  //std::cerr<<"Memory: "<<getMemorySize()<<"\n";

  ManagedDataBlock2D *newBlock = m_blocks[0];
  newBlock->initialize();
  CompressedPointer tmpBuff = compressBlock(newBlock,0);

  for(size_t i=0;i<NVectors;i+=m_vectorsPerBlock)
  {
    CompressedPointer p = std::make_pair(new Bytef[tmpBuff.second],tmpBuff.second);
    memcpy(p.first,tmpBuff.first,p.second);
    m_compressedData[i] = p;
  }
}

/// Destructor. Clears the buffer and deletes the temporary file.
CompressedWorkspace2D::~CompressedWorkspace2D()
{
  for(CompressedMap::const_iterator it=m_compressedData.begin();it!= m_compressedData.end();++it)
  {
    delete [] it->second.first;
  }
}

/**  This function decides if ManagedDataBlock2D with given startIndex needs to 
be loaded from storage and loads it.
@param newBlock :: Returned data block address
@param startIndex :: Starting spectrum index in the block
*/
void CompressedWorkspace2D::readDataBlock(ManagedDataBlock2D *newBlock,std::size_t startIndex)const
{
  // You only need to read it if it hasn't been loaded before
  if (!newBlock->isLoaded())
  {
    uncompressBlock(newBlock,startIndex);
  }
}

void CompressedWorkspace2D::writeDataBlock(ManagedDataBlock2D *toWrite) const
{
  CompressedPointer p = compressBlock(toWrite,toWrite->minIndex());
  CompressedPointer old_p = m_compressedData[toWrite->minIndex()];
  if (old_p.first) delete [] old_p.first;
  m_compressedData[toWrite->minIndex()] = p;
}

size_t CompressedWorkspace2D::getMemorySize() const
{
  double sz = 0.;
  for(CompressedMap::const_iterator it=m_compressedData.begin();it!= m_compressedData.end();++it)
    sz += static_cast<double>(it->second.second);
  //std::cerr<<"Memory: "<<sz/1e6<<" + "<<double(getNumberBlocks()) * double(m_blockSize)/1e6
  //  << " + " << double(m_inBuffer.size())*sizeof(double)/1e6<< " + " << double(m_outBuffer.size())/1e6<<'\n';
  sz += static_cast<double>(getNumberBlocks() * m_blockSize + m_inBuffer.size()*sizeof(double) + m_outBuffer.size());
  return static_cast<size_t>(sz);
}

/**
 *  @param block :: Pointer to the source block for compression
 *  @param startIndex :: The starting index of the block
 *  @return pointer to the compressed block
 */
CompressedWorkspace2D::CompressedPointer CompressedWorkspace2D::compressBlock(ManagedDataBlock2D* block,std::size_t startIndex) const
{
  //std::cerr<<"compress "<<startIndex<<'\n';
  size_t j = 0;
  for(size_t i=0;i<m_vectorsPerBlock;i++)
  {
    ManagedHistogram1D * spec = dynamic_cast<ManagedHistogram1D *>(block->getSpectrum(startIndex + i));
    MantidVec& X = spec->directDataX();
    std::copy(X.begin(),X.end(),m_inBuffer.begin() + j);
    j += m_XLength;
  }
  for(size_t i=0;i<m_vectorsPerBlock;i++)
  {
    ManagedHistogram1D * spec = dynamic_cast<ManagedHistogram1D *>(block->getSpectrum(startIndex + i));
    MantidVec& Y = spec->directDataY();
    std::copy(Y.begin(),Y.end(),m_inBuffer.begin() + j);
    j += m_YLength;
  }
  for(size_t i=0;i<m_vectorsPerBlock;i++)
  {
    ManagedHistogram1D * spec = dynamic_cast<ManagedHistogram1D *>(block->getSpectrum(startIndex + i));
    MantidVec& E = spec->directDataE();
    std::copy(E.begin(),E.end(),m_inBuffer.begin() + j);
    j += m_YLength;
  }

  uLongf nBuff = static_cast<uLongf>(m_outBuffer.size());

  compress2(&m_outBuffer[0],&nBuff,reinterpret_cast<Bytef*>(&m_inBuffer[0]),static_cast<uLong>(m_vectorSize*m_vectorsPerBlock),1);

  Bytef* tmp = new Bytef[nBuff];
  memcpy(tmp,&m_outBuffer[0],nBuff);

  return CompressedPointer(tmp,nBuff);
}

/**
 *  @param block :: Pointer to the destination decompressed block
 *  @param startIndex :: The starting index of the block
 */
void CompressedWorkspace2D::uncompressBlock(ManagedDataBlock2D* block,std::size_t startIndex)const
{
  uLongf nBuff = static_cast<uLongf>(m_outBuffer.size());
  CompressedPointer p = m_compressedData[startIndex];
  int status = uncompress (&m_outBuffer[0], &nBuff, p.first, static_cast<uLong>(p.second));

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

  size_t j = 0;
  for(size_t i=0;i<m_vectorsPerBlock;i++)
  {
    ManagedHistogram1D * spec = dynamic_cast<ManagedHistogram1D *>(block->getSpectrum(startIndex + i));
    MantidVec& X = spec->directDataX();
    X.assign(out + j,out + j + m_XLength);
    j += m_XLength;
  }
  for(size_t i=0;i<m_vectorsPerBlock;i++)
  {
    ManagedHistogram1D * spec = dynamic_cast<ManagedHistogram1D *>(block->getSpectrum(startIndex + i));
    MantidVec& Y = spec->directDataY();
    Y.assign(out + j,out + j + m_YLength);
    j += m_YLength;
  }
  for(size_t i=0;i<m_vectorsPerBlock;i++)
  {
    ManagedHistogram1D * spec = dynamic_cast<ManagedHistogram1D *>(block->getSpectrum(startIndex + i));
    MantidVec& E = spec->directDataE();
    E.assign(out + j,out + j + m_YLength);
    j += m_YLength;
  }

}

} // namespace DataObjects
} // namespace Mantid
