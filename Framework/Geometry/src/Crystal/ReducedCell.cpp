/* File: ReducedCell.cpp */

#include "MantidGeometry/Crystal/ReducedCell.h"
#include "MantidKernel/V3D.h"
#include <stdexcept>

namespace Mantid {
namespace Geometry {
using Mantid::Kernel::V3D;
using Mantid::Kernel::DblMatrix;

/**
 *  Array of basic transformations from reduced cell to conventional cell
 *  for rows 1 to 44 of Table 2.  This array is indexed by the row number
 *  1 to 44.  Entry 0 is the identity matrix.
 */
static const double transforms[ReducedCell::NUM_CELL_TYPES + 1][3][3] = // row
    {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},                                 //  0

     {{1, -1, 1}, {1, 1, -1}, {-1, 1, 1}},   //  1
     {{1, -1, 0}, {-1, 0, 1}, {-1, -1, -1}}, //  2
     {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},      //  3
     {{1, -1, 0}, {-1, 0, 1}, {-1, -1, -1}}, //  4
     {{1, 0, 1}, {1, 1, 0}, {0, 1, 1}},      //  5

     {{0, 1, 1}, {1, 0, 1}, {1, 1, 0}},       //  6
     {{1, 0, 1}, {1, 1, 0}, {0, 1, 1}},       //  7
     {{-1, -1, 0}, {-1, 0, -1}, {0, -1, -1}}, //  8
     {{1, 0, 0}, {-1, 1, 0}, {-1, -1, 3}},    //  9
     {{1, 1, 0}, {1, -1, 0}, {0, 0, -1}},     // 10

     {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},  // 11
     {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},  // 12
     {{1, 1, 0}, {-1, 1, 0}, {0, 0, 1}}, // 13
     {{1, 1, 0}, {-1, 1, 0}, {0, 0, 1}}, // 14
     {{1, 0, 0}, {0, 1, 0}, {1, 1, 2}},  // 15

     {{-1, -1, 0}, {1, -1, 0}, {1, 1, 2}},  // 16
     {{-1, 0, -1}, {-1, -1, 0}, {0, 1, 1}}, // 17
     {{0, -1, 1}, {1, -1, -1}, {1, 0, 0}},  // 18
     {{-1, 0, 0}, {0, -1, 1}, {-1, 1, 1}},  // 19
     {{0, 1, 1}, {0, 1, -1}, {-1, 0, 0}},   // 20

     {{0, 1, 0}, {0, 0, 1}, {1, 0, 0}},  // 21
     {{0, 1, 0}, {0, 0, 1}, {1, 0, 0}},  // 22
     {{0, 1, 1}, {0, -1, 1}, {1, 0, 0}}, // 23
     {{1, 2, 1}, {0, -1, 1}, {1, 0, 0}}, // 24
     {{0, 1, 1}, {0, -1, 1}, {1, 0, 0}}, // 25

     {{1, 0, 0}, {-1, 2, 0}, {-1, 0, 2}},   // 26
     {{0, -1, 1}, {-1, 0, 0}, {1, -1, -1}}, // 27
     {{-1, 0, 0}, {-1, 0, 2}, {0, 1, 0}},   // 28
     {{1, 0, 0}, {1, -2, 0}, {0, 0, -1}},   // 29
     {{0, 1, 0}, {0, 1, -2}, {-1, 0, 0}},   // 30

     {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},    // 31
     {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},    // 32
     {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},    // 33
     {{-1, 0, 0}, {0, 0, -1}, {0, -1, 0}}, // 34
     {{0, -1, 0}, {-1, 0, 0}, {0, 0, -1}}, // 35

     {{1, 0, 0}, {-1, 0, -2}, {0, 1, 0}},   // 36
     {{1, 0, 2}, {1, 0, 0}, {0, 1, 0}},     // 37
     {{-1, 0, 0}, {1, 2, 0}, {0, 0, -1}},   // 38
     {{-1, -2, 0}, {-1, 0, 0}, {0, 0, -1}}, // 39
     {{0, -1, 0}, {0, 1, 2}, {-1, 0, 0}},   // 40

     {{0, -1, -2}, {0, -1, 0}, {-1, 0, 0}},  // 41
     {{-1, 0, 0}, {0, -1, 0}, {1, 1, 2}},    // 42
     {{-1, 0, 0}, {-1, -1, -2}, {0, -1, 0}}, // 43
     {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};     // 44

/**
 *  These transforms pre-multiply the basic transforms in certain cases,
 *  as listed in the footnotes to Table 2.
 */
static const double transform_modifier[2][3][3] = {
    {{0, 0, -1}, {0, 1, 0}, {1, 0, 1}},   //  0
    {{-1, 0, -1}, {0, 1, 0}, {1, 0, 0}}}; //  1

/**
 *  Array of Strings specifying the cell type for reduced cells for rows
 *  1 to 44 of Table 2.  This array is indexed by the row number 1 to 44.
 *  Entry 0 is the String "None".
 */
static const std::string lattice_types[ReducedCell::NUM_CELL_TYPES + 1] = {
    ReducedCell::NONE(), //  0

    ReducedCell::CUBIC(),        //  1
    ReducedCell::RHOMBOHEDRAL(), //  2
    ReducedCell::CUBIC(),        //  3
    ReducedCell::RHOMBOHEDRAL(), //  4
    ReducedCell::CUBIC(),        //  5

    ReducedCell::TETRAGONAL(),   //  6
    ReducedCell::TETRAGONAL(),   //  7
    ReducedCell::ORTHORHOMBIC(), //  8
    ReducedCell::RHOMBOHEDRAL(), //  9
    ReducedCell::MONOCLINIC(),   // 10

    ReducedCell::TETRAGONAL(),   // 11
    ReducedCell::HEXAGONAL(),    // 12
    ReducedCell::ORTHORHOMBIC(), // 13
    ReducedCell::MONOCLINIC(),   // 14
    ReducedCell::TETRAGONAL(),   // 15

    ReducedCell::ORTHORHOMBIC(), // 16
    ReducedCell::MONOCLINIC(),   // 17
    ReducedCell::TETRAGONAL(),   // 18
    ReducedCell::ORTHORHOMBIC(), // 19
    ReducedCell::MONOCLINIC(),   // 20

    ReducedCell::TETRAGONAL(),   // 21
    ReducedCell::HEXAGONAL(),    // 22
    ReducedCell::ORTHORHOMBIC(), // 23
    ReducedCell::RHOMBOHEDRAL(), // 24
    ReducedCell::MONOCLINIC(),   // 25

    ReducedCell::ORTHORHOMBIC(), // 26
    ReducedCell::MONOCLINIC(),   // 27
    ReducedCell::MONOCLINIC(),   // 28
    ReducedCell::MONOCLINIC(),   // 29
    ReducedCell::MONOCLINIC(),   // 30

    ReducedCell::TRICLINIC(),    // 31
    ReducedCell::ORTHORHOMBIC(), // 32
    ReducedCell::MONOCLINIC(),   // 33
    ReducedCell::MONOCLINIC(),   // 34
    ReducedCell::MONOCLINIC(),   // 35

    ReducedCell::ORTHORHOMBIC(), // 36
    ReducedCell::MONOCLINIC(),   // 37
    ReducedCell::ORTHORHOMBIC(), // 38
    ReducedCell::MONOCLINIC(),   // 39
    ReducedCell::ORTHORHOMBIC(), // 40

    ReducedCell::MONOCLINIC(),   // 41
    ReducedCell::ORTHORHOMBIC(), // 42
    ReducedCell::MONOCLINIC(),   // 43
    ReducedCell::TRICLINIC()};   // 44

/**
 *  Array of Strings specifying the centering for reduced cells for rows
 *  1 to 44 of Table 2.  This array is indexed by the row number 1 to 44.
 *  Entry 0 is the String "None".
 */
static const std::string center_types[ReducedCell::NUM_CELL_TYPES + 1] = {
    ReducedCell::NONE(), //  0

    ReducedCell::F_CENTERED(), //  1
    ReducedCell::R_CENTERED(), //  2
    ReducedCell::P_CENTERED(), //  3
    ReducedCell::R_CENTERED(), //  4
    ReducedCell::I_CENTERED(), //  5

    ReducedCell::I_CENTERED(), //  6
    ReducedCell::I_CENTERED(), //  7
    ReducedCell::I_CENTERED(), //  8
    ReducedCell::R_CENTERED(), //  9
    ReducedCell::C_CENTERED(), // 10

    ReducedCell::P_CENTERED(), // 11
    ReducedCell::P_CENTERED(), // 12
    ReducedCell::C_CENTERED(), // 13
    ReducedCell::C_CENTERED(), // 14
    ReducedCell::I_CENTERED(), // 15

    ReducedCell::F_CENTERED(), // 16
    ReducedCell::I_CENTERED(), // 17
    ReducedCell::I_CENTERED(), // 18
    ReducedCell::I_CENTERED(), // 19
    ReducedCell::C_CENTERED(), // 20

    ReducedCell::P_CENTERED(), // 21
    ReducedCell::P_CENTERED(), // 22
    ReducedCell::C_CENTERED(), // 23
    ReducedCell::R_CENTERED(), // 24
    ReducedCell::C_CENTERED(), // 25

    ReducedCell::F_CENTERED(), // 26
    ReducedCell::I_CENTERED(), // 27
    ReducedCell::C_CENTERED(), // 28
    ReducedCell::C_CENTERED(), // 29
    ReducedCell::C_CENTERED(), // 30

    ReducedCell::P_CENTERED(), // 31
    ReducedCell::P_CENTERED(), // 32
    ReducedCell::P_CENTERED(), // 33
    ReducedCell::P_CENTERED(), // 34
    ReducedCell::P_CENTERED(), // 35

    ReducedCell::C_CENTERED(), // 36
    ReducedCell::C_CENTERED(), // 37
    ReducedCell::C_CENTERED(), // 38
    ReducedCell::C_CENTERED(), // 39
    ReducedCell::C_CENTERED(), // 40

    ReducedCell::C_CENTERED(),  // 41
    ReducedCell::I_CENTERED(),  // 42
    ReducedCell::I_CENTERED(),  // 43
    ReducedCell::P_CENTERED()}; // 44

/**
 *  Construct a ReducedCell object representing the specified row of
 *  Table 2 for a reduced cell with the specified lattice parameters,
 *  if the form number is between 1 and 44 inclusive.  If the form number
 *  is specified to be zero, the scalar values will be calculated according
 *  to the column headers for Table 2, for comparison purposes.
 *
 *  @param  form_num  The row number from Table 2, that specifies the
 *                    reduced form number.
 *  @param  a         Real space unit cell length "a".
 *  @param  b         Real space unit cell length "b".
 *  @param  c         Real space unit cell length "c".
 *  @param  alpha     Resl space unit cell angle "alpha", in degrees.
 *  @param  beta      Resl space unit cell angle "beta", in degrees.
 *  @param  gamma     Resl space unit cell angle "gamma", in degrees.
 */
ReducedCell::ReducedCell(size_t form_num, double a, double b, double c,
                         double alpha, double beta, double gamma) {
  if (a <= 0 || b <= 0 || c <= 0) {
    throw std::invalid_argument("ReducedCell(): a, b, c, must be positive");
  }
  if (alpha <= 0 || alpha >= 180 || beta <= 0 || beta >= 180 || gamma <= 0 ||
      gamma >= 180) {
    throw std::invalid_argument(
        "ReducedCell(): alpha, beta, gamma, must be between 0 and 180 degrees");
  }

  alpha = alpha * M_PI / 180;
  beta = beta * M_PI / 180;
  gamma = gamma * M_PI / 180;

  init(form_num, a * a, b * b, c * c, b * c * cos(alpha), a * c * cos(beta),
       a * b * cos(gamma));
}

/**
 *  Initialize all private data to represent one row of Table 2, for the
 *  row specified by the form number and for the given lattice parameters.
 *  The form number must be between 1 and 44 to represent an actual row of
 *  the table and must be 0 to represent the column header scalars, for
 *  comparison purposes.
 */
void ReducedCell::init(size_t f_num, double a_a, double b_b, double c_c,
                       double b_c, double a_c, double a_b) {
  if (f_num > NUM_CELL_TYPES) {
    throw std::invalid_argument("Reduced form number must be no more than 44");
  }
  // The mixed dot products should be > 0 for + cell
  // types and always appear inside absolute value
  // for - cell types, therefore we can deal with
  // the absolute value for all rows in the table.
  if (f_num > 0) {
    b_c = fabs(b_c);
    a_c = fabs(a_c);
    a_b = fabs(a_b);
  }

  form_num = f_num;

  transform = DblMatrix(3, 3, false);
  for (size_t row = 0; row < 3; row++)
    for (size_t col = 0; col < 3; col++) {
      transform[row][col] = transforms[f_num][row][col];
    }
  cell_type = lattice_types[form_num];
  centering = center_types[form_num];

  if (form_num == 0) {
    scalars[0] = a_a;
    scalars[1] = b_b;
    scalars[2] = c_c;
  } else if (form_num <= 8) {
    scalars[0] = a_a;
    scalars[1] = a_a;
    scalars[2] = a_a;
  } else if (form_num <= 17) {
    scalars[0] = a_a;
    scalars[1] = a_a;
    scalars[2] = c_c;
  } else if (form_num <= 25) {
    scalars[0] = a_a;
    scalars[1] = b_b;
    scalars[2] = b_b;
  } else {
    scalars[0] = a_a;
    scalars[1] = b_b;
    scalars[2] = c_c;
  }

  double value;
  switch (form_num) {
  case 0:
    scalars[3] = b_c;
    scalars[4] = a_c;
    scalars[5] = a_b;
    break;
  case 1:
    scalars[3] = a_a / 2;
    scalars[4] = a_a / 2;
    scalars[5] = a_a / 2;
    break;
  case 2:
    scalars[3] = b_c;
    scalars[4] = b_c;
    scalars[5] = b_c;
    break;
  case 3:
    scalars[3] = 0;
    scalars[4] = 0;
    scalars[5] = 0;
    break;
  case 4:
    value = -fabs(b_c);
    scalars[3] = value;
    scalars[4] = value;
    scalars[5] = value;
    break;
  case 5:
    scalars[3] = -a_a / 3;
    scalars[4] = -a_a / 3;
    scalars[5] = -a_a / 3;
    break;
  case 6:
    value = (-a_a + fabs(a_b)) / 2;
    scalars[3] = value;
    scalars[4] = value;
    scalars[5] = -fabs(a_b);
    ;
    break;
  case 7:
    value = (-a_a + fabs(b_c)) / 2;
    scalars[3] = -fabs(b_c);
    scalars[4] = value;
    scalars[5] = value;
    break;
  case 8:
    scalars[3] = -fabs(b_c);
    scalars[4] = -fabs(a_c);
    scalars[5] = -(fabs(a_a) - fabs(b_c) - fabs(a_c));
    break;
  case 9:
    scalars[3] = a_a / 2;
    scalars[4] = a_a / 2;
    scalars[5] = a_a / 2;
    break;
  case 10:
    scalars[3] = b_c;
    scalars[4] = b_c;
    scalars[5] = a_b;
    foot_note_d(c_c, b_c);
    break;
  case 11:
    scalars[3] = 0;
    scalars[4] = 0;
    scalars[5] = 0;
    break;
  case 12:
    scalars[3] = 0;
    scalars[4] = 0;
    scalars[5] = -a_a / 2;
    break;
  case 13:
    scalars[3] = 0;
    scalars[4] = 0;
    scalars[5] = -fabs(a_b);
    break;
  case 14:
    value = -fabs(b_c);
    scalars[3] = value;
    scalars[4] = value;
    scalars[5] = -fabs(a_b);
    foot_note_d(c_c, b_c);
    break;
  case 15:
    scalars[3] = -a_a / 2;
    scalars[4] = -a_a / 2;
    scalars[5] = 0;
    break;
  case 16:
    value = -fabs(b_c);
    scalars[3] = value;
    scalars[4] = value;
    scalars[5] = -(a_a - 2 * fabs(b_c));
    break;
  case 17:
    scalars[3] = -fabs(b_c);
    scalars[4] = -fabs(a_c);
    scalars[5] = -(a_a - fabs(b_c) - fabs(a_c));
    foot_note_e(a_a, c_c, a_c);
    break;
  case 18:
    scalars[3] = a_a / 4;
    scalars[4] = a_a / 2;
    scalars[5] = a_a / 2;
    break;
  case 19:
    scalars[3] = b_c;
    scalars[4] = a_a / 2;
    scalars[5] = a_a / 2;
    break;
  case 20:
    scalars[3] = b_c;
    scalars[4] = a_c;
    scalars[5] = a_c;
    foot_note_b(a_a, a_c);
    break;
  case 21:
    scalars[3] = 0;
    scalars[4] = 0;
    scalars[5] = 0;
    break;
  case 22:
    scalars[3] = -b_b / 2;
    scalars[4] = 0;
    scalars[5] = 0;
    break;
  case 23:
    scalars[3] = -fabs(b_c);
    scalars[4] = 0;
    scalars[5] = 0;
    break;
  case 24:
    scalars[3] = -(b_b - a_a / 3) / 2;
    scalars[4] = -a_a / 3;
    scalars[5] = -a_a / 3;
    break;
  case 25:
    value = -fabs(a_c);
    scalars[3] = -fabs(b_c);
    scalars[4] = value;
    scalars[5] = value;
    foot_note_b(a_a, a_c);
    break;
  case 26:
    scalars[3] = a_a / 4;
    scalars[4] = a_a / 2;
    scalars[5] = a_a / 2;
    break;
  case 27:
    scalars[3] = b_c;
    scalars[4] = a_a / 2;
    scalars[5] = a_a / 2;
    foot_note_f(b_b, c_c, b_c);
    break;
  case 28:
    scalars[3] = a_b / 2;
    scalars[4] = a_a / 2;
    scalars[5] = a_b;
    break;
  case 29:
    scalars[3] = a_c / 2;
    scalars[4] = a_c;
    scalars[5] = a_a / 2;
    break;
  case 30:
    scalars[3] = b_b / 2;
    scalars[4] = a_b / 2;
    scalars[5] = a_b;
    break;
  case 31:
    scalars[3] = b_c;
    scalars[4] = a_c;
    scalars[5] = a_b;
    break;
  case 32:
    scalars[3] = 0;
    scalars[4] = 0;
    scalars[5] = 0;
    break;
  case 33:
    scalars[3] = 0;
    scalars[4] = -fabs(a_c);
    scalars[5] = 0;
    break;
  case 34:
    scalars[3] = 0;
    scalars[4] = 0;
    scalars[5] = -fabs(a_b);
    break;
  case 35:
    scalars[3] = -fabs(b_c);
    scalars[4] = 0;
    scalars[5] = 0;
    break;
  case 36:
    scalars[3] = 0;
    scalars[4] = -a_a / 2;
    scalars[5] = 0;
    break;
  case 37:
    scalars[3] = -fabs(b_c);
    scalars[4] = -a_a / 2;
    scalars[5] = 0;
    foot_note_c(b_b, b_c);
    break;
  case 38:
    scalars[3] = 0;
    scalars[4] = 0;
    scalars[5] = -a_a / 2;
    break;
  case 39:
    scalars[3] = -fabs(b_c);
    scalars[4] = 0;
    scalars[5] = -a_a / 2;
    foot_note_d(c_c, b_c);
    break;
  case 40:
    scalars[3] = -b_b / 2;
    scalars[4] = 0;
    scalars[5] = 0;
    break;
  case 41:
    scalars[3] = -b_b / 2;
    scalars[4] = -fabs(a_c);
    scalars[5] = 0;
    foot_note_b(a_a, a_c);
    break;
  case 42:
    scalars[3] = -b_b / 2;
    scalars[4] = -a_a / 2;
    scalars[5] = 0;
    break;
  case 43:
    scalars[3] = -(b_b - fabs(a_b)) / 2;
    scalars[4] = -(a_a - fabs(a_b)) / 2;
    scalars[5] = -fabs(a_b);
    break;
  case 44:
    scalars[3] = -fabs(b_c);
    scalars[4] = -fabs(a_c);
    scalars[5] = -fabs(a_b);
    break;
  }
}

/**
 *  Adjust tranform and centering according to foot note b of the paper
 */
void ReducedCell::foot_note_b(double a_a, double a_c) {
  if (a_a < 4 * fabs(a_c)) // foot note b
  {
    premultiply(0); // use matrix modification 0
    centering = I_CENTERED();
  }
}

/**
 *  Adjust tranform and centering according to foot note c of the paper
 */
void ReducedCell::foot_note_c(double b_b, double b_c) {
  if (b_b < 4 * fabs(b_c)) // foot note c
  {
    premultiply(0); // use matrix modification 0
    centering = I_CENTERED();
  }
}

/**
 *  Adjust tranform and centering according to foot note d of the paper
 */
void ReducedCell::foot_note_d(double c_c, double b_c) {
  if (c_c < 4 * fabs(b_c)) // foot note d
  {
    premultiply(0); // use matrix modification 0
    centering = I_CENTERED();
  }
}

/**
 *  Adjust tranform and centering according to foot note e of the paper
 */
void ReducedCell::foot_note_e(double a_a, double c_c, double a_c) {
  if (3 * a_a < c_c + 2 * fabs(a_c)) // foot note e
  {
    premultiply(1); // use matrix modification 1
    centering = C_CENTERED();
  }
}

/**
 *  Adjust tranform and centering according to foot note f of the paper
 */
void ReducedCell::foot_note_f(double b_b, double c_c, double b_c) {
  if (3 * b_b < c_c + 2 * fabs(b_c)) // foot note f
  {
    premultiply(1); // use matrix modification 1
    centering = C_CENTERED();
  }
}

/**
 * Adjust the tranformation for this reduced cell by premultiplying
 * by modification transform 0 or 1.
 */
void ReducedCell::premultiply(size_t index) {
  DblMatrix modifier = DblMatrix(3, 3, false);
  for (size_t row = 0; row < 3; row++)
    for (size_t col = 0; col < 3; col++) {
      modifier[row][col] = transform_modifier[index][row][col];
    }
  transform = modifier * transform;
}

/**
 *  Get the form number used to construct this form
 */
size_t ReducedCell::GetFormNum() const { return form_num; }

/**
 *  Get the cell type of this form
 */
std::string ReducedCell::GetCellType() const { return std::string(cell_type); }

/**
 *  Get centering assigned to this form.  NOTE: This might be different
 *  from the requested cell type, since sometimes the centering is
 *  changed.  See the foot notes to Table 2 in the paper for details.
 */
std::string ReducedCell::GetCentering() const { return std::string(centering); }

/**
 * Get the maximum absolute weighted difference between the scalars
 * for the specifed ReducedCellInfo object and this ReducedCellInfo.
 * A fairly complicated weighting is used to make the effect of a
 * difference in cell edge length on lattice corner positions is
 * comparable to the effect of a difference in the angles.
 *
 * @param other  The ReducedCellInfo object to compare with the current
 *               object.
 *
 * @return  The maximum absolute difference between the scalars.
 */
double ReducedCell::WeightedDistance(const ReducedCell &other) const {
  std::vector<double> vals_1 = norm_vals(*this);
  std::vector<double> vals_2 = norm_vals(other);

  double max = 0;

  for (size_t i = 0; i < vals_1.size(); i++) {
    double difference = fabs(vals_1[i] - vals_2[i]);
    if (difference > max)
      max = difference;
  }
  return max;
}

/**
 * Get list of six values, related to the six scalars, but adjusted so
 * that changes in these values represent changes of positions of the
 * lattice corners of approximately the same magnitude.  This is useful
 * when comparing how close the lattice for one cell is to the lattice
 * for another cell.
 */
std::vector<double> ReducedCell::norm_vals(const ReducedCell &info) const {
  std::vector<double> vals;
  vals.resize(6);

  double a = sqrt(info.scalars[0]);
  double b = sqrt(info.scalars[1]);
  double c = sqrt(info.scalars[2]);

  // Use the side lengths themselves, instead of squares of sides
  // so errors correspond to errors in lattice positions
  vals[0] = a;
  vals[1] = b;
  vals[2] = c;
  // Use law of cosines to interpret errors in dot products
  // interms of errors in lattice positions.
  vals[3] = sqrt((b * b + c * c - 2 * info.scalars[3]));
  vals[4] = sqrt((a * a + c * c - 2 * info.scalars[4]));
  vals[5] = sqrt((a * a + b * b - 2 * info.scalars[5]));
  return vals;
}

/**
 *  Return the transformation to map the reduced cell to the conventional
 *  cell, as listed in Table 2, if the form number is between 1 and 44.
 *  If the form number is 0, this returns the identity transformation.
 *
 *  @return The transformation.
 */
Kernel::DblMatrix ReducedCell::GetTransformation() { return transform; }

} // namespace Mantid
} // namespace Geometry
