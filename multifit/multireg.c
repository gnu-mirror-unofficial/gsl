/* multifit/multireg.c
 * 
 * Copyright (C) 2015 Patrick Alken
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * References:
 *
 * [1] P. C. Hansen & D. P. O'Leary, "The use of the L-curve in
 * the regularization of discrete ill-posed problems",  SIAM J. Sci.
 * Comput. 14 (1993), pp. 1487-1503.
 *
 * [2] P. C. Hansen, "Discrete Inverse Problems: Insight and Algorithms,"
 * SIAM Press, 2010.
 */

#include <config.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>

#include "linear_common.c"

/* XXX:  Form the orthogonal matrix Q from the packed QR matrix */
static int
gsl_linalg_Q_unpack (const gsl_matrix * QR, const gsl_vector * tau, gsl_matrix * Q)
{
  const size_t M = QR->size1;
  const size_t N = QR->size2;

  if (Q->size1 != M || Q->size2 != M)
    {
      GSL_ERROR ("Q matrix must be M x M", GSL_ENOTSQR);
    }
  else if (tau->size != GSL_MIN (M, N))
    {
      GSL_ERROR ("size of tau must be MIN(M,N)", GSL_EBADLEN);
    }
  else
    {
      size_t i;

      /* Initialize Q to the identity */

      gsl_matrix_set_identity (Q);

      for (i = GSL_MIN (M, N); i-- > 0;)
        {
          gsl_vector_const_view c = gsl_matrix_const_column (QR, i);
          gsl_vector_const_view h = gsl_vector_const_subvector (&c.vector,
                                                                i, M - i);
          gsl_matrix_view m = gsl_matrix_submatrix (Q, i, i, M - i, M - i);
          double ti = gsl_vector_get (tau, i);
          gsl_linalg_householder_hm (ti, &h.vector, &m.matrix);
        }

      return GSL_SUCCESS;
    }
}

int
gsl_multifit_linear_solve (const double lambda,
                           const gsl_matrix * X,
                           const gsl_vector * y,
                           gsl_vector * c,
                           double *rnorm,
                           double *snorm,
                           gsl_multifit_linear_workspace * work)
{
  size_t rank;
  int status;

  status = multifit_linear_solve(X, y, GSL_DBL_EPSILON, lambda, &rank, c,
                                 rnorm, snorm, work);

  return status;
} /* gsl_multifit_linear_solve() */

/*
gsl_multifit_linear_wstdform1()
  Using regularization matrix
L = diag(l_1,l_2,...,l_p), transform to Tikhonov standard form:

X~ = sqrt(W) X L^{-1}
y~ = sqrt(W) y
c~ = L c

Inputs: L    - Tikhonov matrix as a vector of diagonal elements p-by-1;
               or NULL for L = I
        X    - least squares matrix n-by-p
        y    - right hand side vector n-by-1
        w    - weight vector n-by-1; or NULL for W = I
        Xs   - least squares matrix in standard form X~ n-by-p
        ys   - right hand side vector in standard form y~ n-by-1
        work - workspace

Return: success/error

Notes:
1) It is allowed for X = Xs and y = ys
*/

int
gsl_multifit_linear_wstdform1 (const gsl_vector * L,
                               const gsl_matrix * X,
                               const gsl_vector * w,
                               const gsl_vector * y,
                               gsl_matrix * Xs,
                               gsl_vector * ys,
                               gsl_multifit_linear_workspace * work)
{
  const size_t n = X->size1;
  const size_t p = X->size2;

  if (n > work->nmax || p > work->pmax)
    {
      GSL_ERROR("observation matrix larger than workspace", GSL_EBADLEN);
    }
  else if (L != NULL && p != L->size)
    {
      GSL_ERROR("L vector does not match X", GSL_EBADLEN);
    }
  else if (n != y->size)
    {
      GSL_ERROR("y vector does not match X", GSL_EBADLEN);
    }
  else if (w != NULL && n != w->size)
    {
      GSL_ERROR("weight vector does not match X", GSL_EBADLEN);
    }
  else if (n != Xs->size1 || p != Xs->size2)
    {
      GSL_ERROR("Xs matrix dimensions do not match X", GSL_EBADLEN);
    }
  else if (n != ys->size)
    {
      GSL_ERROR("ys vector must be length n", GSL_EBADLEN);
    }
  else
    {
      int status = GSL_SUCCESS;

      gsl_matrix_memcpy(Xs, X);
      gsl_vector_memcpy(ys, y);

      if (w != NULL)
        {
          size_t i;

          /* construct Xs = sqrt(W) X and ys = sqrt(W) y */
          for (i = 0; i < n; ++i)
            {
              double wi = gsl_vector_get(w, i);
              double swi;
              gsl_vector_view row = gsl_matrix_row(Xs, i);
              double *yi = gsl_vector_ptr(ys, i);

              if (wi < 0.0)
                wi = 0.0;

              swi = sqrt(wi);
              gsl_vector_scale(&row.vector, swi);
              *yi *= swi;
            }
        }

      if (L != NULL)
        {
          size_t j;

          /* construct X~ = sqrt(W) X * L^{-1} matrix */
          for (j = 0; j < p; ++j)
            {
              gsl_vector_view Xj = gsl_matrix_column(Xs, j);
              double lj = gsl_vector_get(L, j);

              if (lj == 0.0)
                {
                  GSL_ERROR("L matrix is singular", GSL_EDOM);
                }

              gsl_vector_scale(&Xj.vector, 1.0 / lj);
            }
        }

      return status;
    }
}

/*
gsl_multifit_linear_stdform1()
  Using regularization matrix L = diag(l_1,l_2,...,l_p),
and W = I, transform to Tikhonov standard form:

X~ = X L^{-1}
y~ = y
c~ = L c

Inputs: L    - Tikhonov matrix as a vector of diagonal elements p-by-1
        X    - least squares matrix n-by-p
        y    - right hand side vector n-by-1
        Xs   - least squares matrix in standard form X~ n-by-p
        ys   - right hand side vector in standard form y~ n-by-1
        work - workspace

Return: success/error

Notes:
1) It is allowed for X = Xs
*/

int
gsl_multifit_linear_stdform1 (const gsl_vector * L,
                              const gsl_matrix * X,
                              const gsl_vector * y,
                              gsl_matrix * Xs,
                              gsl_vector * ys,
                              gsl_multifit_linear_workspace * work)
{
  int status;

  status = gsl_multifit_linear_wstdform1(L, X, NULL, y, Xs, ys, work);

  return status;
}

/*
gsl_multifit_linear_wstdform2()
  Using regularization matrix L which is m-by-p, transform to Tikhonov
standard form. This routine is separated into two cases:

Case 1: m >= p, here we can use the QR decomposition of L = QR, and note
that ||L c|| = ||R c|| where R is p-by-p. Therefore,

X~ = X R^{-1} is n-by-p
y~ = y is n-by-1
c~ is p-by-1
M is m-by-p (workspace)

Case 2: m < p

X~ is (n - p + m)-by-m
y~ is (n - p + m)-by-1
c~ is m-by-1
M is p-by-n (workspace)

Inputs: L    - regularization matrix m-by-p
        X    - least squares matrix n-by-p
        w    - weight vector n-by-1; or NULL for W = I
        y    - right hand side vector n-by-1
        Xs   - (output) least squares matrix in standard form
               case 1: n-by-p
               case 2: (n - p + m)-by-m
        ys   - (output) right hand side vector in standard form
               case 1: n-by-1
               case 2: (n - p + m)-by-1
        M    - (output) workspace matrix needed to reconstruct solution vector
               case 1: m-by-p
               case 2: p-by-n
        work - workspace

Return: success/error

Notes:
1) If L is square, on output:
   M(1:p,1:p) = R factor in QR decomposition of L
   Xs = X R^{-1}
   ys = y

2) If L is rectangular, on output:
   work->Linv = pseudo inverse of L
*/

int
gsl_multifit_linear_wstdform2 (const gsl_matrix * L,
                               const gsl_matrix * X,
                               const gsl_vector * w,
                               const gsl_vector * y,
                               gsl_matrix * Xs,
                               gsl_vector * ys,
                               gsl_matrix * M,
                               gsl_multifit_linear_workspace * work)
{
  const size_t m = L->size1;
  const size_t n = X->size1;
  const size_t p = X->size2;

  if (n > work->nmax || p > work->pmax)
    {
      GSL_ERROR("observation matrix larger than workspace", GSL_EBADLEN);
    }
  else if (p != L->size2)
    {
      GSL_ERROR("L and X matrices have different numbers of columns", GSL_EBADLEN);
    }
  else if (n != y->size)
    {
      GSL_ERROR("y vector does not match X", GSL_EBADLEN);
    }
  else if (w != NULL && n != w->size)
    {
      GSL_ERROR("weights vector must be length n", GSL_EBADLEN);
    }
  else if (m >= p) /* square or tall L matrix */
    {
      /* the sizes of Xs and ys depend on whether m >= p or m < p */
      if (n != Xs->size1 || p != Xs->size2)
        {
          GSL_ERROR("Xs matrix must be n-by-p", GSL_EBADLEN);
        }
      else if (n != ys->size)
        {
          GSL_ERROR("ys vector must have length n", GSL_EBADLEN);
        }
      else if (m != M->size1 || p != M->size2)
        {
          GSL_ERROR("M matrix must be m-by-p", GSL_EBADLEN);
        }
      else
        {
          int status;
          size_t i;
          gsl_matrix_view QR = gsl_matrix_submatrix(M, 0, 0, m, p);
          gsl_matrix_view R = gsl_matrix_submatrix(M, 0, 0, p, p);
          gsl_vector_view tau = gsl_vector_subvector(work->xt, 0, GSL_MIN(m, p));

          gsl_matrix_set_zero(M); /* not used */

          /* compute QR decomposition of L */
          gsl_matrix_memcpy(&QR.matrix, L);
          status = gsl_linalg_QR_decomp(&QR.matrix, &tau.vector);
          if (status)
            return status;

          if (w != NULL)
            {
              /* compute Xs = sqrt(W) X and ys = sqrt(W) y */
              status = gsl_multifit_linear_wstdform1(NULL, X, w, y, Xs, ys, work);
              if (status)
                return status;
            }
          else
            {
              gsl_vector_memcpy(ys, y);
              gsl_matrix_memcpy(Xs, X);
            }

          /* compute X~ = X R^{-1} using QR decomposition of L */
          for (i = 0; i < n; ++i)
            {
              gsl_vector_view v = gsl_matrix_row(Xs, i);

              /* solve: R^T y = X_i */
              gsl_blas_dtrsv(CblasUpper, CblasTrans, CblasNonUnit, &R.matrix, &v.vector);
            }

          return GSL_SUCCESS;
        }

    }
  else /* L matrix with m < p */
    {
      const size_t pm = p - m;
      const size_t npm = n - pm;

      if (npm != Xs->size1 || m != Xs->size2)
        {
          GSL_ERROR("Xs matrix must be (n-p+m)-by-m", GSL_EBADLEN);
        }
      else if (npm != ys->size)
        {
          GSL_ERROR("ys vector must be of length (n-p+m)", GSL_EBADLEN);
        }
      else if (p != M->size1 || n != M->size2)
        {
          GSL_ERROR("M matrix must be p-by-n", GSL_EBADLEN);
        }
      else
        {
          int status;
          gsl_vector_view tauv1 = gsl_vector_subvector(work->xt, 0, GSL_MIN(p, m));
          gsl_vector_view tauv2 = gsl_vector_subvector(work->t, 0, GSL_MIN(n, pm));
          gsl_matrix_view LT = gsl_matrix_submatrix(work->Q, 0, 0, p, m);          /* L^T */
          gsl_matrix_view Lp = gsl_matrix_submatrix(work->Linv, 0, 0, p, m);       /* L_inv */
          gsl_matrix_view B = gsl_matrix_submatrix(work->A, 0, 0, n, pm);          /* X * K_o */
          gsl_matrix_view C = gsl_matrix_submatrix(work->A, 0, 0, npm, p);         /* H_q^T X */

          gsl_matrix *K = gsl_matrix_alloc(p, p);
          gsl_matrix *H = gsl_matrix_alloc(n, n);
          gsl_matrix *M1 = gsl_matrix_alloc(pm, n);
          gsl_matrix *X1 = gsl_matrix_alloc(n, p);
          gsl_vector *y1 = gsl_vector_alloc(n);

          gsl_matrix_view Rp, Ko, Kp, Ho, To;
          size_t i;

          if (w != NULL)
            {
              GSL_ERROR("weights not yet supported for general L", GSL_EINVAL);
              /* compute X1 = sqrt(W) X and y1 = sqrt(W) y */
              status = gsl_multifit_linear_wstdform1(NULL, X, w, y, X1, y1, work);
              if (status)
                return status;
            }
          else
            {
              gsl_vector_memcpy(y1, y);
              gsl_matrix_memcpy(X1, X);
            }

          /* compute QR decomposition [K,R] = qr(L^T) */
          gsl_matrix_transpose_memcpy(&LT.matrix, L);
          status = gsl_linalg_QR_decomp(&LT.matrix, &tauv1.vector);
          if (status)
            return status;

          Rp = gsl_matrix_submatrix(&LT.matrix, 0, 0, m, m);

          gsl_linalg_Q_unpack(&LT.matrix, &tauv1.vector, K);
          Ko = gsl_matrix_submatrix(K, 0, m, p, pm);
          Kp = gsl_matrix_submatrix(K, 0, 0, p, m);

          /* compute QR decomposition [H,T] = qr(X * K_o) */
          gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, X1, &Ko.matrix, 0.0, &B.matrix);
          gsl_linalg_QR_decomp(&B.matrix, &tauv2.vector);
          gsl_linalg_Q_unpack(&B.matrix, &tauv2.vector, H);
          Ho = gsl_matrix_submatrix(H, 0, 0, n, pm);
          To = gsl_matrix_submatrix(&B.matrix, 0, 0, pm, pm);

          /* solve: R_p L_inv^T = K_p^T for L_inv */
          gsl_matrix_memcpy(&Lp.matrix, &Kp.matrix);
          for (i = 0; i < p; ++i)
            {
              gsl_vector_view x = gsl_matrix_row(&Lp.matrix, i);
              gsl_blas_dtrsv(CblasUpper, CblasNoTrans, CblasNonUnit, &Rp.matrix, &x.vector);
            }

          /* compute: ys = H_q^T y; this is equivalent to computing
           * the last q elements of H^T y (q = npm) */
          {
            gsl_vector_view v = gsl_vector_subvector(y1, pm, npm);
            gsl_linalg_QR_QTvec(&B.matrix, &tauv2.vector, y1);
            gsl_vector_memcpy(ys, &v.vector);
          }

          /* compute: M1 = inv(T_o) * H_o^T */
          gsl_matrix_transpose_memcpy(M1, &Ho.matrix);
          for (i = 0; i < n; ++i)
            {
              gsl_vector_view x = gsl_matrix_column(M1, i);
              gsl_blas_dtrsv(CblasUpper, CblasNoTrans, CblasNonUnit, &To.matrix, &x.vector);
            }

          /* compute: M = K_o * M1 */
          gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &Ko.matrix, M1, 0.0, M);

          /* compute: C = H_q^T X; this is equivalent to computing H^T X and
           * keeping the last q rows (q = npm) */
          {
            gsl_matrix_view m = gsl_matrix_submatrix(X1, pm, 0, npm, p);
            gsl_linalg_QR_QTmat(&B.matrix, &tauv2.vector, X1);
            gsl_matrix_memcpy(&C.matrix, &m.matrix);
          }
          /* compute: Xs = C * L_inv */
          gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &C.matrix, &Lp.matrix, 0.0, Xs);

          gsl_matrix_free(K);
          gsl_matrix_free(H);
          gsl_matrix_free(M1);
          gsl_matrix_free(X1);
          gsl_vector_free(y1);

          return GSL_SUCCESS;
        }
    }
}

int
gsl_multifit_linear_stdform2 (const gsl_matrix * L,
                              const gsl_matrix * X,
                              const gsl_vector * y,
                              gsl_matrix * Xs,
                              gsl_vector * ys,
                              gsl_matrix * M,
                              gsl_multifit_linear_workspace * work)
{
  int status;

  status = gsl_multifit_linear_wstdform2(L, X, NULL, y, Xs, ys, M, work);

  return status;
}

/*
gsl_multifit_linear_genform1()
  Backtransform regularized solution vector using matrix
L = diag(L)
*/

int
gsl_multifit_linear_genform1 (const gsl_vector * L,
                              const gsl_vector * cs,
                              gsl_vector * c,
                              gsl_multifit_linear_workspace * work)
{
  if (L->size > work->pmax)
    {
      GSL_ERROR("L vector does not match workspace", GSL_EBADLEN);
    }
  else if (L->size != cs->size)
    {
      GSL_ERROR("cs vector does not match L", GSL_EBADLEN);
    }
  else if (L->size != c->size)
    {
      GSL_ERROR("c vector does not match L", GSL_EBADLEN);
    }
  else
    {
      /* compute true solution vector c = L^{-1} c~ */
      gsl_vector_memcpy(c, cs);
      gsl_vector_div(c, L);

      return GSL_SUCCESS;
    }
}

/*
gsl_multifit_linear_genform2()
  Backtransform regularized solution vector using matrix L; (L,tau) contain
QR decomposition of original L

Inputs: L    - regularization matrix m-by-p
        X    - original least squares matrix n-by-p
        y    - original rhs vector n-by-1
        cs   - standard form solution vector
        c    - (output) original solution vector
        M    -
        work - workspace
*/

int
gsl_multifit_linear_genform2 (const gsl_matrix * L,
                              const gsl_matrix * X,
                              const gsl_vector * y,
                              const gsl_vector * cs,
                              const gsl_matrix * M,
                              gsl_vector * c,
                              gsl_multifit_linear_workspace * work)
{
  const size_t m = L->size1;
  const size_t n = X->size1;
  const size_t p = X->size2;

  if (n > work->nmax || p > work->pmax)
    {
      GSL_ERROR("X matrix does not match workspace", GSL_EBADLEN);
    }
  else if (p != L->size2)
    {
      GSL_ERROR("L matrix does not match X", GSL_EBADLEN);
    }
  else if (p != c->size)
    {
      GSL_ERROR("c vector does not match X", GSL_EBADLEN);
    }
  else if (n != y->size)
    {
      GSL_ERROR("y vector does not match X", GSL_EBADLEN);
    }
  else if (m >= p)                    /* square or tall L matrix */
    {
      if (p != cs->size)
        {
          GSL_ERROR("cs vector must be length p", GSL_EBADLEN);
        }
      else if (m != M->size1 || p != M->size2)
        {
          GSL_ERROR("M matrix must be m-by-p", GSL_EBADLEN);
        }
      else
        {
          int s;
          gsl_matrix_const_view R = gsl_matrix_const_submatrix(M, 0, 0, p, p); /* R factor of L */

          /* solve R c = cs for true solution c, using QR decomposition of L */
          gsl_vector_memcpy(c, cs);
          s = gsl_blas_dtrsv(CblasUpper, CblasNoTrans, CblasNonUnit, &R.matrix, c);

          return s;
        }
    }
  else                                /* rectangular L matrix with m < p */
    {
      if (m != cs->size)
        {
          GSL_ERROR("cs vector must be length m", GSL_EBADLEN);
        }
      else if (p != M->size1 || n != M->size2)
        {
          GSL_ERROR("M matrix must be size p-by-n", GSL_EBADLEN);
        }
      else
        {
          gsl_vector_view Linv_cs = gsl_vector_subvector(work->xt, 0, p);    /* L_inv * cs */
          gsl_vector_view workn = gsl_vector_subvector(work->t, 0, n);
          gsl_matrix_view Lp = gsl_matrix_submatrix(work->Linv, 0, 0, p, m); /* L_inv */

          /* compute L_inv * cs */
          gsl_blas_dgemv(CblasNoTrans, 1.0, &Lp.matrix, cs, 0.0, &Linv_cs.vector);

          /* compute: workn = y - X L_inv cs */
          gsl_vector_memcpy(&workn.vector, y);
          gsl_blas_dgemv(CblasNoTrans, -1.0, X, &Linv_cs.vector, 1.0, &workn.vector);

          /* compute: c = L_inv cs + M * workn */
          gsl_vector_memcpy(c, &Linv_cs.vector);
          gsl_blas_dgemv(CblasNoTrans, 1.0, M, &workn.vector, 1.0, c);

          return GSL_SUCCESS;
        }
    }
}

/*
gsl_multifit_linear_lreg()
  Calculate regularization parameters to use in L-curve
analysis

Inputs: smin      - smallest singular value of LS system
        smax      - largest singular value of LS system > 0
        reg_param - (output) vector of regularization parameters
                    derived from singular values

Return: success/error
*/

int
gsl_multifit_linear_lreg (const double smin, const double smax,
                          gsl_vector * reg_param)
{
  if (smax <= 0.0)
    {
      GSL_ERROR("smax must be positive", GSL_EINVAL);
    }
  else
    {
      const size_t N = reg_param->size;

      /* smallest regularization parameter */
      const double smin_ratio = 16.0 * GSL_DBL_EPSILON;
      const double new_smin = GSL_MAX(smin, smax*smin_ratio);
      double ratio;
      size_t i;

      gsl_vector_set(reg_param, N - 1, new_smin);

      /* ratio so that reg_param(1) = s(1) */
      ratio = pow(smax / new_smin, 1.0 / (N - 1.0));

      /* calculate the regularization parameters */
      for (i = N - 1; i > 0 && i--; )
        {
          double rp1 = gsl_vector_get(reg_param, i + 1);
          gsl_vector_set(reg_param, i, ratio * rp1);
        }

      return GSL_SUCCESS;
    }
}

/*
gsl_multifit_linear_lcurve()
  Calculate L-curve using regularization parameters estimated
from singular values of least squares matrix

Inputs: y         - right hand side vector
        reg_param - (output) vector of regularization parameters
                    derived from singular values
        rho       - (output) vector of residual norms ||y - X c||
        eta       - (output) vector of solution norms ||lambda c||
        work      - workspace

Return: success/error

Notes:
1) SVD of X must be computed first by calling multifit_linear_svd();
   work->n and work->p are initialized by this function
*/

int
gsl_multifit_linear_lcurve (const gsl_vector * y,
                            gsl_vector * reg_param,
                            gsl_vector * rho, gsl_vector * eta,
                            gsl_multifit_linear_workspace * work)
{
  const size_t n = y->size;
  const size_t N = rho->size; /* number of points on L-curve */

  if (n != work->n)
    {
      GSL_ERROR("y vector does not match workspace", GSL_EBADLEN);
    }
  else if (N < 3)
    {
      GSL_ERROR ("at least 3 points are needed for L-curve analysis",
                 GSL_EBADLEN);
    }
  else if (N != eta->size)
    {
      GSL_ERROR ("size of rho and eta vectors do not match",
                 GSL_EBADLEN);
    }
  else if (reg_param->size != eta->size)
    {
      GSL_ERROR ("size of reg_param and eta vectors do not match",
                 GSL_EBADLEN);
    }
  else
    {
      int status = GSL_SUCCESS;
      const size_t p = work->p;

      size_t i, j;

      gsl_matrix_view A = gsl_matrix_submatrix(work->A, 0, 0, n, p);
      gsl_vector_view S = gsl_vector_subvector(work->S, 0, p);
      gsl_vector_view xt = gsl_vector_subvector(work->xt, 0, p);
      gsl_vector_view workp = gsl_matrix_subcolumn(work->QSI, 0, 0, p);
      gsl_vector_view workp2 = gsl_vector_subvector(work->D, 0, p); /* D isn't used for regularized problems */

      const double smax = gsl_vector_get(&S.vector, 0);
      const double smin = gsl_vector_get(&S.vector, p - 1);

      double dr; /* residual error from projection */
      double normy = gsl_blas_dnrm2(y);
      double normUTy;

      /* compute projection xt = U^T y */
      gsl_blas_dgemv (CblasTrans, 1.0, &A.matrix, y, 0.0, &xt.vector);

      normUTy = gsl_blas_dnrm2(&xt.vector);
      dr = normy*normy - normUTy*normUTy;

      /* calculate regularization parameters */
      gsl_multifit_linear_lreg(smin, smax, reg_param);

      for (i = 0; i < N; ++i)
        {
          double lambda = gsl_vector_get(reg_param, i);
          double lambda_sq = lambda * lambda;

          for (j = 0; j < p; ++j)
            {
              double sj = gsl_vector_get(&S.vector, j);
              double xtj = gsl_vector_get(&xt.vector, j);
              double f = sj / (sj*sj + lambda_sq);

              gsl_vector_set(&workp.vector, j, f * xtj);
              gsl_vector_set(&workp2.vector, j, (1.0 - sj*f) * xtj);
            }

          gsl_vector_set(eta, i, gsl_blas_dnrm2(&workp.vector));
          gsl_vector_set(rho, i, gsl_blas_dnrm2(&workp2.vector));
        }

      if (n > p && dr > 0.0)
        {
          /* add correction to residual norm (see eqs 6-7 of [1]) */
          for (i = 0; i < N; ++i)
            {
              double rhoi = gsl_vector_get(rho, i);
              double *ptr = gsl_vector_ptr(rho, i);

              *ptr = sqrt(rhoi*rhoi + dr);
            }
        }

      /* restore D to identity matrix */
      gsl_vector_set_all(work->D, 1.0);

      return status;
    }
} /* gsl_multifit_linear_lcurve() */

/*
gsl_multifit_linear_lcorner()
  Determine point on L-curve of maximum curvature. For each
set of 3 points on the L-curve, the circle which passes through
the 3 points is computed. The radius of the circle is then used
as an estimate of the curvature at the middle point. The point
with maximum curvature is then selected.

Inputs: rho - vector of residual norms ||A x - b||
        eta - vector of solution norms ||L x||
        idx - (output) index i such that
              (log(rho(i)),log(eta(i))) is the point of
              maximum curvature

Return: success/error
*/

int
gsl_multifit_linear_lcorner(const gsl_vector *rho,
                            const gsl_vector *eta,
                            size_t *idx)
{
  const size_t n = rho->size;

  if (n < 3)
    {
      GSL_ERROR ("at least 3 points are needed for L-curve analysis",
                 GSL_EBADLEN);
    }
  else if (n != eta->size)
    {
      GSL_ERROR ("size of rho and eta vectors do not match",
                 GSL_EBADLEN);
    }
  else
    {
      int s = GSL_SUCCESS;
      size_t i;
      double x1, y1;      /* first point of triangle on L-curve */
      double x2, y2;      /* second point of triangle on L-curve */
      double rmin = -1.0; /* minimum radius of curvature */

      /* initial values */
      x1 = log(gsl_vector_get(rho, 0));
      y1 = log(gsl_vector_get(eta, 0));

      x2 = log(gsl_vector_get(rho, 1));
      y2 = log(gsl_vector_get(eta, 1));

      for (i = 1; i < n - 1; ++i)
        {
          /*
           * The points (x1,y1), (x2,y2), (x3,y3) are the previous,
           * current, and next point on the L-curve. We will find
           * the circle which fits these 3 points and take its radius
           * as an estimate of the curvature at this point.
           */
          double x3 = log(gsl_vector_get(rho, i + 1));
          double y3 = log(gsl_vector_get(eta, i + 1));

          double x21 = x2 - x1;
          double y21 = y2 - y1;
          double x31 = x3 - x1;
          double y31 = y3 - y1;
          double h21 = x21*x21 + y21*y21;
          double h31 = x31*x31 + y31*y31;
          double d = fabs(2.0 * (x21*y31 - x31*y21));
          double r = sqrt(h21*h31*((x3-x2)*(x3-x2)+(y3-y2)*(y3-y2))) / d;

          /* if d =~ 0 then there are nearly colinear points */
          if (gsl_finite(r))
            {
              /* check for smallest radius of curvature */
              if (r < rmin || rmin < 0.0)
                {
                  rmin = r;
                  *idx = i;
                }
            }

          /* update previous/current L-curve values */
          x1 = x2;
          y1 = y2;
          x2 = x3;
          y2 = y3;
        }

      /* check if a minimum radius was found */
      if (rmin < 0.0)
        {
          /* possibly co-linear points */
          GSL_ERROR("failed to find minimum radius", GSL_EINVAL);
        }

      return s;
    }
} /* gsl_multifit_linear_lcorner() */

/*
gsl_multifit_linear_lcorner2()
  Determine point on L-curve (lambda^2, ||c||^2) of maximum curvature.
For each set of 3 points on the L-curve, the circle which passes through
the 3 points is computed. The radius of the circle is then used
as an estimate of the curvature at the middle point. The point
with maximum curvature is then selected.

This routine is based on the paper

M. Rezghi and S. M. Hosseini, "A new variant of L-curve for Tikhonov
regularization", J. Comp. App. Math., 231 (2009).

Inputs: reg_param - vector of regularization parameters
        eta       - vector of solution norms ||L x||
        idx       - (output) index i such that
                    (lambda(i)^2,eta(i)^2) is the point of
                    maximum curvature

Return: success/error
*/

int
gsl_multifit_linear_lcorner2(const gsl_vector *reg_param,
                             const gsl_vector *eta,
                             size_t *idx)
{
  const size_t n = reg_param->size;

  if (n < 3)
    {
      GSL_ERROR ("at least 3 points are needed for L-curve analysis",
                 GSL_EBADLEN);
    }
  else if (n != eta->size)
    {
      GSL_ERROR ("size of reg_param and eta vectors do not match",
                 GSL_EBADLEN);
    }
  else
    {
      int s = GSL_SUCCESS;
      size_t i;
      double x1, y1;      /* first point of triangle on L-curve */
      double x2, y2;      /* second point of triangle on L-curve */
      double rmin = -1.0; /* minimum radius of curvature */

      /* initial values */
      x1 = gsl_vector_get(reg_param, 0);
      x1 *= x1;
      y1 = gsl_vector_get(eta, 0);
      y1 *= y1;

      x2 = gsl_vector_get(reg_param, 1);
      x2 *= x2;
      y2 = gsl_vector_get(eta, 1);
      y2 *= y2;

      for (i = 1; i < n - 1; ++i)
        {
          /*
           * The points (x1,y1), (x2,y2), (x3,y3) are the previous,
           * current, and next point on the L-curve. We will find
           * the circle which fits these 3 points and take its radius
           * as an estimate of the curvature at this point.
           */
          double lamip1 = gsl_vector_get(reg_param, i + 1);
          double etaip1 = gsl_vector_get(eta, i + 1);
          double x3 = lamip1 * lamip1;
          double y3 = etaip1 * etaip1;

          double x21 = x2 - x1;
          double y21 = y2 - y1;
          double x31 = x3 - x1;
          double y31 = y3 - y1;
          double h21 = x21*x21 + y21*y21;
          double h31 = x31*x31 + y31*y31;
          double d = fabs(2.0 * (x21*y31 - x31*y21));
          double r = sqrt(h21*h31*((x3-x2)*(x3-x2)+(y3-y2)*(y3-y2))) / d;

          /* if d =~ 0 then there are nearly colinear points */
          if (gsl_finite(r))
            {
              /* check for smallest radius of curvature */
              if (r < rmin || rmin < 0.0)
                {
                  rmin = r;
                  *idx = i;
                }
            }

          /* update previous/current L-curve values */
          x1 = x2;
          y1 = y2;
          x2 = x3;
          y2 = y3;
        }

      /* check if a minimum radius was found */
      if (rmin < 0.0)
        {
          /* possibly co-linear points */
          GSL_ERROR("failed to find minimum radius", GSL_EINVAL);
        }

      return s;
    }
} /* gsl_multifit_linear_lcorner2() */

#define GSL_MULTIFIT_MAXK      100

/*
gsl_multifit_linear_L()
  Compute discrete approximation to derivative operator of order
k on a regular grid of p points, ie: L is (p-k)-by-p
*/

int
gsl_multifit_linear_Lk(const size_t p, const size_t k, gsl_matrix *L)
{
  if (p <= k)
    {
      GSL_ERROR("p must be larger than derivative order", GSL_EBADLEN);
    }
  else if (k >= GSL_MULTIFIT_MAXK - 1)
    {
      GSL_ERROR("derivative order k too large", GSL_EBADLEN);
    }
  else if (p - k != L->size1 || p != L->size2)
    {
      GSL_ERROR("L matrix must be (p-k)-by-p", GSL_EBADLEN);
    }
  else
    {
      double c_data[GSL_MULTIFIT_MAXK];
      gsl_vector_view cv = gsl_vector_view_array(c_data, k + 1);
      size_t i, j;

      /* zeroth derivative */
      if (k == 0)
        {
          gsl_matrix_set_identity(L);
          return GSL_SUCCESS;
        }

      gsl_matrix_set_zero(L);
  
      gsl_vector_set_zero(&cv.vector);
      gsl_vector_set(&cv.vector, 0, -1.0);
      gsl_vector_set(&cv.vector, 1, 1.0);

      for (i = 1; i < k; ++i)
        {
          double cjm1 = 0.0;

          for (j = 0; j < k + 1; ++j)
            {
              double cj = gsl_vector_get(&cv.vector, j);

              gsl_vector_set(&cv.vector, j, cjm1 - cj);
              cjm1 = cj;
            }
        }

      /* build L, the c_i are the entries on the diagonals */
      for (i = 0; i < k + 1; ++i)
        {
          gsl_vector_view v = gsl_matrix_superdiagonal(L, i);
          double ci = gsl_vector_get(&cv.vector, i);

          gsl_vector_set_all(&v.vector, ci);
        }

      return GSL_SUCCESS;
    }
} /* gsl_multifit_linear_Lk() */

/*
gsl_multifit_linear_Lsobolev()
  Construct Sobolev smoothing norm operator

L = [ a_0 I; a_1 L_1; a_2 L_2; ...; a_k L_k ]

by computing the Cholesky factor of L^T L

Inputs: p     - number of columns of L
        kmax  - maximum derivative order (< p)
        alpha - vector of weights; alpha_k multiplies L_k, size kmax + 1
        L     - (output) Sobolev matrix p-by-p
        work  - workspace

Notes:
1) work->Linv is used to store intermediate L_k matrices
*/

int
gsl_multifit_linear_Lsobolev(const size_t p, const size_t kmax,
                             const gsl_vector *alpha, gsl_matrix *L,
                             gsl_multifit_linear_workspace *work)
{
  if (p > work->pmax)
    {
      GSL_ERROR("p is larger than workspace", GSL_EBADLEN);
    }
  else if (p <= kmax)
    {
      GSL_ERROR("p must be larger than derivative order", GSL_EBADLEN);
    }
  else if (kmax + 1 != alpha->size)
    {
      GSL_ERROR("alpha must be size kmax + 1", GSL_EBADLEN);
    }
  else if (p != L->size1)
    {
      GSL_ERROR("L matrix is wrong size", GSL_EBADLEN);
    }
  else if (L->size1 != L->size2)
    {
      GSL_ERROR("L matrix is not square", GSL_ENOTSQR);
    }
  else
    {
      int s;
      size_t j, k;
      gsl_vector_view d = gsl_matrix_diagonal(L);
      const double alpha0 = gsl_vector_get(alpha, 0);

      /* initialize L to alpha0^2 I */
      gsl_matrix_set_zero(L);
      gsl_vector_add_constant(&d.vector, alpha0 * alpha0);

      for (k = 1; k <= kmax; ++k)
        {
          gsl_matrix_view Lk = gsl_matrix_submatrix(work->Linv, 0, 0, p - k, p);
          double ak = gsl_vector_get(alpha, k);

          /* compute a_k L_k */
          s = gsl_multifit_linear_Lk(p, k, &Lk.matrix);
          if (s)
            return s;
          gsl_matrix_scale(&Lk.matrix, ak);

          /* LTL += L_k^T L_k */
          gsl_blas_dsyrk(CblasLower, CblasTrans, 1.0, &Lk.matrix, 1.0, L);
        }

      s = gsl_linalg_cholesky_decomp(L);
      if (s)
        return s;

      /* zero out lower triangle */
      for (j = 0; j < p; ++j)
        {
          for (k = 0; k < j; ++k)
            gsl_matrix_set(L, j, k, 0.0);
        }

      return GSL_SUCCESS;
    }
}