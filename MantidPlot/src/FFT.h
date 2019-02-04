// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : FFT.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef FFT_H
#define FFT_H

#include "Filter.h"

class FFT : public Filter {
  Q_OBJECT

public:
  FFT(ApplicationWindow *parent, Table *t, const QString &realColName,
      const QString &imagColName = QString(), int from = 1, int to = -1);
  FFT(ApplicationWindow *parent, Graph *g, const QString &curveTitle);
  FFT(ApplicationWindow *parent, Graph *g, const QString &curveTitle,
      double start, double end);

  void setInverseFFT(bool inverse = true) { d_inverse = inverse; };
  void setSampling(double sampling) { d_sampling = sampling; };
  void normalizeAmplitudes(bool norm = true) { d_normalize = norm; };
  void shiftFrequencies(bool shift = true) { d_shift_order = shift; };

private:
  void init();
  void output() override;
  void output(const QString &text);

  QString fftCurve();
  QString fftTable();

  bool setDataFromTable(Table *t, const QString &realColName,
                        const QString &imagColName = QString(), int from = 0,
                        int to = -1) override;

  double d_sampling;
  //! Flag telling if an inverse FFT must be performed.
  bool d_inverse;
  //! Flag telling if the amplitudes in the output spectrum must be normalized.
  bool d_normalize;
  //! Flag telling if the output frequencies must be shifted in order to have a
  // zero-centered spectrum.
  bool d_shift_order;

  int d_real_col, d_imag_col;
};

#endif
