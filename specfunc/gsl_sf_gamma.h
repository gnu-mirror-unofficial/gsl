/* Author:  G. Jungman
 * RCS:     $Id$
 */
#ifndef GSL_GAMMAFUNCTION_H_
#define GSL_GAMMAFUNCTION_H_


/* Log[Gamma(x)]; Lanczos method */
int gsl_sf_lngamma_e(double x, double * result);  /* GSL_EDOM */

double gsl_sf_lngamma(double);    /* domain */


/* Gamma(z) for z complex.
   Calculates:
      lnr = log|Gamma(z)|
      arg = arg(Gamma(z))  in (-Pi, Pi]
 */
int gsl_sf_lngamma_complex_e(double zr, double zi, double * lnr, double * arg); /* GSL_EDOM */


/* n! */
int gsl_sf_fact_e(int n, double * result);   /* GSL_EDOM, GSL_OVRFLW */


/* n!! = n(n-2)(n-4) ...  */
int gsl_sf_doublefact_e(int n, double * result);


/* log(n!) 
 * Faster then ln(Gamma(n+1)) for n < 170; defers for larger n.
 */
int gsl_sf_lnfact_e(int n, double * result);  /* GSL_EDOM */

double gsl_sf_lnfact(int n);  /* domain */


/* n choose m */
int gsl_sf_lnchoose_e(unsigned int n, unsigned int m, double * result); /* GSL_EDOM    */
int gsl_sf_choose_e(unsigned int n, unsigned int m, double * result);   /* GSL_EDOM, GSL_EOVRFLW */

double gsl_sf_lnchoose(unsigned int n, unsigned int m);  /* domain */
double gsl_sf_choose(unsigned int n, unsigned int m);    /* domain, overflow */


int gsl_sf_lngamma_impl(double x, double * result);
int gsl_sf_lngamma_complex_impl(double zr, double zi, double * lnr, double * arg);

int gsl_sf_fact_impl(int n, double * result);
int gsl_sf_lnfact_impl(int n, double * result);

int gsl_sf_doublefact_impl(int n, double * result);

int gsl_sf_choose_impl(unsigned int n, unsigned int m, double * result);
int gsl_sf_lnchoose_impl(unsigned int n, unsigned int m, double * result);


#endif /* !GSL_GAMMAFUNCTION_H_ */
