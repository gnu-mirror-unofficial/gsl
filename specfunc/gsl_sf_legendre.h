/* Author:  G. Jungman
 * RCS:     $Id$
 */
#ifndef GSL_SF_LEGENDRE_H_
#define GSL_SF_LEGENDRE_H_


/* P_l(x)   l >= 0; |x| <= 1
 *
 * exceptions: GSL_EDOM
 */
int     gsl_sf_legendre_Pl_impl(int l, double x, double * result, double * harvest);
int     gsl_sf_legendre_Pl_e(int l, double x, double * result);
double  gsl_sf_legendre_Pl(int l, double x);


/* P_l(x) for l=0,...,lmax
 *
 * exceptions: GSL_EDOM
 */
int gsl_sf_legendre_Pl_array_impl(int lmax, double x, double * result_array);
int gsl_sf_legendre_Pl_array_e(int lmax, double x, double * result_array);


/* P_l(x), l=1,2,3,4,5
 *
 * exceptions: none
 */
double gsl_sf_legendre_P1(double x);
double gsl_sf_legendre_P2(double x);
double gsl_sf_legendre_P3(double x);
double gsl_sf_legendre_P4(double x);
double gsl_sf_legendre_P5(double x);


/* P_l^m(x)  m >= 0; l >= m; x >= 0
 *
 * Note that this function grows combinatorially with l.
 * Therefore we can easily generate an overflow for l larger
 * than about 150.
 *
 * There is no trouble for small m, but when m and l are both large,
 * then there will be trouble. Rather than allow overflows, these
 * functions refuse to calculate when they can sense that l and m are
 * too big.
 *
 * If you really want to calculate a spherical harmonic, then DO NOT
 * use this. Instead use legendre_sphPlm() below, which  uses a similar
 * recursion, but with the normalized functions.
 *
 * exceptions: GSL_EDOM, GSL_EOVRFLW
 */
int     gsl_sf_legendre_Plm_impl(int l, int m, double one_m_x, double one_p_x, double * result, double * harvest);
int     gsl_sf_legendre_Plm_e(int l, int m, double x, double * result);
double  gsl_sf_legendre_Plm(int l, int m, double x);


/* P_l^m(x)  m >= 0; l >= m; x >= 0
 * l=|m|,...,lmax
 *
 * exceptions: 
 */
int gsl_sf_legendre_Plm_array_e(int lmax, int m, double x, double * result_array);


/* P_l^m(x), normalized properly for use in spherical harmonics
 * m >= 0; l >= m; x >= 0
 *
 * There is no overflow problem, as there is for the
 * standard normalization of P_l^m(x).
 *
 * Specifically, it returns:
 *
 *        sqrt((2l+1)/(4pi)) sqrt((l-m)!/(l+m)!) P_l^m(x)
 *
 * exceptions: GSL_EDOM
 */
int     gsl_sf_legendre_sphPlm_impl(int l, int m, double one_m_x, double one_p_x, double * result, double * harvest);
int     gsl_sf_legendre_sphPlm_e(int l, int m, double x, double * result);
double  gsl_sf_legendre_sphPlm(int l, int m, double x);


/* P_l^m(x), normalized properly for use in spherical harmonics
 * m >= 0; l >= m; x >= 0
 * l=|m|,...,lmax
 *
 * exceptions: 
 */
int gsl_sf_legendre_sphPlm_array_e(int lmax, int m, double x, double * result_array);


/* size of result_array[] needed for the array versions
 * (lmax - m + 1)
 */
int gsl_sf_legendre_array_size(const int lmax, const int m);


/* Irregular (Spherical) Conical Function, mu=1/2
 * P^{1/2}_{-1/2 + I lambda}(x)
 *
 * exceptions: 
 */
int     gsl_sf_conical_half_impl(double lambda, double x, double * result);
int     gsl_sf_conical_half_e(double lambda, double x, double * result);
double  gsl_sf_conical_half(double lambda, double x);


/* Regular (Spherical) Conical Function, mu=-1/2
 * P^{-1/2}_{-1/2 + I lambda}(x)
 *
 * exceptions: 
 */
int     gsl_sf_conical_mhalf_impl(double lambda, double x, double * result);
int     gsl_sf_conical_mhalf_e(double lambda, double x, double * result);
double  gsl_sf_conical_mhalf(double lambda, double x);


/* Conical Function, mu=0
 * P^{0}_{-1/2 + I lambda}(x)
 *
 * exceptions: 
 */
int     gsl_sf_conical_0_impl(double lambda, double x, double * result);
int     gsl_sf_conical_0_e(double lambda, double x, double * result);
double  gsl_sf_conical_0(double lambda, double x);


/* Conical Function, mu=1
 * P^{1}_{-1/2 + I lambda}(x)
 *
 * exceptions: 
 */
int     gsl_sf_conical_1_impl(double lambda, double x, double * result);
int     gsl_sf_conical_1_e(double lambda, double x, double * result);
double  gsl_sf_conical_1(double lambda, double x);


/* Regular (Spherical) Conical Function
 * P^{-1/2-l}_{-1/2 + I lambda}(x)
 *
 * exceptions: 
 */
int     gsl_sf_conical_sph_reg_impl(int l, double lambda, double x, double * result);
int     gsl_sf_conical_sph_reg_e(int l, double lambda, double x, double * result);
double  gsl_sf_conical_sph_reg(int l, double lambda, double x);


/* Regular (Spherical) Conical Function
 * P^{-1/2-l}_{-1/2 + I lambda}(x)
 * l=0,...,lmax
 *
 * exceptions: 
 */
int gsl_sf_conical_sph_reg_array_impl(int lmax, double lambda, double x, double * result_array);
int gsl_sf_conical_sph_reg_array_e(int lmax, double lambda, double x, double * result_array);



/*
int gsl_sf_conical_xlt1_large_mu_impl(double mu, double tau, double x, double * result);
*/


#ifdef HAVE_INLINE
extern inline double gsl_sf_legendre_P1(double x) { return x; }
extern inline double gsl_sf_legendre_P2(double x) { return 0.5*(3.0*x*x - 1.0); }
extern inline double gsl_sf_legendre_P3(double x) { return 0.5*x*(5.0*x*x - 3.0); }
extern inline double gsl_sf_legendre_P4(double x)
{ double x2 = x*x; return (35.0*x2*x2 - 30.0*x2 + 3.0)/8.0; }
extern inline double gsl_sf_legendre_P5(double x)
{ double x2 = x*x; return x*(63.0*x2*x2 - 70.0*x2 + 15.0)/8.0; }
#endif /* HAVE_INLINE */

#ifdef HAVE_INLINE
extern inline int gsl_sf_legendre_array_size(const int lmax, const int m) { return lmax-m+1; }
#endif /* HAVE_INLINE */


#endif /* !GSL_SF_LEGENDRE_H_ */
