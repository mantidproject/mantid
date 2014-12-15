/***************************************************************************
    File                 : fft2D.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : FFT for matrices

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "fft2D.h"
#include "../Matrix.h"
//#define _USE_MATH_DEFINES
#include <math.h>
#include <QVarLengthArray>
void fft(double* x_int_re, double* x_int_im, int taille)
{
    int size_2=taille>>1;
    double base = 2*M_PI/taille;
    const double SQ_2=sqrt(2.0);
    QVarLengthArray<double> pair_re(size_2), pair_im(size_2), impair_re(size_2), impair_im(size_2);//double pair_re[size_2], pair_im[size_2], impair_re[size_2], impair_im[size_2];
    for(int i=0; i<size_2; i++){
        int tmp1=(i<<1);
        pair_re[i]=x_int_re[tmp1];
        pair_im[i]=x_int_im[tmp1];
        impair_re[i]=x_int_re[tmp1+1];
        impair_im[i]=x_int_im[tmp1+1];
    }

    if(taille>2){
        fft(pair_re.data(),pair_im.data(),size_2);
        fft(impair_re.data(),impair_im.data(),size_2);
    }

    for(int i=0; i<size_2; i++){
        double tmp=base*i;
        double tmpcos=cos(tmp);
        double tmpsin=sin(tmp);
        x_int_re[i]=(pair_re[i]+impair_re[i]*tmpcos+impair_im[i]*tmpsin)/SQ_2;
        x_int_im[i]=(pair_im[i]+impair_im[i]*tmpcos-impair_re[i]*tmpsin)/SQ_2;
        x_int_re[i+size_2]=(pair_re[i]-impair_re[i]*tmpcos-impair_im[i]*tmpsin)/SQ_2;
        x_int_im[i+size_2]=(pair_im[i]-impair_im[i]*tmpcos+impair_re[i]*tmpsin)/SQ_2;
    }
}

void fft_inv(double* x_int_re, double* x_int_im, int taille)
{
    int size_2=taille>>1;
    double base=2*M_PI/taille;
    const double SQ_2=sqrt(2.0);//const double SQ_2=sqrt(2);
    QVarLengthArray<double> pair_re(size_2), pair_im(size_2), impair_re(size_2), impair_im(size_2);//double pair_re[size_2], pair_im[size_2], impair_re[size_2], impair_im[size_2];
    for(int i=0; i<size_2; i++){
        int tmp1=i<<1;
        pair_re[i]=x_int_re[tmp1];
        pair_im[i]=x_int_im[tmp1];
        impair_re[i]=x_int_re[tmp1+1];
        impair_im[i]=x_int_im[tmp1+1];
    }

    if(taille>2){
        fft_inv(pair_re.data(), pair_im.data(),size_2);//fft_inv(pair_re, pair_im,size_2);
        fft_inv(impair_re.data(), impair_im.data(),size_2);//fft_inv(impair_re, impair_im,size_2);
    }

    for(int i=0; i<size_2; i++){
        double tmp=base*i;
        double tmpcos=cos(tmp);
        double tmpsin=sin(tmp);
        x_int_re[i]=(pair_re[i]+impair_re[i]*tmpcos-impair_im[i]*tmpsin)/SQ_2;
        x_int_im[i]=(pair_im[i]+impair_im[i]*tmpcos+impair_re[i]*tmpsin)/SQ_2;
        x_int_re[i+size_2]=(pair_re[i]-impair_re[i]*tmpcos+impair_im[i]*tmpsin)/SQ_2;
        x_int_im[i+size_2]=(pair_im[i]-impair_im[i]*tmpcos-impair_re[i]*tmpsin)/SQ_2;
    }
}

void fft2d(double **xtre, double **xtim, int width, int height)
{
    double **xint_re = Matrix::allocateMatrixData(height, width);
    if (!xint_re)
        return;
    double **xint_im = Matrix::allocateMatrixData(height, width);
    if (!xint_im){
        Matrix::freeMatrixData(xint_re, height);
        return;
    }

    QVarLengthArray<double> x_int_l(width), x_int2_l(width), x_int_c(height), x_int2_c(height);//double x_int_l[width], x_int2_l[width], x_int_c[height], x_int2_c[height];
    for(int k=0; k<height; k++){
        for(int j=0; j<width; j++){
            //x_int_l[j] = x[k][j];
            //x_int2_l[j] = 0.0;

            x_int_l[j] = xtre[k][j];
            x_int2_l[j] = xtim[k][j];
        }
        fft(x_int_l.data(), x_int2_l.data(), width);//fft(x_int_l, x_int2_l, width);
        for(int j=0; j<width; j++){
            xint_re[k][j]=x_int_l[j];
            xint_im[k][j]=x_int2_l[j];
        }
    }

    for(int k=0; k<width; k++){
        for(int i=0; i<height; i++){
            x_int_c[i]=xint_re[i][k];
            x_int2_c[i]=xint_im[i][k];
        }
        fft(x_int_c.data(),x_int2_c.data(), height) ;//fft(x_int_c,x_int2_c, height) ;
        for(int i=0; i<height; i++){
            xtre[(i+(height>>1))%height][(k+(width>>1))%width]=x_int_c[i];
            xtim[(i+(height>>1))%height][(k+(width>>1))%width]=x_int2_c[i];
        }
    }
    Matrix::freeMatrixData(xint_re, height);
    Matrix::freeMatrixData(xint_im, height);
}

void fft2d_inv(double **xtre, double **xtim, double **xrec_re, double **xrec_im, int width, int height)
{
    double **xint_re = Matrix::allocateMatrixData(height, width);
    if (!xint_re)
        return;
    double **xint_im = Matrix::allocateMatrixData(height, width);
    if (!xint_im){
        Matrix::freeMatrixData(xint_re, height);
        return;
    }

    QVarLengthArray<double> x_int_l(width), x_int2_l(width), x_int_c(height), x_int2_c(height);//double x_int_l[width], x_int2_l[width], x_int_c[height], x_int2_c[height];
    for(int k=0; k<height; k++){
        for(int j=0; j<width; j++){
            int idx_h = (k+(height>>1))%height;
            int idx_w = (j+(width>>1))%width;
            x_int_l[j] = xtre[idx_h][idx_w];
            x_int2_l[j] = xtim[idx_h][idx_w];
        }
        fft_inv(x_int_l.data(), x_int2_l.data(), width) ;//fft_inv(x_int_l, x_int2_l, width) ;
        for(int j=0; j<width; j++){
            xint_re[k][j] = x_int_l[j];
            xint_im[k][j] = x_int2_l[j];
        }
    }
    for(int k=0; k<width; k++){
        for(int i=0; i<height; i++){
            x_int_c[i] = xint_re[i][k];
            x_int2_c[i] = xint_im[i][k];
        }
        fft_inv(x_int_c.data(),x_int2_c.data(), height) ;//fft_inv(x_int_c,x_int2_c, height) ;
        for(int i=0; i<height; i++){
            xrec_re[i][k] = x_int_c[i];
            xrec_im[i][k] = x_int2_c[i];
        }
    }
    Matrix::freeMatrixData(xint_re, height);
    Matrix::freeMatrixData(xint_im, height);
}
