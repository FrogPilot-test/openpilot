#include "car.h"

namespace {
#define DIM 9
#define EDIM 9
#define MEDIM 9
typedef void (*Hfun)(double *, double *, double *);

double mass;

void set_mass(double x){ mass = x;}

double rotational_inertia;

void set_rotational_inertia(double x){ rotational_inertia = x;}

double center_to_front;

void set_center_to_front(double x){ center_to_front = x;}

double center_to_rear;

void set_center_to_rear(double x){ center_to_rear = x;}

double stiffness_front;

void set_stiffness_front(double x){ stiffness_front = x;}

double stiffness_rear;

void set_stiffness_rear(double x){ stiffness_rear = x;}
const static double MAHA_THRESH_25 = 3.8414588206941227;
const static double MAHA_THRESH_24 = 5.991464547107981;
const static double MAHA_THRESH_30 = 3.8414588206941227;
const static double MAHA_THRESH_26 = 3.8414588206941227;
const static double MAHA_THRESH_27 = 3.8414588206941227;
const static double MAHA_THRESH_29 = 3.8414588206941227;
const static double MAHA_THRESH_28 = 3.8414588206941227;
const static double MAHA_THRESH_31 = 3.8414588206941227;

/******************************************************************************
 *                       Code generated with SymPy 1.12                       *
 *                                                                            *
 *              See http://www.sympy.org/ for more information.               *
 *                                                                            *
 *                         This file is part of 'ekf'                         *
 ******************************************************************************/
void err_fun(double *nom_x, double *delta_x, double *out_5730635526955243028) {
   out_5730635526955243028[0] = delta_x[0] + nom_x[0];
   out_5730635526955243028[1] = delta_x[1] + nom_x[1];
   out_5730635526955243028[2] = delta_x[2] + nom_x[2];
   out_5730635526955243028[3] = delta_x[3] + nom_x[3];
   out_5730635526955243028[4] = delta_x[4] + nom_x[4];
   out_5730635526955243028[5] = delta_x[5] + nom_x[5];
   out_5730635526955243028[6] = delta_x[6] + nom_x[6];
   out_5730635526955243028[7] = delta_x[7] + nom_x[7];
   out_5730635526955243028[8] = delta_x[8] + nom_x[8];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_2126725142301084152) {
   out_2126725142301084152[0] = -nom_x[0] + true_x[0];
   out_2126725142301084152[1] = -nom_x[1] + true_x[1];
   out_2126725142301084152[2] = -nom_x[2] + true_x[2];
   out_2126725142301084152[3] = -nom_x[3] + true_x[3];
   out_2126725142301084152[4] = -nom_x[4] + true_x[4];
   out_2126725142301084152[5] = -nom_x[5] + true_x[5];
   out_2126725142301084152[6] = -nom_x[6] + true_x[6];
   out_2126725142301084152[7] = -nom_x[7] + true_x[7];
   out_2126725142301084152[8] = -nom_x[8] + true_x[8];
}
void H_mod_fun(double *state, double *out_298022881705835774) {
   out_298022881705835774[0] = 1.0;
   out_298022881705835774[1] = 0;
   out_298022881705835774[2] = 0;
   out_298022881705835774[3] = 0;
   out_298022881705835774[4] = 0;
   out_298022881705835774[5] = 0;
   out_298022881705835774[6] = 0;
   out_298022881705835774[7] = 0;
   out_298022881705835774[8] = 0;
   out_298022881705835774[9] = 0;
   out_298022881705835774[10] = 1.0;
   out_298022881705835774[11] = 0;
   out_298022881705835774[12] = 0;
   out_298022881705835774[13] = 0;
   out_298022881705835774[14] = 0;
   out_298022881705835774[15] = 0;
   out_298022881705835774[16] = 0;
   out_298022881705835774[17] = 0;
   out_298022881705835774[18] = 0;
   out_298022881705835774[19] = 0;
   out_298022881705835774[20] = 1.0;
   out_298022881705835774[21] = 0;
   out_298022881705835774[22] = 0;
   out_298022881705835774[23] = 0;
   out_298022881705835774[24] = 0;
   out_298022881705835774[25] = 0;
   out_298022881705835774[26] = 0;
   out_298022881705835774[27] = 0;
   out_298022881705835774[28] = 0;
   out_298022881705835774[29] = 0;
   out_298022881705835774[30] = 1.0;
   out_298022881705835774[31] = 0;
   out_298022881705835774[32] = 0;
   out_298022881705835774[33] = 0;
   out_298022881705835774[34] = 0;
   out_298022881705835774[35] = 0;
   out_298022881705835774[36] = 0;
   out_298022881705835774[37] = 0;
   out_298022881705835774[38] = 0;
   out_298022881705835774[39] = 0;
   out_298022881705835774[40] = 1.0;
   out_298022881705835774[41] = 0;
   out_298022881705835774[42] = 0;
   out_298022881705835774[43] = 0;
   out_298022881705835774[44] = 0;
   out_298022881705835774[45] = 0;
   out_298022881705835774[46] = 0;
   out_298022881705835774[47] = 0;
   out_298022881705835774[48] = 0;
   out_298022881705835774[49] = 0;
   out_298022881705835774[50] = 1.0;
   out_298022881705835774[51] = 0;
   out_298022881705835774[52] = 0;
   out_298022881705835774[53] = 0;
   out_298022881705835774[54] = 0;
   out_298022881705835774[55] = 0;
   out_298022881705835774[56] = 0;
   out_298022881705835774[57] = 0;
   out_298022881705835774[58] = 0;
   out_298022881705835774[59] = 0;
   out_298022881705835774[60] = 1.0;
   out_298022881705835774[61] = 0;
   out_298022881705835774[62] = 0;
   out_298022881705835774[63] = 0;
   out_298022881705835774[64] = 0;
   out_298022881705835774[65] = 0;
   out_298022881705835774[66] = 0;
   out_298022881705835774[67] = 0;
   out_298022881705835774[68] = 0;
   out_298022881705835774[69] = 0;
   out_298022881705835774[70] = 1.0;
   out_298022881705835774[71] = 0;
   out_298022881705835774[72] = 0;
   out_298022881705835774[73] = 0;
   out_298022881705835774[74] = 0;
   out_298022881705835774[75] = 0;
   out_298022881705835774[76] = 0;
   out_298022881705835774[77] = 0;
   out_298022881705835774[78] = 0;
   out_298022881705835774[79] = 0;
   out_298022881705835774[80] = 1.0;
}
void f_fun(double *state, double dt, double *out_2001108754065845754) {
   out_2001108754065845754[0] = state[0];
   out_2001108754065845754[1] = state[1];
   out_2001108754065845754[2] = state[2];
   out_2001108754065845754[3] = state[3];
   out_2001108754065845754[4] = state[4];
   out_2001108754065845754[5] = dt*((-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]))*state[6] - 9.8000000000000007*state[8] + stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*state[1]) + (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*state[4])) + state[5];
   out_2001108754065845754[6] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*state[4])) + state[6];
   out_2001108754065845754[7] = state[7];
   out_2001108754065845754[8] = state[8];
}
void F_fun(double *state, double dt, double *out_5743885166475278634) {
   out_5743885166475278634[0] = 1;
   out_5743885166475278634[1] = 0;
   out_5743885166475278634[2] = 0;
   out_5743885166475278634[3] = 0;
   out_5743885166475278634[4] = 0;
   out_5743885166475278634[5] = 0;
   out_5743885166475278634[6] = 0;
   out_5743885166475278634[7] = 0;
   out_5743885166475278634[8] = 0;
   out_5743885166475278634[9] = 0;
   out_5743885166475278634[10] = 1;
   out_5743885166475278634[11] = 0;
   out_5743885166475278634[12] = 0;
   out_5743885166475278634[13] = 0;
   out_5743885166475278634[14] = 0;
   out_5743885166475278634[15] = 0;
   out_5743885166475278634[16] = 0;
   out_5743885166475278634[17] = 0;
   out_5743885166475278634[18] = 0;
   out_5743885166475278634[19] = 0;
   out_5743885166475278634[20] = 1;
   out_5743885166475278634[21] = 0;
   out_5743885166475278634[22] = 0;
   out_5743885166475278634[23] = 0;
   out_5743885166475278634[24] = 0;
   out_5743885166475278634[25] = 0;
   out_5743885166475278634[26] = 0;
   out_5743885166475278634[27] = 0;
   out_5743885166475278634[28] = 0;
   out_5743885166475278634[29] = 0;
   out_5743885166475278634[30] = 1;
   out_5743885166475278634[31] = 0;
   out_5743885166475278634[32] = 0;
   out_5743885166475278634[33] = 0;
   out_5743885166475278634[34] = 0;
   out_5743885166475278634[35] = 0;
   out_5743885166475278634[36] = 0;
   out_5743885166475278634[37] = 0;
   out_5743885166475278634[38] = 0;
   out_5743885166475278634[39] = 0;
   out_5743885166475278634[40] = 1;
   out_5743885166475278634[41] = 0;
   out_5743885166475278634[42] = 0;
   out_5743885166475278634[43] = 0;
   out_5743885166475278634[44] = 0;
   out_5743885166475278634[45] = dt*(stiffness_front*(-state[2] - state[3] + state[7])/(mass*state[1]) + (-stiffness_front - stiffness_rear)*state[5]/(mass*state[4]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[6]/(mass*state[4]));
   out_5743885166475278634[46] = -dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*pow(state[1], 2));
   out_5743885166475278634[47] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_5743885166475278634[48] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_5743885166475278634[49] = dt*((-1 - (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*pow(state[4], 2)))*state[6] - (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*pow(state[4], 2)));
   out_5743885166475278634[50] = dt*(-stiffness_front*state[0] - stiffness_rear*state[0])/(mass*state[4]) + 1;
   out_5743885166475278634[51] = dt*(-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]));
   out_5743885166475278634[52] = dt*stiffness_front*state[0]/(mass*state[1]);
   out_5743885166475278634[53] = -9.8000000000000007*dt;
   out_5743885166475278634[54] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front - pow(center_to_rear, 2)*stiffness_rear)*state[6]/(rotational_inertia*state[4]));
   out_5743885166475278634[55] = -center_to_front*dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*pow(state[1], 2));
   out_5743885166475278634[56] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_5743885166475278634[57] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_5743885166475278634[58] = dt*(-(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*pow(state[4], 2)) - (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*pow(state[4], 2)));
   out_5743885166475278634[59] = dt*(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(rotational_inertia*state[4]);
   out_5743885166475278634[60] = dt*(-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])/(rotational_inertia*state[4]) + 1;
   out_5743885166475278634[61] = center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_5743885166475278634[62] = 0;
   out_5743885166475278634[63] = 0;
   out_5743885166475278634[64] = 0;
   out_5743885166475278634[65] = 0;
   out_5743885166475278634[66] = 0;
   out_5743885166475278634[67] = 0;
   out_5743885166475278634[68] = 0;
   out_5743885166475278634[69] = 0;
   out_5743885166475278634[70] = 1;
   out_5743885166475278634[71] = 0;
   out_5743885166475278634[72] = 0;
   out_5743885166475278634[73] = 0;
   out_5743885166475278634[74] = 0;
   out_5743885166475278634[75] = 0;
   out_5743885166475278634[76] = 0;
   out_5743885166475278634[77] = 0;
   out_5743885166475278634[78] = 0;
   out_5743885166475278634[79] = 0;
   out_5743885166475278634[80] = 1;
}
void h_25(double *state, double *unused, double *out_3455291011871388754) {
   out_3455291011871388754[0] = state[6];
}
void H_25(double *state, double *unused, double *out_761261685286660542) {
   out_761261685286660542[0] = 0;
   out_761261685286660542[1] = 0;
   out_761261685286660542[2] = 0;
   out_761261685286660542[3] = 0;
   out_761261685286660542[4] = 0;
   out_761261685286660542[5] = 0;
   out_761261685286660542[6] = 1;
   out_761261685286660542[7] = 0;
   out_761261685286660542[8] = 0;
}
void h_24(double *state, double *unused, double *out_3647373860870508432) {
   out_3647373860870508432[0] = state[4];
   out_3647373860870508432[1] = state[5];
}
void H_24(double *state, double *unused, double *out_2796663913515662594) {
   out_2796663913515662594[0] = 0;
   out_2796663913515662594[1] = 0;
   out_2796663913515662594[2] = 0;
   out_2796663913515662594[3] = 0;
   out_2796663913515662594[4] = 1;
   out_2796663913515662594[5] = 0;
   out_2796663913515662594[6] = 0;
   out_2796663913515662594[7] = 0;
   out_2796663913515662594[8] = 0;
   out_2796663913515662594[9] = 0;
   out_2796663913515662594[10] = 0;
   out_2796663913515662594[11] = 0;
   out_2796663913515662594[12] = 0;
   out_2796663913515662594[13] = 0;
   out_2796663913515662594[14] = 1;
   out_2796663913515662594[15] = 0;
   out_2796663913515662594[16] = 0;
   out_2796663913515662594[17] = 0;
}
void h_30(double *state, double *unused, double *out_8627859083536347001) {
   out_8627859083536347001[0] = state[4];
}
void H_30(double *state, double *unused, double *out_6155428656204956213) {
   out_6155428656204956213[0] = 0;
   out_6155428656204956213[1] = 0;
   out_6155428656204956213[2] = 0;
   out_6155428656204956213[3] = 0;
   out_6155428656204956213[4] = 1;
   out_6155428656204956213[5] = 0;
   out_6155428656204956213[6] = 0;
   out_6155428656204956213[7] = 0;
   out_6155428656204956213[8] = 0;
}
void h_26(double *state, double *unused, double *out_9215569105496727098) {
   out_9215569105496727098[0] = state[7];
}
void H_26(double *state, double *unused, double *out_4502765004160716766) {
   out_4502765004160716766[0] = 0;
   out_4502765004160716766[1] = 0;
   out_4502765004160716766[2] = 0;
   out_4502765004160716766[3] = 0;
   out_4502765004160716766[4] = 0;
   out_4502765004160716766[5] = 0;
   out_4502765004160716766[6] = 0;
   out_4502765004160716766[7] = 1;
   out_4502765004160716766[8] = 0;
}
void h_27(double *state, double *unused, double *out_6195143597572165722) {
   out_6195143597572165722[0] = state[3];
}
void H_27(double *state, double *unused, double *out_3980665344404531302) {
   out_3980665344404531302[0] = 0;
   out_3980665344404531302[1] = 0;
   out_3980665344404531302[2] = 0;
   out_3980665344404531302[3] = 1;
   out_3980665344404531302[4] = 0;
   out_3980665344404531302[5] = 0;
   out_3980665344404531302[6] = 0;
   out_3980665344404531302[7] = 0;
   out_3980665344404531302[8] = 0;
}
void h_29(double *state, double *unused, double *out_1686129053181883463) {
   out_1686129053181883463[0] = state[1];
}
void H_29(double *state, double *unused, double *out_6665660000519348397) {
   out_6665660000519348397[0] = 0;
   out_6665660000519348397[1] = 1;
   out_6665660000519348397[2] = 0;
   out_6665660000519348397[3] = 0;
   out_6665660000519348397[4] = 0;
   out_6665660000519348397[5] = 0;
   out_6665660000519348397[6] = 0;
   out_6665660000519348397[7] = 0;
   out_6665660000519348397[8] = 0;
}
void h_28(double *state, double *unused, double *out_5591827712895688205) {
   out_5591827712895688205[0] = state[0];
}
void H_28(double *state, double *unused, double *out_2815096399534550305) {
   out_2815096399534550305[0] = 1;
   out_2815096399534550305[1] = 0;
   out_2815096399534550305[2] = 0;
   out_2815096399534550305[3] = 0;
   out_2815096399534550305[4] = 0;
   out_2815096399534550305[5] = 0;
   out_2815096399534550305[6] = 0;
   out_2815096399534550305[7] = 0;
   out_2815096399534550305[8] = 0;
}
void h_31(double *state, double *unused, double *out_3612477680222517899) {
   out_3612477680222517899[0] = state[8];
}
void H_31(double *state, double *unused, double *out_730615723409700114) {
   out_730615723409700114[0] = 0;
   out_730615723409700114[1] = 0;
   out_730615723409700114[2] = 0;
   out_730615723409700114[3] = 0;
   out_730615723409700114[4] = 0;
   out_730615723409700114[5] = 0;
   out_730615723409700114[6] = 0;
   out_730615723409700114[7] = 0;
   out_730615723409700114[8] = 1;
}
#include <eigen3/Eigen/Dense>
#include <iostream>

typedef Eigen::Matrix<double, DIM, DIM, Eigen::RowMajor> DDM;
typedef Eigen::Matrix<double, EDIM, EDIM, Eigen::RowMajor> EEM;
typedef Eigen::Matrix<double, DIM, EDIM, Eigen::RowMajor> DEM;

void predict(double *in_x, double *in_P, double *in_Q, double dt) {
  typedef Eigen::Matrix<double, MEDIM, MEDIM, Eigen::RowMajor> RRM;

  double nx[DIM] = {0};
  double in_F[EDIM*EDIM] = {0};

  // functions from sympy
  f_fun(in_x, dt, nx);
  F_fun(in_x, dt, in_F);


  EEM F(in_F);
  EEM P(in_P);
  EEM Q(in_Q);

  RRM F_main = F.topLeftCorner(MEDIM, MEDIM);
  P.topLeftCorner(MEDIM, MEDIM) = (F_main * P.topLeftCorner(MEDIM, MEDIM)) * F_main.transpose();
  P.topRightCorner(MEDIM, EDIM - MEDIM) = F_main * P.topRightCorner(MEDIM, EDIM - MEDIM);
  P.bottomLeftCorner(EDIM - MEDIM, MEDIM) = P.bottomLeftCorner(EDIM - MEDIM, MEDIM) * F_main.transpose();

  P = P + dt*Q;

  // copy out state
  memcpy(in_x, nx, DIM * sizeof(double));
  memcpy(in_P, P.data(), EDIM * EDIM * sizeof(double));
}

// note: extra_args dim only correct when null space projecting
// otherwise 1
template <int ZDIM, int EADIM, bool MAHA_TEST>
void update(double *in_x, double *in_P, Hfun h_fun, Hfun H_fun, Hfun Hea_fun, double *in_z, double *in_R, double *in_ea, double MAHA_THRESHOLD) {
  typedef Eigen::Matrix<double, ZDIM, ZDIM, Eigen::RowMajor> ZZM;
  typedef Eigen::Matrix<double, ZDIM, DIM, Eigen::RowMajor> ZDM;
  typedef Eigen::Matrix<double, Eigen::Dynamic, EDIM, Eigen::RowMajor> XEM;
  //typedef Eigen::Matrix<double, EDIM, ZDIM, Eigen::RowMajor> EZM;
  typedef Eigen::Matrix<double, Eigen::Dynamic, 1> X1M;
  typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> XXM;

  double in_hx[ZDIM] = {0};
  double in_H[ZDIM * DIM] = {0};
  double in_H_mod[EDIM * DIM] = {0};
  double delta_x[EDIM] = {0};
  double x_new[DIM] = {0};


  // state x, P
  Eigen::Matrix<double, ZDIM, 1> z(in_z);
  EEM P(in_P);
  ZZM pre_R(in_R);

  // functions from sympy
  h_fun(in_x, in_ea, in_hx);
  H_fun(in_x, in_ea, in_H);
  ZDM pre_H(in_H);

  // get y (y = z - hx)
  Eigen::Matrix<double, ZDIM, 1> pre_y(in_hx); pre_y = z - pre_y;
  X1M y; XXM H; XXM R;
  if (Hea_fun){
    typedef Eigen::Matrix<double, ZDIM, EADIM, Eigen::RowMajor> ZAM;
    double in_Hea[ZDIM * EADIM] = {0};
    Hea_fun(in_x, in_ea, in_Hea);
    ZAM Hea(in_Hea);
    XXM A = Hea.transpose().fullPivLu().kernel();


    y = A.transpose() * pre_y;
    H = A.transpose() * pre_H;
    R = A.transpose() * pre_R * A;
  } else {
    y = pre_y;
    H = pre_H;
    R = pre_R;
  }
  // get modified H
  H_mod_fun(in_x, in_H_mod);
  DEM H_mod(in_H_mod);
  XEM H_err = H * H_mod;

  // Do mahalobis distance test
  if (MAHA_TEST){
    XXM a = (H_err * P * H_err.transpose() + R).inverse();
    double maha_dist = y.transpose() * a * y;
    if (maha_dist > MAHA_THRESHOLD){
      R = 1.0e16 * R;
    }
  }

  // Outlier resilient weighting
  double weight = 1;//(1.5)/(1 + y.squaredNorm()/R.sum());

  // kalman gains and I_KH
  XXM S = ((H_err * P) * H_err.transpose()) + R/weight;
  XEM KT = S.fullPivLu().solve(H_err * P.transpose());
  //EZM K = KT.transpose(); TODO: WHY DOES THIS NOT COMPILE?
  //EZM K = S.fullPivLu().solve(H_err * P.transpose()).transpose();
  //std::cout << "Here is the matrix rot:\n" << K << std::endl;
  EEM I_KH = Eigen::Matrix<double, EDIM, EDIM>::Identity() - (KT.transpose() * H_err);

  // update state by injecting dx
  Eigen::Matrix<double, EDIM, 1> dx(delta_x);
  dx  = (KT.transpose() * y);
  memcpy(delta_x, dx.data(), EDIM * sizeof(double));
  err_fun(in_x, delta_x, x_new);
  Eigen::Matrix<double, DIM, 1> x(x_new);

  // update cov
  P = ((I_KH * P) * I_KH.transpose()) + ((KT.transpose() * R) * KT);

  // copy out state
  memcpy(in_x, x.data(), DIM * sizeof(double));
  memcpy(in_P, P.data(), EDIM * EDIM * sizeof(double));
  memcpy(in_z, y.data(), y.rows() * sizeof(double));
}




}
extern "C" {

void car_update_25(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_25, H_25, NULL, in_z, in_R, in_ea, MAHA_THRESH_25);
}
void car_update_24(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<2, 3, 0>(in_x, in_P, h_24, H_24, NULL, in_z, in_R, in_ea, MAHA_THRESH_24);
}
void car_update_30(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_30, H_30, NULL, in_z, in_R, in_ea, MAHA_THRESH_30);
}
void car_update_26(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_26, H_26, NULL, in_z, in_R, in_ea, MAHA_THRESH_26);
}
void car_update_27(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_27, H_27, NULL, in_z, in_R, in_ea, MAHA_THRESH_27);
}
void car_update_29(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_29, H_29, NULL, in_z, in_R, in_ea, MAHA_THRESH_29);
}
void car_update_28(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_28, H_28, NULL, in_z, in_R, in_ea, MAHA_THRESH_28);
}
void car_update_31(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_31, H_31, NULL, in_z, in_R, in_ea, MAHA_THRESH_31);
}
void car_err_fun(double *nom_x, double *delta_x, double *out_5730635526955243028) {
  err_fun(nom_x, delta_x, out_5730635526955243028);
}
void car_inv_err_fun(double *nom_x, double *true_x, double *out_2126725142301084152) {
  inv_err_fun(nom_x, true_x, out_2126725142301084152);
}
void car_H_mod_fun(double *state, double *out_298022881705835774) {
  H_mod_fun(state, out_298022881705835774);
}
void car_f_fun(double *state, double dt, double *out_2001108754065845754) {
  f_fun(state,  dt, out_2001108754065845754);
}
void car_F_fun(double *state, double dt, double *out_5743885166475278634) {
  F_fun(state,  dt, out_5743885166475278634);
}
void car_h_25(double *state, double *unused, double *out_3455291011871388754) {
  h_25(state, unused, out_3455291011871388754);
}
void car_H_25(double *state, double *unused, double *out_761261685286660542) {
  H_25(state, unused, out_761261685286660542);
}
void car_h_24(double *state, double *unused, double *out_3647373860870508432) {
  h_24(state, unused, out_3647373860870508432);
}
void car_H_24(double *state, double *unused, double *out_2796663913515662594) {
  H_24(state, unused, out_2796663913515662594);
}
void car_h_30(double *state, double *unused, double *out_8627859083536347001) {
  h_30(state, unused, out_8627859083536347001);
}
void car_H_30(double *state, double *unused, double *out_6155428656204956213) {
  H_30(state, unused, out_6155428656204956213);
}
void car_h_26(double *state, double *unused, double *out_9215569105496727098) {
  h_26(state, unused, out_9215569105496727098);
}
void car_H_26(double *state, double *unused, double *out_4502765004160716766) {
  H_26(state, unused, out_4502765004160716766);
}
void car_h_27(double *state, double *unused, double *out_6195143597572165722) {
  h_27(state, unused, out_6195143597572165722);
}
void car_H_27(double *state, double *unused, double *out_3980665344404531302) {
  H_27(state, unused, out_3980665344404531302);
}
void car_h_29(double *state, double *unused, double *out_1686129053181883463) {
  h_29(state, unused, out_1686129053181883463);
}
void car_H_29(double *state, double *unused, double *out_6665660000519348397) {
  H_29(state, unused, out_6665660000519348397);
}
void car_h_28(double *state, double *unused, double *out_5591827712895688205) {
  h_28(state, unused, out_5591827712895688205);
}
void car_H_28(double *state, double *unused, double *out_2815096399534550305) {
  H_28(state, unused, out_2815096399534550305);
}
void car_h_31(double *state, double *unused, double *out_3612477680222517899) {
  h_31(state, unused, out_3612477680222517899);
}
void car_H_31(double *state, double *unused, double *out_730615723409700114) {
  H_31(state, unused, out_730615723409700114);
}
void car_predict(double *in_x, double *in_P, double *in_Q, double dt) {
  predict(in_x, in_P, in_Q, dt);
}
void car_set_mass(double x) {
  set_mass(x);
}
void car_set_rotational_inertia(double x) {
  set_rotational_inertia(x);
}
void car_set_center_to_front(double x) {
  set_center_to_front(x);
}
void car_set_center_to_rear(double x) {
  set_center_to_rear(x);
}
void car_set_stiffness_front(double x) {
  set_stiffness_front(x);
}
void car_set_stiffness_rear(double x) {
  set_stiffness_rear(x);
}
}

const EKF car = {
  .name = "car",
  .kinds = { 25, 24, 30, 26, 27, 29, 28, 31 },
  .feature_kinds = {  },
  .f_fun = car_f_fun,
  .F_fun = car_F_fun,
  .err_fun = car_err_fun,
  .inv_err_fun = car_inv_err_fun,
  .H_mod_fun = car_H_mod_fun,
  .predict = car_predict,
  .hs = {
    { 25, car_h_25 },
    { 24, car_h_24 },
    { 30, car_h_30 },
    { 26, car_h_26 },
    { 27, car_h_27 },
    { 29, car_h_29 },
    { 28, car_h_28 },
    { 31, car_h_31 },
  },
  .Hs = {
    { 25, car_H_25 },
    { 24, car_H_24 },
    { 30, car_H_30 },
    { 26, car_H_26 },
    { 27, car_H_27 },
    { 29, car_H_29 },
    { 28, car_H_28 },
    { 31, car_H_31 },
  },
  .updates = {
    { 25, car_update_25 },
    { 24, car_update_24 },
    { 30, car_update_30 },
    { 26, car_update_26 },
    { 27, car_update_27 },
    { 29, car_update_29 },
    { 28, car_update_28 },
    { 31, car_update_31 },
  },
  .Hes = {
  },
  .sets = {
    { "mass", car_set_mass },
    { "rotational_inertia", car_set_rotational_inertia },
    { "center_to_front", car_set_center_to_front },
    { "center_to_rear", car_set_center_to_rear },
    { "stiffness_front", car_set_stiffness_front },
    { "stiffness_rear", car_set_stiffness_rear },
  },
  .extra_routines = {
  },
};

ekf_lib_init(car)
