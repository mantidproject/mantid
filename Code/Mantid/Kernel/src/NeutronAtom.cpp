/*
 * NeutronAtom.cpp
 *
 *, Created on: Oct 21, 2010
 *, , Author: pf9
 */
#include "MantidKernel/NeutronAtom.h"
#include <math.h> // this defines NAN

using std::string;

namespace Mantid
{
namespace Kernel
{

NeutronAtom::NeutronAtom(const uint8_t z,
                         const double coh_b_real, const double inc_b_real,
                         const double coh_xs, const double inc_xs,
                         const double tot_xs, const double abs_xs):
                         z_number(z), a_number(0),
                         coh_scatt_length_real(coh_b_real), coh_scatt_length_img(0.),
                         inc_scatt_length_real(inc_b_real), inc_scatt_length_img(0.),
                         coh_scatt_xs(coh_xs), inc_scatt_xs(inc_xs),
                         tot_scatt_xs(tot_xs), abs_scatt_xs(abs_xs) {}

NeutronAtom::NeutronAtom(const uint8_t z, const uint8_t a,
                         const double coh_b_real, const double inc_b_real,
                         const double coh_xs, const double inc_xs,
                         const double tot_xs, const double abs_xs):
                         z_number(z), a_number(a),
                         coh_scatt_length_real(coh_b_real), coh_scatt_length_img(0.),
                         inc_scatt_length_real(inc_b_real), inc_scatt_length_img(0.),
                         coh_scatt_xs(coh_xs), inc_scatt_xs(inc_xs),
                         tot_scatt_xs(tot_xs), abs_scatt_xs(abs_xs) {}

NeutronAtom::NeutronAtom(const uint8_t z, const uint8_t a,
                         const double coh_b_real, const double coh_b_img,
                         const double inc_b_real, const double inc_b_img,
                         const double coh_xs, const double inc_xs,
                         const double tot_xs, const double abs_xs):
                         z_number(z), a_number(a),
                         coh_scatt_length_real(coh_b_real), coh_scatt_length_img(coh_b_img),
                         inc_scatt_length_real(inc_b_real), inc_scatt_length_img(inc_b_img),
                         coh_scatt_xs(coh_xs), inc_scatt_xs(inc_xs),
                         tot_scatt_xs(tot_xs), abs_scatt_xs(abs_xs) {}

static const NeutronAtom H( 1, -3.7390, 0., 1.7568, 80.26, 82.02, 0.3326);
static const NeutronAtom H1(1, 1, -3.7406, 25.274, 1.7583, 80.27, 82.03, 0.3326);
static const NeutronAtom H2(1, 2, 6.671, 4.04, 5.592, 2.05, 7.64, 0.000519);
static const NeutronAtom H3(1, 3, 4.792, -1.04, 2.89, 0.14, 3.03, 0.);
static const NeutronAtom He(2, 3.26, 0., 1.34, 0., 1.34, 0.00747);
static const NeutronAtom He3(2, 3, 5.74, -1.483, -2.5, 2.568, 4.42, 1.6, 6, 5333.);
static const NeutronAtom He4(2, 4, 3.26, 0., 1.34, 0., 1.34, 0.);
static const NeutronAtom Li(3, -1.90, 0., 0.454, 0.92, 1.37, 70.5);
static const NeutronAtom Li6(3, 6, 2.00, -0.261, -1.89, 0.26, 0.51, 0.46, 0.97, 940.);
static const NeutronAtom Li7(3, 7, -2.22, -2.49, 0.619, 0.78, 1.4, 0.0454);
static const NeutronAtom Be(4, 7.79, 0.12, 7.63, 0.0018, 7.63, 0.0076);
static const NeutronAtom B(5, 0, 5.30, -0.213, 0., 0., 3.54, 1.7, 5.24, 767.);
static const NeutronAtom B10(5, 10, -0.1, -1.066, -4.7, 1.231, 0.144, 3, 3.1, 3835.);
static const NeutronAtom B11(5, 11, 6.65, -1.3, 5.56, 0.21, 5.77, 0.0055);
static const NeutronAtom C(6, 6.6460, 0., 5.551, 0.001, 5.551, 0.0035);
static const NeutronAtom C12(6, 12, 6.6511, 0., 5.559, 0., 5.559, 0.00353);
static const NeutronAtom C13(6, 13, 6.19, -0.52, 4.81, 0.034, 4.84, 0.00137);
static const NeutronAtom N(7, 9.36, 0., 11.01, 0.5, 11.51, 1.9);
static const NeutronAtom N14(7, 14, 9.37, 2.0, 11.03, 0.5, 11.53, 1.91);
static const NeutronAtom N15(7, 15, 6.44, -0.02, 5.21, 0.00005, 5.21, 0.000024);
static const NeutronAtom O(8, 5.803, 0., 4.232, 0.0008, 4.232, 0.00019);
static const NeutronAtom O16(8, 16, 5.803, 0., 4.232, 0., 4.232, 0.0001);
static const NeutronAtom O17(8, 17, 5.78, 0.18, 4.2, 0.004, 4.2, 0.236);
static const NeutronAtom O18(8, 18, 5.84, 0., 4.29, 0., 4.29, 0.00016);
static const NeutronAtom F(9, 5.654, -0.082, 4.017, 0.0008, 4.018, 0.0096);
static const NeutronAtom Ne(10, 4.566, 0., 2.62, 0.008, 2.628, 0.039);
static const NeutronAtom Ne20(10, 20, 4.631, 0., 2.695, 0., 2.695, 0.036);
static const NeutronAtom Ne21(10, 21, 6.66, NAN, 5.6, 0.05, 5.7, 0.67); // TODO (+/-)0.6
static const NeutronAtom Ne22(10, 22, 3.87, 0., 1.88, 0., 1.88, 0.046);
static const NeutronAtom Na(11, 3.63, 3.59, 1.66, 1.62, 3.28, 0.53);
static const NeutronAtom Mg(12, 5.375, 0., 3.631, 0.08, 3.71, 0.063);
static const NeutronAtom Mg24(12, 24, 5.66, 0., 4.03, 0., 4.03, 0.05);
static const NeutronAtom Mg25(12, 25, 3.62, 1.48, 1.65, 0.28, 1.93, 0.19);
static const NeutronAtom Mg26(12, 26, 4.89, 0., 3, 0., 3, 0.0382);
static const NeutronAtom Al(13, 3.449, 0.256, 1.495, 0.0082, 1.503, 0.231);
static const NeutronAtom Si(14, 4.1491, 0., 2.163, 0.004, 2.167, 0.171);
static const NeutronAtom Si28(14, 28, 4.107, 0., 2.12, 0., 2.12, 0.177);
static const NeutronAtom Si29(14, 29, 4.70, 0.09, 2.78, 0.001, 2.78, 0.101);
static const NeutronAtom Si30(14, 30, 4.58, 0., 2.64, 0., 2.64, 0.107);
static const NeutronAtom P(15, 5.13, 0.2, 3.307, 0.005, 3.312, 0.172);
static const NeutronAtom S(16, 2.847, 0., 1.0186, 0.007, 1.026, 0.53);
static const NeutronAtom S32(16, 32, 2.804, 0., 0.988, 0., 0.988, 0.54);
static const NeutronAtom S33(16, 33, 4.74, 1.5, 2.8, 0.3, 3.1, 0.54);
static const NeutronAtom S34(16, 34, 3.48, 0., 1.52, 0., 1.52, 0.227);
static const NeutronAtom S36(16, 36, 3., 0., 1.1, 0., 1.1, 0.15);
static const NeutronAtom Cl(17, 9.5770, 0., 11.5257, 5.3, 16.8, 33.5);
static const NeutronAtom Cl35(17, 35, 11.65, 6.1, 17.06, 4.7, 21.8, 44.1);
static const NeutronAtom Cl37(17, 37, 3.08, 0.1, 1.19, 0.001, 1.19, 0.433);
static const NeutronAtom Ar(18, 1.909, 0., 0.458, 0.225, 0.683, 0.675);
static const NeutronAtom Ar36(18, 36, 24.90, 0., 77.9, 0., 77.9, 5.2);
static const NeutronAtom Ar38(18, 38, 3.5, 0., 1.5, 0., 1.5, 0.8);
static const NeutronAtom Ar40(18, 40, 1.830, 0., 0.421, 0., 0.421, 0.66);
static const NeutronAtom K(19, 3.67, 0., 1.69, 0.27, 1.96, 2.1);
static const NeutronAtom K39(19, 39, 3.74, 1.4, 1.76, 0.25, 2.01, 2.1);
static const NeutronAtom K40(19, 40, 3., 0., 1.1, 0.5, 1.6, 35.);
static const NeutronAtom K41(19, 41, 2.69, 1.5, 0.91, 0.3, 1.2, 1.46);
static const NeutronAtom Ca(20, 4.70, 0., 2.78, 0.05, 2.83, 0.43);
static const NeutronAtom Ca40(20, 40, 4.80, 0., 2.9, 0., 2.9, 0.41);
static const NeutronAtom Ca42(20, 42, 3.36, 0., 1.42, 0., 1.42, 0.68);
static const NeutronAtom Ca43(20, 43, -1.56, 0., 0.31, 0.5, 0.8, 6.2);
static const NeutronAtom Ca44(20, 44, 1.42, 0., 0.25, 0., 0.25, 0.88);
static const NeutronAtom Ca46(20, 46, 3.6, 0., 1.6, 0., 1.6, 0.74);
static const NeutronAtom Ca48(20, 48, 0.39, 0., 0.019, 0., 0.019, 1.09);
static const NeutronAtom Sc(21, 12.29, -6.0, 19, 4.5, 23.5, 27.5);
static const NeutronAtom Ti(22, -3.438, 0., 1.485, 2.87, 4.35, 6.09);
static const NeutronAtom Ti46(22, 46, 4.93, 0., 3.05, 0., 3.05, 0.59);
static const NeutronAtom Ti47(22, 47, 3.63, -3.5, 1.66, 1.5, 3.2, 1.7);
static const NeutronAtom Ti48(22, 48, -6.08, 0., 4.65, 0., 4.65, 7.84);
static const NeutronAtom Ti49(22, 49, 1.04, 5.1, 0.14, 3.3, 3.4, 2.2);
static const NeutronAtom Ti50(22, 50, 6.18, 0., 4.8, 0., 4.8, 0.179);
static const NeutronAtom V(23, -0.3824, 0., 0.0184, 5.08, 5.1, 5.08);
static const NeutronAtom V50(23, 50, 7.6, 0., 7.3, 0.5, 7.8, 60.);
static const NeutronAtom V51(23, 51, -0.402, 6.35, 0.0203, 5.07, 5.09, 4.9);
static const NeutronAtom Cr(24, 3.635, 0., 1.66, 1.83, 3.49, 3.05);
static const NeutronAtom Cr50(24, 50, -4.50, 0., 2.54, 0., 2.54, 15.8);
static const NeutronAtom Cr52(24, 52, 4.920, 0., 3.042, 0., 3.042, 0.76);
static const NeutronAtom Cr53(24, 53, -4.20, 6.87, 2.22, 5.93, 8.15, 18.1);
static const NeutronAtom Cr54(24, 54, 4.55, 0., 2.6, 0., 2.6, 0.36);
static const NeutronAtom Mn(25, -3.73, 1.79, 1.75, 0.4, 2.15, 13.3);
static const NeutronAtom Fe(26, 9.45, 0., 11.22, 0.4, 11.62, 2.56);
static const NeutronAtom Fe54(26, 54, 4.2, 0., 2.2, 0., 2.2, 2.25);
static const NeutronAtom Fe56(26, 56, 9.94, 0., 12.42, 0., 12.42, 2.59);
static const NeutronAtom Fe57(26, 57, 2.3, 0., 0.66, 0.3, 1, 2.48);
static const NeutronAtom Fe58(26, 58, 15., 0., 28, 0., 28., 1.28);
static const NeutronAtom Co(27, 2.49, -6.2, 0.779, 4.8, 5.6, 37.18);
static const NeutronAtom Ni(28, 10.3, 0., 13.3, 5.2, 18.5, 4.49);
static const NeutronAtom Ni58(28, 58, 14.4, 0., 26.1, 0., 26.1, 4.6);
static const NeutronAtom Ni60(28, 60,  2.8, 0., 0.99, 0., 0.99, 2.9);
static const NeutronAtom Ni61(28, 61, 7.60, NAN, 7.26, 1.9, 9.2, 2.5); // TODO (+/-)3.9
static const NeutronAtom Ni62(28, 62, -8.7, 0., 9.5, 0., 9.5, 14.5);
static const NeutronAtom Ni64(28, 64, -0.37, 0., 0.017, 0., 0.017, 1.52);
static const NeutronAtom Cu(29, 7.718, 0., 7.485, 0.55, 8.03, 3.78);
static const NeutronAtom Cu63(29, 63, 6.43, 0.22, 5.2, 0.006, 5.2, 4.5);
static const NeutronAtom Cu65(29, 65, 10.61, 1.79, 14.1, 0.4, 14.5, 2.17);
static const NeutronAtom Zn(30, 5.680, 0., 4.054, 0.077, 4.131, 1.11);
static const NeutronAtom Zn64(30, 64, 5.22, 0., 3.42, 0., 3.42, 0.93);
static const NeutronAtom Zn66(30, 66, 5.97, 0., 4.48, 0., 4.48, 0.62);
static const NeutronAtom Zn67(30, 67, 7.56, -1.50, 7.18, 0.28, 7.46, 6.8);
static const NeutronAtom Zn68(30, 68, 6.03, 0., 4.57, 0., 4.57, 1.1);
static const NeutronAtom Zn70(30, 70, 6., 0., 4.5, 0., 4.5, 0.092);
static const NeutronAtom Ga(31, 7.288, 0., 6.675, 0.16, 6.83, 2.75);
static const NeutronAtom Ga69(31, 69, 7.88, -0.85, 7.8, 0.091, 7.89, 2.18);
static const NeutronAtom Ga71(31, 71, 6.40, -0.82, 5.15, 0.084, 5.23, 3.61);
static const NeutronAtom Ge(32, 8.185, 0., 8.42, 0.18, 8.6, 2.2);
static const NeutronAtom Ge70(32, 70, 10.0, 0., 12.6, 0., 12.6, 3);
static const NeutronAtom Ge72(32, 72, 8.51, 0., 9.1, 0., 9.1, 0.8);
static const NeutronAtom Ge73(32, 73, 5.02, 3.4, 3.17, 1.5, 4.7, 15.1);
static const NeutronAtom Ge74(32, 74, 7.58, 0., 7.2, 0., 7.2, 0.4);
static const NeutronAtom Ge76(32, 76, 8.2, 0., 8., 0., 8., 0.16);
static const NeutronAtom As(33, 6.58, -0.69, 5.44, 0.06, 5.5, 4.5);
static const NeutronAtom Se(34, 7.970, 0., 7.98, 0.32, 8.3, 11.7);
static const NeutronAtom Se74(34, 74, 0.8, 0., 0.1, 0., 0.1, 51.8);
static const NeutronAtom Se76(34, 76, 12.2, 0., 18.7, 0., 18.7, 85.);
static const NeutronAtom Se77(34, 77, 8.25, NAN, 8.6, 0.05, 8.65, 42.); // TODO (+/-)0.6(1.6)
static const NeutronAtom Se78(34, 78, 8.24, 0., 8.5, 0., 8.5, 0.43);
static const NeutronAtom Se80(34, 80, 7.48, 0., 7.03, 0., 7.03, 0.61);
static const NeutronAtom Se82(34, 82, 6.34, 0., 5.05, 0., 5.05, 0.044);
static const NeutronAtom Br(35, 6.795, 0., 5.8, 0.1, 5.9, 6.9);
static const NeutronAtom Br79(35, 79, 6.80, -1.1, 5.81, 0.15, 5.96, 11);
static const NeutronAtom Br81(35, 81, 6.79, 0.6, 5.79, 0.05, 5.84, 2.7);
static const NeutronAtom Kr(36, 7.81, 0., 7.67, 0.01, 7.68, 25.);
static const NeutronAtom Kr78(36, 78, 0., 0., 0., 0., 0., 6.4);
static const NeutronAtom Kr80(36, 80, 0., 0., 0., 0., 0., 11.8);
static const NeutronAtom Kr82(36, 82, 0., 0., 0., 0., 0., 29.);
static const NeutronAtom Kr83(36, 83, 0., 0., 0., 0., 0., 185.);
static const NeutronAtom Kr84(36, 84, 0., 0., 0., 0., 6.6, 0.113);
static const NeutronAtom Kr86(36, 86, 8.1, 0., 8.2, 0., 8.2, 0.003);
static const NeutronAtom Rb(37, 7.09, 0., 6.32, 0.5, 6.8, 0.38);
static const NeutronAtom Rb85(37, 85, 7.03, 0., 6.2, 0.5, 6.7, 0.48);
static const NeutronAtom Rb87(37, 87, 7.23, 0., 6.6, 0.5, 7.1, 0.12);
static const NeutronAtom Sr(38, 7.02, 0., 6.19, 0.06, 6.25, 1.28);
static const NeutronAtom Sr84(38, 84, 7., 0., 6., 0., 6., 0.87);
static const NeutronAtom Sr86(38, 86, 5.67, 0., 4.04, 0., 4.04, 1.04);
static const NeutronAtom Sr87(38, 87, 7.40, 0., 6.88, 0.5, 7.4, 16.);
static const NeutronAtom Sr88(38, 88, 7.15, 0., 6.42, 0., 6.42, 0.058);
static const NeutronAtom Y(39, 7.75, 1.1, 7.55, 0.15, 7.7, 1.28);
static const NeutronAtom Zr(40, 7.16, 0., 6.44, 0.02, 6.46, 0.185);
static const NeutronAtom Zr90(40, 90, 6.4, 0., 5.1, 0., 5.1, 0.011);
static const NeutronAtom Zr91(40, 91, 8.7, -1.08, 9.5, 0.15, 9.7, 1.17);
static const NeutronAtom Zr92(40, 92, 7.4, 0., 6.9, 0., 6.9, 0.22);
static const NeutronAtom Zr94(40, 94, 8.2, 0., 8.4, 0., 8.4, 0.0499);
static const NeutronAtom Zr96(40, 96, 5.5, 0., 3.8, 0., 3.8, 0.0229);
static const NeutronAtom Nb(41, 7.054, -0.139, 6.253, 0.0024, 6.255, 1.15);
static const NeutronAtom Mo(42, 6.715, 0., 5.67, 0.04, 5.71, 2.48);
static const NeutronAtom Mo92(42, 92, 6.91, 0., 6, 0., 6, 0.019);
static const NeutronAtom Mo94(42, 94, 6.80, 0., 5.81, 0., 5.81, 0.015);
static const NeutronAtom Mo95(42, 95, 6.91, 0., 6, 0.5, 6.5, 13.1);
static const NeutronAtom Mo96(42, 96, 6.20, 0., 4.83, 0., 4.83, 0.5);
static const NeutronAtom Mo97(42, 97, 7.24, 0., 6.59, 0.5, 7.1, 2.5);
static const NeutronAtom Mo98(42, 98, 6.58, 0., 5.44, 0., 5.44, 0.127);
static const NeutronAtom Mo100(42, 100, 6.73, 0., 5.69, 0., 5.69, 0.4);
static const NeutronAtom Tc(43, 6.8, 0., 5.8, 0.5, 6.3, 20.);
static const NeutronAtom Ru(44, 7.03, 0., 6.21, 0.4, 6.6, 2.56);
static const NeutronAtom Ru96(44, 96, 0., 0., 0., 0., 0., 0.28);
static const NeutronAtom Ru98(44, 98, 0., 0., 0., 0., 0., NAN); // TODO <8.
static const NeutronAtom Ru99(44, 99, 0., 0., 0., 0., 0., 6.9);
static const NeutronAtom Ru100(44, 100, 0., 0., 0., 0., 0., 4.8);
static const NeutronAtom Ru101(44, 101, 0., 0., 0., 0., 0., 3.3);
static const NeutronAtom Ru102(44, 102, 0., 0., 0., 0., 144.8, 1.17);
static const NeutronAtom Ru104(44, 104, 0., 0., 0., 0., 4.483, 0.31);
static const NeutronAtom Rh(45, 5.88, 0., 4.34, 0.3, 4.6, 144.8);
static const NeutronAtom Pd(46, 5.91, 0., 4.39, 0.093, 4.48, 6.9);
static const NeutronAtom Pd102(46, 102, 7.7, 0., 7.5, 0., 7.5, 3.4);
static const NeutronAtom Pd104(46, 104, 7.7, 0., 7.5, 0., 7.5, 0.6);
static const NeutronAtom Pd105(46, 105, 5.5, -2.6, 3.8, 0.8, 4.6, 20.);
static const NeutronAtom Pd106(46, 106, 6.4, 0., 5.1, 0., 5.1, 0.304);
static const NeutronAtom Pd108(46, 108, 4.1, 0., 2.1, 0., 2.1, 8.55);
static const NeutronAtom Pd110(46, 110, 7.7, 0., 7.5, 0., 7.5, 0.226);
static const NeutronAtom Ag(47, 5.922, 0., 4.407, 0.58, 4.99, 63.3);
static const NeutronAtom Ag107(47, 107, 7.555, 1.00, 7.17, 0.13, 7.3, 37.6);
static const NeutronAtom Ag109(47, 109, 4.165, -1.60, 2.18, 0.32, 2.5, 91.0);
static const NeutronAtom Cd(48, 0, 4.87,-0.70, 0., 0., 3.04, 3.46, 6.5, 2520.);
static const NeutronAtom Cd106(48, 106, 5., 0., 3.1, 0., 3.1, 1);
static const NeutronAtom Cd108(48, 108, 5.4, 0., 3.7, 0., 3.7, 1.1);
static const NeutronAtom Cd110(48, 110, 5.9, 0., 4.4, 0., 4.4, 11);
static const NeutronAtom Cd111(48, 111, 6.5, 0., 5.3, 0.3, 5.6, 24);
static const NeutronAtom Cd112(48, 112, 6.4, 0., 5.1, 0., 5.1, 2.2);
static const NeutronAtom Cd113(48, 113, -8.0, -5.73, 0., 0., 12.1, 0.3, 12.4, 20600.);
static const NeutronAtom Cd114(48, 114, 7.5, 0., 7.1, 0., 7.1, 0.34);
static const NeutronAtom Cd116(48, 116, 6.3, 0., 5, 0., 5, 0.075);
static const NeutronAtom In(49, 0, 4.065, -0.0539, 0., 0., 2.08, 0.54, 2.62, 193.8);
static const NeutronAtom In113(49, 113, 5.39, NAN, 3.65, 0.000037, 3.65, 12.0); // TODO (+/-)0.017
static const NeutronAtom In115(49, 115, 4.01, -0.0562, -2.1, 0., 2.02, 0.55, 2.57, 202.);
static const NeutronAtom Sn(50, 6.225, 0., 4.871, 0.022, 4.892, 0.626);
static const NeutronAtom Sn112(50, 112, 6., 0., 4.5, 0., 4.5, 1);
static const NeutronAtom Sn114(50, 114, 6.2, 0., 4.8, 0., 4.8, 0.114);
static const NeutronAtom Sn115(50, 115, 6., 0., 4.5, 0.3, 4.8, 30.);
static const NeutronAtom Sn116(50, 116, 5.93, 0., 4.42, 0., 4.42, 0.14);
static const NeutronAtom Sn117(50, 117, 6.48, 0., 5.28, 0.3, 5.6, 2.3);
static const NeutronAtom Sn118(50, 118, 6.07, 0., 4.63, 0., 4.63, 0.22);
static const NeutronAtom Sn119(50, 119, 6.12, 0., 4.71, 0.3, 5, 2.2);
static const NeutronAtom Sn120(50, 120, 6.49, 0., 5.29, 0., 5.29, 0.14);
static const NeutronAtom Sn122(50, 122, 5.74, 0., 4.14, 0., 4.14, 0.18);
static const NeutronAtom Sn124(50, 124, 5.97, 0., 4.48, 0., 4.48, 0.133);
static const NeutronAtom Sb(51, 5.57, 0., 3.9, 0.007, 3.9, 4.91);
static const NeutronAtom Sb121(51, 121, 5.71, -0.05, 4.1, 0.0003, 4.1, 5.75);
static const NeutronAtom Sb123(51, 123, 5.38, -0.10, 3.64, 0.001, 3.64, 3.8);
static const NeutronAtom Te(52, 5.80, 0., 4.23, 0.09, 4.32, 4.7);
static const NeutronAtom Te120(52, 120, 5.3, 0., 3.5, 0., 3.5, 2.3);
static const NeutronAtom Te122(52, 122, 3.8, 0., 1.8, 0., 1.8, 3.4);
static const NeutronAtom Te123(52, 123, -0.05, -0.116, -2.04, 0., 0.002, 0.52, 0.52, 418.);
static const NeutronAtom Te124(52, 124, 7.96, 0., 8, 0., 8, 6.8);
static const NeutronAtom Te125(52, 125, 5.02, -0.26, 3.17, 0.008, 3.18, 1.55);
static const NeutronAtom Te126(52, 126, 5.56, 0., 3.88, 0., 3.88, 1.04);
static const NeutronAtom Te128(52, 128, 5.89, 0., 4.36, 0., 4.36, 0.215);
static const NeutronAtom Te130(52, 130, 6.02, 0., 4.55, 0., 4.55, 0.29);
static const NeutronAtom I(53, 5.28, 1.58, 3.5, 0.31, 3.81, 6.15);
static const NeutronAtom Xe(54, 4.92, 3.04, 2.96, 0., 0., 23.9);
static const NeutronAtom Xe124(54, 124, 0., 0., 0., 0., 0., 165.);
static const NeutronAtom Xe126(54, 126, 0., 0., 0., 0., 0., 3.5);
static const NeutronAtom Xe128(54, 128, 0., 0., 0., 0., 0., NAN); // TODO <8
static const NeutronAtom Xe129(54, 129, 0., 0., 0., 0., 0., 21.);
static const NeutronAtom Xe130(54, 130, 0., 0., 0., 0., 0., NAN); // TODO <26.
static const NeutronAtom Xe131(54, 131, 0., 0., 0., 0., 0., 85.);
static const NeutronAtom Xe132(54, 132, 0., 0., 0., 0., 0., 0.45);
static const NeutronAtom Xe134(54, 134, 0., 0., 0., 0., 0., 0.265);
static const NeutronAtom Xe136(54, 136, 0., 0., 0., 0., 0., 0.26);
static const NeutronAtom Cs(55, 5.42, 1.29, 3.69, 0.21, 3.9, 29.0);
static const NeutronAtom Ba(56, 5.07, 0., 3.23, 0.15, 3.38, 1.1);
static const NeutronAtom Ba130(56, 130, -3.6, 0., 1.6, 0., 1.6, 30.);
static const NeutronAtom Ba132(56, 132, 7.8, 0., 7.6, 0., 7.6, 7);
static const NeutronAtom Ba134(56, 134, 5.7, 0., 4.08, 0., 4.08, 2.0);
static const NeutronAtom Ba135(56, 135, 4.67, 0., 2.74, 0.5, 3.2, 5.8);
static const NeutronAtom Ba136(56, 136, 4.91, 0., 3.03, 0., 3.03, 0.68);
static const NeutronAtom Ba137(56, 137, 6.83, 0., 5.86, 0.5, 6.4, 3.6);
static const NeutronAtom Ba138(56, 138, 4.84, 0., 2.94, 0., 2.94, 0.27);
static const NeutronAtom La(57, 8.24, 0., 8.53, 1.13, 9.66, 8.97);
static const NeutronAtom La138(57, 138, 8., 0., 8., 0.5, 8.5, 57.);
static const NeutronAtom La139(57, 139, 8.24, 3.0, 8.53, 1.13, 9.66, 8.93);
static const NeutronAtom Ce(58, 4.84, 0., 2.94, 0.001, 2.94, 0.63);
static const NeutronAtom Ce136(58, 136, 5.80, 0., 4.23, 0., 4.23, 7.3);
static const NeutronAtom Ce138(58, 138, 6.70, 0., 5.64, 0., 5.64, 1.1);
static const NeutronAtom Ce140(58, 140, 4.84, 0., 2.94, 0., 2.94, 0.57);
static const NeutronAtom Ce142(58, 142, 4.75, 0., 2.84, 0., 2.84, 0.95);
static const NeutronAtom Pr(59, 4.58, -0.35, 2.64, 0.015, 2.66, 11.5);
static const NeutronAtom Nd(60, 7.69, 0., 7.43, 9.2, 16.6, 50.5);
static const NeutronAtom Nd142(60, 142, 7.7, 0., 7.5, 0., 7.5, 18.7);
static const NeutronAtom Nd143(60, 143, 14., NAN, 25., 55., 80., 337.); // TODO (+/-)21.(1.)
static const NeutronAtom Nd144(60, 144, 2.8, 0., 1, 0., 1, 3.6);
static const NeutronAtom Nd145(60, 145, 14., 0., 25., 5., 30., 42.);
static const NeutronAtom Nd146(60, 146, 8.7, 0., 9.5, 0., 9.5, 1.4);
static const NeutronAtom Nd148(60, 148, 5.7, 0., 4.1, 0., 4.1, 2.5);
static const NeutronAtom Nd150(60, 150, 5.3, 0., 3.5, 0., 3.5, 1.2);
static const NeutronAtom Pm(61, 12.6, NAN, 20.0, 1.3, 21.3, 168.4); // TODO (+/-)3.2(2.5)
static const NeutronAtom Sm(62, 0, 0.80, -1.65, 0., 0., 0.422, 39., 39., 5922.);
static const NeutronAtom Sm144(62, 144, -3., 0., 1., 0., 1., 0.7);
static const NeutronAtom Sm147(62, 147, 14., NAN, 25., 143, 39., 57.); // TODO (+/-)11.(7.)
static const NeutronAtom Sm148(62, 148, -3., 0., 1., 0., 1., 2.4);
static const NeutronAtom Sm149(62, 149, -19.2, -11.7, NAN, -10.3, 63.5, 137., 200., 42080.); // TODO (+/-)31.4
static const NeutronAtom Sm150(62, 150, 14., 0., 25., 0., 25., 104.);
static const NeutronAtom Sm152(62, 152, -5.0, 0., 3.1, 0., 3.1, 206.);
static const NeutronAtom Sm154(62, 154, 9.3, 0., 11., 0., 11., 8.4);
static const NeutronAtom Eu(63, 0, 7.22, -1.26, 0., 0., 6.57, 2.5, 9.2, 4530.);
static const NeutronAtom Eu151(63, 151, 6.13, -2.53, NAN, -2.14, 5.5, 3.1, 8.6, 9100.); // TODO (+/-)4.5
static const NeutronAtom Eu153(63, 153, 8.22, NAN, 8.5, 1.3, 9.8, 312.); // TODO (+/-)3.2
static const NeutronAtom Gd(64, 0, 6.5, -13.82, 0., 0., 29.3, 151., 180., 49700.);
static const NeutronAtom Gd152(64, 152, 10., 0., 13., 0., 13., 735.);
static const NeutronAtom Gd154(64, 154, 10., 0., 13., 0., 13., 85.);
static const NeutronAtom Gd155(64, 155, 6.0, -17.0, NAN, -13.16, 40.8, 25., 66., 61100.); // TODO (+/-)5.(5.)
static const NeutronAtom Gd156(64, 156, 6.3, 0., 5, 0., 5, 1.5);
static const NeutronAtom Gd157(64, 157, -1.14, -71.9, NAN, -55.8, 650., 394., 1044., 259000.); // TODO (+/-)5.(5.)
static const NeutronAtom Gd158(64, 158, 9., 0., 10., 0., 10., 2.2);
static const NeutronAtom Gd160(64, 160, 9.15, 0., 10.52, 0., 10.52, 0.77);
static const NeutronAtom Tb(65, 7.38, -0.17, 6.84, 0.004, 6.84, 23.4);
static const NeutronAtom Dy(66, 0, 16.9, -0.276, 0., 0., 35.9, 54.4, 90.3, 994.);
static const NeutronAtom Dy156(66, 156, 6.1, 0., 4.7, 0., 4.7, 33.);
static const NeutronAtom Dy158(66, 158, 6., 0., 5., 0., 5., 43.);
static const NeutronAtom Dy160(66, 160, 6.7, 0., 5.6, 0., 5.6, 56.);
static const NeutronAtom Dy161(66, 161, 10.3, NAN, 13.3, 3., 16., 600.); // TODO (+/-)4.9
static const NeutronAtom Dy162(66, 162, -1.4, 0., 0.25, 0., 0.25, 194.);
static const NeutronAtom Dy163(66, 163, 5.0, 1.3, 3.1, 0.21, 3.3, 124.);
static const NeutronAtom Dy164(66, 164, 49.4, -0.79, 0., 0., 307., 0., 307., 2840.);
static const NeutronAtom Ho(67, 8.01, -1.70, 8.06, 0.36, 8.42, 64.7);
static const NeutronAtom Er(68, 7.79, 0., 7.63, 1.1, 8.7, 159.);
static const NeutronAtom Er162(68, 162, 8.8, 0., 9.7, 0., 9.7, 19.);
static const NeutronAtom Er164(68, 164, 8.2, 0., 8.4, 0., 8.4, 13.);
static const NeutronAtom Er166(68, 166, 10.6, 0., 14.1, 0., 14.1, 19.6);
static const NeutronAtom Er167(68, 167, 3.0, 1.0, 1.1, 0.13, 1.2, 659.);
static const NeutronAtom Er168(68, 168, 7.4, 0., 6.9, 0., 6.9, 2.74);
static const NeutronAtom Er170(68, 170, 9.6, 0., 11.6, 0., 11.6, 5.8);
static const NeutronAtom Tm(69, 7.07, 0.9, 6.28, 0.1, 6.38, 100.);
static const NeutronAtom Yb(70, 12.43, 0., 19.42, 4, 23.4, 34.8);
static const NeutronAtom Yb168(70, 168, -4.07, -0.62, 0., 0., 2.13, 0., 2.13, 2230.);
static const NeutronAtom Yb170(70, 170, 6.77, 0., 5.8, 0., 5.8, 11.4);
static const NeutronAtom Yb171(70, 171, 9.66, -5.59, 11.7, 3.9, 15.6, 48.6);
static const NeutronAtom Yb172(70, 172, 9.43, 0., 11.2, 0., 11.2, 0.8);
static const NeutronAtom Yb173(70, 173, 9.56, -5.3, 11.5, 3.5, 15, 17.1);
static const NeutronAtom Yb174(70, 174, 19.3, 0., 46.8, 0., 46.8, 69.4);
static const NeutronAtom Yb176(70, 176, 8.72, 0., 9.6, 0., 9.6, 2.85);
static const NeutronAtom Lu(71, 7.21, 0., 6.53, 0.7, 7.2, 74.);
static const NeutronAtom Lu175(71, 175, 7.24, NAN, 6.59, 0.6, 7.2, 21.); // TODO (+/-)2.2
static const NeutronAtom Lu176(71, 175, 6.1, -0.57, NAN, 0.61, 4.7, 1.2, 5.9, 2065.); // TODO (+/-)3.0
static const NeutronAtom Hf(72, 7.7, 0., 7.6, 2.6, 10.2, 104.1);
static const NeutronAtom Hf174(72, 174, 10.9, 0., 15., 0., 15., 561.);
static const NeutronAtom Hf176(72, 176, 6.61, 0., 5.5, 0., 5.5, 23.5);
static const NeutronAtom Hf177(72, 177, 0.8, NAN, 0.1, 0.1, 0.2, 373.); // TODO (+/-)0.9(1.3)
static const NeutronAtom Hf178(72, 178, 5.9, 0., 4.4, 0., 4.4, 84.);
static const NeutronAtom Hf179(72, 179, 7.46, NAN, 7, 0.14, 7.1, 41.); // TODO (+/-)1.06
static const NeutronAtom Hf180(72, 180, 13.2, 0., 21.9, 0., 21.9, 13.04);
static const NeutronAtom Ta(73, 6.91, 0., 6, 0.01, 6.01, 20.6);
static const NeutronAtom Ta180(73, 180, 7., 0., 6.2, 0.5, 7., 563.);
static const NeutronAtom Ta181(73, 181, 6.91, -0.29, 6, 0.011, 6.01, 20.5);
static const NeutronAtom W(74, 4.86, 0., 2.97, 1.63, 4.6, 18.3);
static const NeutronAtom W180(74, 180, 5., 0., 3., 0., 3., 30.);
static const NeutronAtom W182(74, 182, 6.97, 0., 6.1, 0., 6.1, 20.7);
static const NeutronAtom W183(74, 183, 6.53, 0., 5.36, 0.3, 5.7, 10.1);
static const NeutronAtom W184(74, 184, 7.48, 0., 7.03, 0., 7.03, 1.7);
static const NeutronAtom W186(74, 186, -0.72, 0., 0.065, 0., 0.065, 37.9);
static const NeutronAtom Re(75, 9.2, 0., 10.6, 0.9, 11.5, 89.7);
static const NeutronAtom Re185(75, 185, 9.0, NAN, 10.2, 0.5, 10.7, 112.); // TODO (+/-)2.0
static const NeutronAtom Re187(75, 187, 9.3, NAN, 10.9, 1, 11.9, 76.4); // TODO (+/-)2.8
static const NeutronAtom Os(76, 10.7, 0., 14.4, 0.3, 14.7, 16);
static const NeutronAtom Os184(76, 184, 10., 0., 13., 0., 13., 3000.);
static const NeutronAtom Os186(76, 186, 11.6, 0., 17., 0., 17., 80.);
static const NeutronAtom Os187(76, 187, 10., 0., 13., 0.3, 13., 320.);
static const NeutronAtom Os188(76, 188, 7.6, 0., 7.3, 0., 7.3, 4.7);
static const NeutronAtom Os189(76, 189, 10.7, 0., 14.4, 0.5, 14.9, 25.);
static const NeutronAtom Os190(76, 190, 11.0, 0., 15.2, 0., 15.2, 13.1);
static const NeutronAtom Os192(76, 192, 11.5, 0., 16.6, 0., 16.6, 2);
static const NeutronAtom Ir(77, 10.6, 0., 14.1, 0., 14., 425.);
static const NeutronAtom Ir191(77, 191, 0., 0., 0., 0., 0., 954.);
static const NeutronAtom Ir193(77, 193, 0., 0., 0., 0., 0., 111.);
static const NeutronAtom Pt(78, 9.60, 0., 11.58, 0.13, 11.71, 10.3);
static const NeutronAtom Pt190(78, 190, 9.0, 0., 10., 0., 10., 152.);
static const NeutronAtom Pt192(78, 192, 9.9, 0., 12.3, 0., 12.3, 10.0);
static const NeutronAtom Pt194(78, 194, 10.55, 0., 14, 0., 14, 1.44);
static const NeutronAtom Pt195(78, 195, 8.83, -1.00, 9.8, 0.13, 9.9, 27.5);
static const NeutronAtom Pt196(78, 196, 9.89, 0., 12.3, 0., 12.3, 0.72);
static const NeutronAtom Pt198(78, 198, 7.8, 0., 7.6, 0., 7.6, 3.66);
static const NeutronAtom Au(79, 7.63, -1.84, 7.32, 0.43, 7.75, 98.65);
static const NeutronAtom Hg(80, 12.692, 0., 20.24, 6.6, 26.8, 372.3);
static const NeutronAtom Hg196(80, 196, 30.3, 0., 115., 0., 115., 3080.);
static const NeutronAtom Hg198(80, 198, 0., 0., 0., 0., 0., 2);
static const NeutronAtom Hg199(80, 199, 16.9, NAN, 36., 30., 66., 2150.); // TODO (+/-)15.5
static const NeutronAtom Hg200(80, 200, 0., 0., 0., 0., 0., NAN); // TODO <60.
static const NeutronAtom Hg201(80, 201, 0., 0., 0., 0., 0., 7.8);
static const NeutronAtom Hg202(80, 202, 0., 0., 0., 0., 9.828, 4.89);
static const NeutronAtom Hg204(80, 204, 0., 0., 0., 0., 0., 0.43);
static const NeutronAtom Tl(81, 8.776, 0., 9.678, 0.21, 9.89, 3.43);
static const NeutronAtom Tl203(81, 203, 6.99, 1.06, 6.14, 0.14, 6.28, 11.4);
static const NeutronAtom Tl205(81, 205, 9.52, -0.242, 11.39, 0.007, 11.4, 0.104);
static const NeutronAtom Pb(82, 9.405, 0., 11.115, 0.003, 11.118, 0.171);
static const NeutronAtom Pb204(82, 204, 9.90, 0., 12.3, 0., 12.3, 0.65);
static const NeutronAtom Pb206(82, 206, 9.22, 0., 10.68, 0., 10.68, 0.03);
static const NeutronAtom Pb207(82, 207, 9.28, 0.14, 10.82, 0.002, 10.82, 0.699);
static const NeutronAtom Pb208(82, 208, 9.50, 0., 11.34, 0., 11.34, 0.00048);
static const NeutronAtom Bi(83, 8.532, 0., 9.148, 0.0084, 9.156, 0.0338);
static const NeutronAtom Po(84, 0., 0.259, 0., 0., 0., 0.);
static const NeutronAtom At(85, 0., 0., 0., 0., 0., 0.);
static const NeutronAtom Rn(86, 0., 0., 0., 0., 12.6, 0.);
static const NeutronAtom Fr(87, 0., 0., 0., 0., 0., 0.);
static const NeutronAtom Ra(88, 10.0, 0., 13., 0., 13., 12.8);
static const NeutronAtom Ac(89, 0., 0., 0., 0., 0., 0.);
static const NeutronAtom Th(90, 10.31, 0., 13.36, 0., 13.36, 7.37);
static const NeutronAtom Pa(91, 9.1, 0., 10.4, 0.1, 10.5, 200.6);
static const NeutronAtom U(92, 8.417, 0., 8.903, 0.005, 8.908, 7.57);
static const NeutronAtom U233(92, 233, 10.1, NAN, 12.8, 0.1, 12.9, 574.7); // TODO (+/-)1.(3.)
static const NeutronAtom U234(92, 234, 12.4, 0., 19.3, 0., 19.3, 100.1);
static const NeutronAtom U235(92, 235, 10.47, NAN, 13.78, 0.2, 14, 680.9); // TODO (+/-)1.3
static const NeutronAtom U238(92, 238, 8.402, 0., 8.871, 0., 8.871, 2.68);
static const NeutronAtom Np(93, 10.55, 0., 14, 0.5, 14.5, 175.9);
static const NeutronAtom Pu(94, 0., 0., 0., 0., 0., 0.);
static const NeutronAtom Pu238(94, 238, 14.1, 0., 25.0, 0., 25.0, 558.);
static const NeutronAtom Pu239(94, 239, 7.7, NAN, 7.5, 0.2, 7.7, 1017.3); // TODO (+/-)1.3(1.9)
static const NeutronAtom Pu240(94, 240, 3.5, 0., 1.54, 0., 1.54, 289.6);
static const NeutronAtom Pu242(94, 242, 8.1, 0., 8.2, 0., 8.2, 18.5);
static const NeutronAtom Am(95, 8.3, NAN, 8.7, 0.3, 9.0, 75.3); // TODO (+/-)2.(7.)
static const NeutronAtom Cm(96, 0., 0., 0., 0., 0., 0.);
static const NeutronAtom Cm244(96, 244, 9.5, 0., 11.3, 0., 11.3, 16.2);
static const NeutronAtom Cm246(96, 246, 9.3, 0., 10.9, 0., 10.9, 1.36);
static const NeutronAtom Cm248(96, 248, 7.7, 0., 7.5, 0., 7.5, 3);

/** All of the atoms in a single array so it can be searched. */
static const NeutronAtom ATOMS[] = {H, H1, H2, H3, He, He3, He4, Li, Li6, Li7, Be, B, B10,
				    B11, C, C12, C13, N, N14, N15, O, O16, O17, O18, F, Ne,
				    Ne20, Ne21, Ne22, Na, Mg, Mg24, Mg25, Mg26, Al, Si, 
				    Si28, Si29, Si30, P, S, S32, S33, S34, S36, Cl, Cl35, 
				    Cl37, Ar, Ar36, Ar38, Ar40, K, K39, K40, K41, Ca, Ca40, 
				    Ca42, Ca43, Ca44, Ca46, Ca48, Sc, Ti, Ti46, Ti47, Ti48, 
				    Ti49, Ti50, V, V50, V51, Cr, Cr50, Cr52, Cr53, Cr54, Mn,
				    Fe, Fe54, Fe56, Fe57, Fe58, Co, Ni, Ni58, Ni60, Ni61,
				    Ni62, Ni64, Cu, Cu63, Cu65, Zn, Zn64, Zn66, Zn67, Zn68,
				    Zn70, Ga, Ga69, Ga71, Ge, Ge70, Ge72, Ge73, Ge74, Ge76,
				    As, Se, Se74, Se76, Se77, Se78, Se80, Se82, Br, Br79, Br81,
				    Kr, Kr78, Kr80, Kr82, Kr83, Kr84, Kr86, Rb, Rb85, Rb87, 
				    Sr, Sr84, Sr86, Sr87, Sr88, Y, Zr, Zr90, Zr91, Zr92, Zr94,
				    Zr96, Nb, Mo, Mo92, Mo94, Mo95, Mo96, Mo97, Mo98, Mo100, Tc,
				    Ru, Ru96, Ru98, Ru99, Ru100, Ru101, Ru102, Ru104, Rh, Pd,
				    Pd102, Pd104, Pd105, Pd106, Pd108, Pd110, Ag, Ag107, Ag109,
				    Cd, Cd106, Cd108, Cd110, Cd111, Cd112, Cd113, Cd114, Cd116,
				    In, In113, In115, Sn, Sn112, Sn114, Sn115, Sn116, Sn117,
				    Sn118, Sn119, Sn120, Sn122, Sn124, Sb, Sb121, Sb123, Te,
				    Te120, Te122, Te123, Te124, Te125, Te126, Te128, Te130, I,
				    Xe, Xe124, Xe126, Xe128, Xe129, Xe130, Xe131, Xe132, Xe134,
				    Xe136, Cs, Ba, Ba130, Ba132, Ba134, Ba135, Ba136, Ba137,
				    Ba138, La, La138, La139, Ce, Ce136, Ce138, Ce140, Ce142, Pr,
				    Nd, Nd142, Nd143, Nd144, Nd145, Nd146, Nd148, Nd150, Pm, Sm,
				    Sm144, Sm147, Sm148, Sm149, Sm150, Sm152, Sm154, Eu, Eu151,
				    Eu153, Gd, Gd152, Gd154, Gd155, Gd156, Gd157, Gd158, Gd160,
				    Tb, Dy, Dy156, Dy158, Dy160, Dy161, Dy162, Dy163, Dy164, Ho,
				    Er, Er162, Er164, Er166, Er167, Er168, Er170, Tm, Yb, Yb168,
				    Yb170, Yb171, Yb172, Yb173, Yb174, Yb176, Lu, Lu175, Lu176,
				    Hf, Hf174, Hf176, Hf177, Hf178, Hf179, Hf180, Ta, Ta180,
				    Ta181, W, W180, W182, W183, W184, W186, Re, Re185, Re187, Os,
				    Os184, Os186, Os187, Os188, Os189, Os190, Os192, Ir, Ir191,
				    Ir193, Pt, Pt190, Pt192, Pt194, Pt195, Pt196, Pt198, Au, Hg,
				    Hg196, Hg198, Hg199, Hg200, Hg201, Hg202, Hg204, Tl, Tl203,
				    Tl205, Pb, Pb204, Pb206, Pb207, Pb208, Bi, Po, At, Rn, Fr, Ra,
				    Ac, Th, Pa, U, U233, U234, U235, U238, Np, Pu, Pu238, Pu239,
				    Pu240, Pu242, Am, Cm, Cm244, Cm246, Cm248};

/** The total number of atoms in the array. */
static const size_t NUM_ATOMS = 371;

} // namespace Kernel
} // namespace Mantid
