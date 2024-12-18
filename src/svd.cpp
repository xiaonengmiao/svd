/******************************************************************************
 *
 * File:           svd.c
 *
 * Created:        11/05/2004
 *
 * Author:         Pavel Sakov
 *                 CSIRO Marine Research
 *
 * Purpose:        Singular value decomposition
 *                 Least squares fitting via singular value decomposition
 *
 * Description:    Provides SVD for dense matrices ported from EISPACK 
 *                 (1972-1973).
 *                 Provides least squares fitting via singular value 
 *                 decomposition; can take into account standard deviation of
 *                 individual measurements.
 *
 * Revisions:      12/1/2005 PS Added svd_lsq() for least squares fitting via 
 *                 SVD.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#define SVD_NMAX 40
#define SVD_EPS 4.0e-15

int svd_verbose = 0;

typedef struct {
    double* v;
    int i;
} indexedvalue;

static int cmp_iv(const void* p1, const void* p2)
{
    double v1 = *((indexedvalue *) p1)->v;
    double v2 = *((indexedvalue *) p2)->v;

    if (v1 > v2)
        return -1;
    if (v1 < v2)
        return 1;
    return 0;
}

static void sortvector(int n, double* v, int* pos)
{
    indexedvalue* iv = NULL;
    int i;

    if (n <= 0)
        return;

    iv = static_cast<indexedvalue*>(malloc(n * sizeof(indexedvalue)));

    for (i = 0; i < n; ++i) {
        iv[i].v = &v[i];
        iv[i].i = i;
    }

    qsort(iv, n, sizeof(indexedvalue), cmp_iv);

    for (i = 0; i < n; ++i)
        pos[i] = iv[i].i;

    free(iv);
}

static void quit(char* format, ...)
{
    va_list args;

    fflush(stdout);             /* just in case -- to have the exit message
                                 * last */

    fprintf(stderr, "\nerror: svd: ");
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    exit(1);
}

/* Allocates n1xn2 matrix of something. Note that it will be accessed as 
 * [n2][n1].
 * @param n1 Number of columns
 * @param n2 Number of rows
 * @return Matrix
 */
static void* alloc2d(int n1, int n2, size_t unitsize)
{
    size_t size;
    char* p;
    char** pp;
    int i;

    if (n1 <= 0 || n2 <= 0)
        quit("alloc2d(): invalid size (n1 = %d, n2 = %d)\n", n1, n2);

    size = n1 * n2;
    if ((p = static_cast<char *>(calloc(size, unitsize))) == NULL)
        quit("alloc2d(): %s\n", strerror(errno));

    size = n2 * sizeof(void*);
    if ((pp = static_cast<char **>(malloc(size))) == NULL)
        quit("alloc2d(): %s\n", strerror(errno));
    for (i = 0; i < n2; i++)
        pp[i] = &p[i * n1 * unitsize];

    return pp;
}

/* Destroys a matrix.
 * @param pp Matrix
 */
static void free2d(void* pp)
{
    void* p;

    p = ((void**) pp)[0];
    free(pp);
    free(p);
}

/** Performs singular value decomposition for a dense matrix.
 * Borrowed from EISPACK (1972-1973).
 *
 * The input matrix A is presented as  A = U.W.V'.
 *
 * @param A Input matrix A [0..m-1][0..n-1]; output matrix U
 * @param n Number of columns
 * @param m Number of rows
 * @param w Ouput vector [0..n-1] that presents diagonal matrix W 
 * @param V output matrix V [0..n-1][0..n-1] (not transposed)
 */
void svd(double** A, int n, int m, double* w, double** V)
{
    double* rv1;
    int i, j, k, l = -1;
    double tst1, c, f, g, h, s, scale;

    assert(m > 0 && n > 0);

    rv1 = static_cast<double*>(malloc(n * sizeof(double)));

    /*
     * householder reduction to bidiagonal form 
     */
    if (svd_verbose) {
        fprintf(stderr, "  svd: householder reduction:");
        fflush(stderr);
    }
    g = 0.0;
    scale = 0.0;
    tst1 = 0.0;
    for (i = 0; i < n; i++) {

        if (svd_verbose > 1) {
            fprintf(stderr, ".");
            fflush(stderr);
        }

        l = i + 1;
        rv1[i] = scale * g;
        g = 0.0;
        s = 0.0;
        scale = 0.0;
        if (i < m) {
            for (k = i; k < m; k++)
                scale += fabs(A[k][i]);
            if (scale != 0.0) {
                for (k = i; k < m; k++) {
                    A[k][i] /= scale;
                    s += A[k][i] * A[k][i];
                }
                f = A[i][i];
                g = -copysign(sqrt(s), f);
                h = f * g - s;
                A[i][i] = f - g;
                if (i < n - 1) {        /* no test in NR */
                    for (j = l; j < n; j++) {
                        s = 0.0;
                        for (k = i; k < m; k++)
                            s += A[k][i] * A[k][j];
                        f = s / h;
                        for (k = i; k < m; k++)
                            A[k][j] += f * A[k][i];
                    }
                }
                for (k = i; k < m; k++)
                    A[k][i] *= scale;
            }
        }
        w[i] = scale * g;
        g = 0.0;
        s = 0.0;
        scale = 0.0;
        if (i < m && i < n - 1) {
            for (k = l; k < n; k++)
                scale += fabs(A[i][k]);
            if (scale != 0.0) {
                for (k = l; k < n; k++) {
                    A[i][k] /= scale;
                    s += A[i][k] * A[i][k];
                }
                f = A[i][l];
                g = -copysign(sqrt(s), f);
                h = f * g - s;
                A[i][l] = f - g;
                for (k = l; k < n; k++)
                    rv1[k] = A[i][k] / h;
                for (j = l; j < m; j++) {
                    s = 0.0;
                    for (k = l; k < n; k++)
                        s += A[j][k] * A[i][k];
                    for (k = l; k < n; k++)
                        A[j][k] += s * rv1[k];
                }
                for (k = l; k < n; k++)
                    A[i][k] *= scale;
            }
        }
        {
            double tmp = fabs(w[i]) + fabs(rv1[i]);

            tst1 = (tst1 > tmp) ? tst1 : tmp;
        }
    }

    /*
     * accumulation of right-hand transformations 
     */
    if (svd_verbose) {
        fprintf(stderr, "\n  svd: accumulating right-hand transformations:");
        fflush(stderr);
    }
    for (i = n - 1; i >= 0; i--) {

        if (svd_verbose > 1) {
            fprintf(stderr, ".");
            fflush(stderr);
        }

        if (i < n - 1) {        /* no test in NR */
            if (g != 0.0) {
                for (j = l; j < n; j++)
                    /*
                     * double division avoids possible underflow 
                     */
                    V[j][i] = (A[i][j] / A[i][l]) / g;
                for (j = l; j < n; j++) {
                    s = 0.0;
                    for (k = l; k < n; k++)
                        s += A[i][k] * V[k][j];
                    for (k = l; k < n; k++)
                        V[k][j] += s * V[k][i];
                }
            }
            for (j = l; j < n; j++) {
                V[i][j] = 0.0;
                V[j][i] = 0.0;
            }
        }
        V[i][i] = 1.0;
        g = rv1[i];
        l = i;
    }

    /*
     * accumulation of left-hand transformations 
     */
    if (svd_verbose) {
        fprintf(stderr, "\n  svd: accumulating left-hand transformations:");
        fflush(stderr);
    }
    for (i = (m < n) ? m - 1 : n - 1; i >= 0; i--) {

        if (svd_verbose > 1) {
            fprintf(stderr, ".");
            fflush(stderr);
        }

        l = i + 1;
        g = w[i];
        if (i != n - 1)
            for (j = l; j < n; j++)
                A[i][j] = 0.0;
        if (g != 0.0) {
            for (j = l; j < n; j++) {
                s = 0.0;
                for (k = l; k < m; k++)
                    s += A[k][i] * A[k][j];
                /*
                 * double division avoids possible underflow
                 */
                f = (s / A[i][i]) / g;
                for (k = i; k < m; k++)
                    A[k][j] += f * A[k][i];
            }
            for (j = i; j < m; j++)
                A[j][i] /= g;
        } else
            for (j = i; j < m; j++)
                A[j][i] = 0.0;
        A[i][i] += 1.0;
    }

    /*
     * diagonalization of the bidiagonal form
     */
    if (svd_verbose) {
        fprintf(stderr, "\n  svd: diagonalization of the bidiagonal form:");
        fflush(stderr);
    }
    for (k = n - 1; k >= 0; k--) {
        int k1 = k - 1;
        int its = 0;

        if (svd_verbose > 1) {
            fprintf(stderr, ".");
            fflush(stderr);
        }

        while (1) {
            int docancellation = 1;
            double x, y, z;
            int l1 = -1;

            its++;
            if (its > SVD_NMAX)
                quit("svd(): no convergence in %d iterations\n", SVD_NMAX);

            for (l = k; l >= 0; l--) {  /* test for splitting */
                double tst2 = fabs(rv1[l]) + tst1;

                if (tst2 == tst1) {
                    docancellation = 0;
                    break;
                }
                l1 = l - 1;
                /*
                 * rv1(1) is always zero, so there is no exit through the
                 * bottom of the loop
                 */
                tst2 = fabs(w[l - 1]) + tst1;
                if (tst2 == tst1)
                    break;
            }
            /*
             * cancellation of rv1[l] if l > 1
             */
            if (docancellation) {
                c = 0.0;
                s = 1.0;
                for (i = l; i <= k; i++) {
                    f = s * rv1[i];
                    rv1[i] = c * rv1[i];
                    if ((fabs(f) + tst1) == tst1)
                        break;
                    g = w[i];
                    h = hypot(f, g);
                    w[i] = h;
                    c = g / h;
                    s = -f / h;
                    for (j = 0; j < m; j++) {
                        double y = A[j][l1];
                        double z = A[j][i];

                        A[j][l1] = y * c + z * s;
                        A[j][i] = z * c - y * s;
                    }
                }
            }
            /*
             * test for convergence
             */
            z = w[k];
            if (l != k) {
                int i1;

                /*
                 * shift from bottom 2 by 2 minor
                 */
                x = w[l];
                y = w[k1];
                g = rv1[k1];
                h = rv1[k];
                f = 0.5 * (((g + z) / h) * ((g - z) / y) + y / h - h / y);
                g = hypot(f, 1.0);
                f = x - (z / x) * z + (h / x) * (y / (f + copysign(g, f)) - h);
                /*
                 * next qr transformation 
                 */
                c = 1.0;
                s = 1.0;
                for (i1 = l; i1 < k; i1++) {
                    i = i1 + 1;
                    g = rv1[i];
                    y = w[i];
                    h = s * g;
                    g = c * g;
                    z = hypot(f, h);
                    rv1[i1] = z;
                    c = f / z;
                    s = h / z;
                    f = x * c + g * s;
                    g = g * c - x * s;
                    h = y * s;
                    y *= c;
                    for (j = 0; j < n; j++) {
                        x = V[j][i1];
                        z = V[j][i];
                        V[j][i1] = x * c + z * s;
                        V[j][i] = z * c - x * s;
                    }
                    z = hypot(f, h);
                    w[i1] = z;
                    /*
                     * rotation can be arbitrary if z = 0
                     */
                    if (z != 0.0) {
                        c = f / z;
                        s = h / z;
                    }
                    f = c * g + s * y;
                    x = c * y - s * g;
                    for (j = 0; j < m; j++) {
                        y = A[j][i1];
                        z = A[j][i];
                        A[j][i1] = y * c + z * s;
                        A[j][i] = z * c - y * s;
                    }
                }
                rv1[l] = 0.0;
                rv1[k] = f;
                w[k] = x;
            } else {
                /*
                 * w[k] is made non-negative
                 */
                if (z < 0.0) {
                    w[k] = -z;
                    for (j = 0; j < n; j++)
                        V[j][k] = -V[j][k];
                }
                break;
            }
        }
    }

    if (svd_verbose) {
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    free(rv1);
}

/** Performs sorting of SVD results in order of decreasing singular values.
 *
 * @param A Input-output matrix U [0..m-1][0..n-1]
 * @param n Number of columns
 * @param m Number of rows
 * @param w Input-ouput vector [0..n-1] that presents diagonal matrix W 
 * @param V Input-output matrix V [0..n-1][0..n-1] (not transposed)
 *
 * This function does the work but has downside that it requires temporal 
 * storage equal to the main storage. This may be  a problem for some large
 * applications, but one does not usually use dense SVD in such cases.
 */
void svd_sort(double** A, int n, int m, double* w, double** V)
{
    int* pos = static_cast<int *>(malloc(n * sizeof(int)));
    double* wold = static_cast<double *>(malloc(n * sizeof(double)));
    double** aold = static_cast<double **>(alloc2d(n, m, sizeof(double)));
    double** vold = static_cast<double **>(alloc2d(n, n, sizeof(double)));
    double wmax;
    int i, j;

    if (svd_verbose) {
        fprintf(stderr, "  svd: sorting:");
        fflush(stderr);
    }

    memcpy(wold, w, n * sizeof(double));
    memcpy(&aold[0][0], &A[0][0], m * n * sizeof(double));
    memcpy(&vold[0][0], &V[0][0], n * n * sizeof(double));

    sortvector(n, w, pos);

    wmax = w[pos[0]];

    for (i = 0; i < n; ++i) {
        w[i] = wold[pos[i]];

        if (w[i] / wmax < SVD_EPS)
            w[i] = 0.0;

        for (j = 0; j < m; ++j)
            A[j][i] = aold[j][pos[i]];
        for (j = 0; j < n; ++j)
            V[j][i] = vold[j][pos[i]];
    }

    free(pos);
    free(wold);
    free2d(aold);
    free2d(vold);

    if (svd_verbose) {
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

void svd_invs(double** AT, double** A, int n, int m, double* w, double** V)
{
    double** vtemp = static_cast<double **>(alloc2d(n, n, sizeof(double)));
    int mnmin;
    int i, j, k;

    memcpy(&vtemp[0][0], &V[0][0], n * n * sizeof(double));

    mnmin = (n < m) ? n : m;

    for (i = 0; i < mnmin; ++i)
    {
        for (j = 0; j < n; ++j)
        {
            vtemp[j][i] = V[j][i] / w[i];
        }     
    }

    for (i = 0; i < n; ++i)
    {
        for (j = 0; j < m; ++j)
        {
            AT[i][j] = 0.;
            for (k = 0; k < mnmin; ++k)
            {
                AT[i][j] += vtemp[i][k] * A[j][k];
            }
        }     
    }

}

static void usage()
{
    printf("Usage: svd <ncolumns> <nrows> <a_11> <a_12> ... <a_mn>\n");
    printf("E.g.:\n");
    printf("  ./svd 4 3 1 0 0 1 -1 0 2 1 1 2 0 1\n");
    printf("  ./svd 3 4 1 0 0 1 -1 0 2 1 1 2 0 1\n");
    exit(0);
}

static void matrix_print(int n, int m, double** A, char* offset)
{
    int i, j;

    for (j = 0; j < m; ++j) {
        printf("%s", offset);
        for (i = 0; i < n; ++i)
            printf("%10.5g ", fabs(A[j][i]) < SVD_EPS ? 0.0 : A[j][i]);
        printf("\n");
    }
}

int main(int argc, char* argv[])
{
    int m, n, mnmin, mnmax, i, j, k;
    double** A = nullptr;
    double** AT = nullptr;
    double** V = NULL;
    double* w = NULL;
    double** W = NULL;

    if (argc < 4)
        usage();

    n = atoi(argv[1]);
    m = atoi(argv[2]);
    mnmin = (n < m) ? n : m;
    mnmax = (n > m) ? n : m;

    if (n <= 0)
        quit("n = %d; expected n > 0\n", n);
    if (m <= 0)
        quit("m = %d; expected m > 0\n", m);

    if (argc != m * n + 3)
        usage();

    A = static_cast<double**>(alloc2d(n, m, sizeof(double)));
    AT = static_cast<double**>(alloc2d(m, n, sizeof(double)));

    for (j = 0, k = 3; j < m; ++j)
        for (i = 0; i < n; ++i, ++k)
            A[j][i] = atof(argv[k]);

    printf("A = \n");
    matrix_print(n, m, A, "  ");

    V = static_cast<double**>(alloc2d(n, n, sizeof(double)));
    w = static_cast<double*>(malloc(mnmax * sizeof(double)));
    W = static_cast<double**>(alloc2d(n, mnmax, sizeof(double)));

    printf("performing SVD:");

    svd(A, n, m, w, V);

    printf(" done\n");

    for (i = 0; i < n; ++i)
        W[i][i] = w[i];

    printf("U =\n");
    matrix_print(n, m, A, "  ");
    printf("W = \n");
    matrix_print(n, n, W, "  ");
    printf("V =\n");
    matrix_print(n, n, V, "  ");

    printf("performing sorting:");

    svd_sort(A, n, m, w, V);

    printf(" done\n");

    for (i = 0; i < n; ++i)
        W[i][i] = w[i];

    printf("U =\n");
    matrix_print(m, m, A, "  ");
    printf("W = \n");
    matrix_print(m, n, W, "  ");
    printf("V =\n");
    matrix_print(n, n, V, "  ");

    svd_invs(AT, A, n, m, w, V);

    printf(" done\n");

    printf("A.T =\n");
    matrix_print(n, m, AT, "  ");

    free2d(A);
    free(w);
    free2d(V);
    free2d(W);

    return 0;
}