#if !defined(_SVD_H)
#define _SVD_H

extern int svd_verbose;

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
void svd(double** a, int n, int m, double* w, double** v);

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
void svd_sort(double** A, int n, int m, double* w, double** V);

/** Performs inversion of a matrix using SVD.
 *
 * @param A Input matrix A [0..m-1][0..n-1]
 * @param n Number of columns
 * @param m Number of rows
 * @param w Input-ouput vector [0..n-1] that presents diagonal matrix W 
 * @param V Input-output matrix V [0..n-1][0..n-1] (not transposed)
 * @param A_inv Input matrix A [0..m-1][0..n-1]; output matrix AT
 */
void svd_invs(double** A, int n, int m, double* w, double** V, double** A_inv);

#endif