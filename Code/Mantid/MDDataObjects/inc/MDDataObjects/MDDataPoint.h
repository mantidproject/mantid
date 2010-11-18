#ifndef MD_DATAPOINT_H
#define MD_DATAPOINT_H

#include <stdint.h>

/**  path-through class which provide transformation of the data from pixel buffer format to the data fields format; 
*
* Templated for efficiency as critical
*
* It is hard-coded in the class that the run indexes and the detector indexes are present in the data and are located 
* after two double sized image fields namely signal and error.  This can be modified in more regular fashion;
*
* CODED FOR LITTLE ENDIAN -> big endian is not checked and not supported at the moment
*

    @author Alex Buts, RAL ISIS
    @date 17/10/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace Mantid{
    namespace MDDataObjects{

//********************************************************************************************************************************************************************

 template<class T=float, class I=uint16_t>
 class MDDataPoint{
 public:
   /**   the constructor which defines the size of the dataset, number of fields in this dataset and data location in memory
   *
   */
   MDDataPoint(char * buf, unsigned int n_dimensions,unsigned int nSignals0,unsigned int nIntFields,unsigned int nExperimentBits=10):
       pDataBuffer(buf),
       nDims(n_dimensions),
       nIndexes(nIntFields),
       nSignals(nSignals0),
       PixIDShift(nExperimentBits)

   {
     unsigned int i;
     pWorkingBuf = new I[nIndexes];
     // arrange array for all data fields length-es which can be present in the data structure and set its initial values to sizeof(T);
      this->field_lengths.assign(n_dimensions+nSignals+nIntFields,sizeof(T));
   
      // set field sizes of signals to size of double;
      for(i=n_dimensions;i<n_dimensions+nSignals;i++){
        this->field_lengths[i]=sizeof(double);
      }
      // set field sizes of all integer indicators to sizeof(I); Some fields will be combined in a partucular way
      for(i=n_dimensions+nSignals;i<n_dimensions+nSignals+nIntFields;i++){
        this->field_lengths[i]=sizeof(I);
      }
      // specialisation : two first fields are packed into one 32 bit field
      this->field_lengths[n_dimensions+nSignals]  =2;
      this->field_lengths[n_dimensions+nSignals+1]=2;

     // allocate and calculate the field locations
     unsigned int n_fields      = n_dimensions+nSignals+nIntFields;
     PixIndex                   = n_dimensions+nSignals;
     field_loc                  = new unsigned int[n_fields];

     field_loc[0] = 0;
     for(i=1;i<n_fields;i++){
        field_loc[i]=field_loc[i-1]+field_lengths[i-1];
     }
   // set up specific pointers for fast accessors;
     MDPointStride= field_loc[n_fields-1]+field_lengths[n_fields-1];
     pSignal      = field_loc[n_dimensions];
     pError       = field_loc[n_dimensions+1];
     pPixIndex    = field_loc[PixIndex];

   // this defines PixID masks and RinID mask -- it is assumed that these data are present in all experiments and are packed into one 32bit word
     RunIDMask=0;
     for(i=0;i<PixIDShift;i++){        RunIDMask=(RunIDMask<<1)|0x1;
     }
     PixIDMask=0;
     for(i=0;i<32-PixIDShift;i++){     PixIDMask=(PixIDMask<<1)|0x1;
     }
     // calculate the size of buffer for data indexes;
     indexBufSize = sizeof(uint32_t);
     if(nIndexes>2){
       indexBufSize+=sizeof(I)*(nIndexes-2);
     }

      
   }
   // Accessors:;
   /// obtain the value of correspondent data field;
   T getDataField(unsigned int nField,size_t n_point)const{return *(reinterpret_cast<T *>     (pDataBuffer + n_point*MDPointStride+field_loc[nField]));}
   double getSignal(size_t n_point)                  const{return *(reinterpret_cast<double *>(pDataBuffer + n_point*MDPointStride+pSignal));}
   double getError  (size_t n_point)                 const{return *(reinterpret_cast<double *>(pDataBuffer + n_point*MDPointStride +pError));}
   I getIndex(unsigned int nf,size_t n_point)        const{return *(reinterpret_cast<I *>     (pDataBuffer + n_point*MDPointStride+field_loc[PixIndex+nf]));}
   // these two are coded for particular type of experiments -> number of runs <~2^9, number of pixels <~ 2^(23)
   uint32_t getRunID(size_t n_point)                 const{return   RunIDMask&(*(reinterpret_cast<uint32_t *>(pDataBuffer+n_point*MDPointStride+pPixIndex)));}
   uint32_t getPixID(size_t n_point)                 const{return ((PixIDMask)&(*(reinterpret_cast<uint32_t *>(pDataBuffer+n_point*MDPointStride +pPixIndex)))>>PixIDShift);}
   /// get size (in bytes for the 
   unsigned int sizeofDataPoint(void)const{return MDPointStride;}
   // copy pixel from the specified location among origin pixels to the specified location among target pixels
   void copyPixel(size_t iOrigin, char *targetBuff, size_t iTarget)const{
     memcpy(base+MDPointStride*iOrigin,targetBuff+MDPointStride*iTarget,MDPointStride);
   }
     
   // function specialized for Horace data e.g. expetcs no more than 2^9 runs and no more than 2^23 pixels (unique detectors)
   void setData(unsigned int ind,T dim_fields[],double SignalFields[],int iFiels[]){
     unsigned int i,i0;

     char *const base = pDataBuffer+ind*MDPointStride;

     memcpy(base,dim_fields,sizeof(T)*nDims);
     i0 = nDims;
     memcpy(base+field_loc[i0],SignalFields,sizeof(double)*nSignals);
     // this part is specialized for particular type of data (indexes ~<2^16)
     i0=PixIndex;
     // compress n_runs and n_pixels into single 32 bit word
     uint32_t *pBuf = (uint32_t *)pWorkingBuf;
     uint32_t  w1   = (uint32_t)iFiels[0];
     uint32_t  w2   = (uint32_t)iFiels[1];
     *pBuf = (w1&RunIDMask)|((w2<<PixIDShift)&(~RunIDMask));
     // add proper number of other indexes
     unsigned int ic = sizeof(uint32_t)/sizeof(I);
     if(ic==0)ic=1;
     for(i=2;i<nIndexes;i++){
       pWorkingBuf[ic]=(I)iFiels[i];
       ic++;
     }
      memcpy(base + field_loc[i0],pWorkingBuf,indexBufSize);

   }
   //
   ~MDDataPoint(){
     delete [] field_loc;
     delete [] pWorkingBuf;
   }
// 
 private:
   unsigned int nDims,nSignals,nIndexes;
   char *const  pDataBuffer;
   unsigned int MDPointStride;
   unsigned int *field_loc;
   I*           pWorkingBuf;
   std::vector<unsigned int> field_lengths;
   unsigned int pSignal,pError,pPixIndex,PixIndex;
   // data to support packing pixID and RunId into one integer word
   uint32_t PixIDMask,RunIDMask,PixIDShift,indexBufSize;

};

}
}

#endif