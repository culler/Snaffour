/*   This file is part of the program Snaffour.
 *
 *   Copyright (C) 2018 by Marc Culler, Nathan Dunfield, Matthias Görner
 *   and others.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Project homepage: https://bitbucket.org/t3m/snaffour/
 *   Author homepage: https://marc-culler.info
 *   Author homepage: http://dunfield.info
 *   Author homepage: http://www.unhyperbolic.org/
 */

#include "F4.h"

/** Struct to hold the result of an extended gcd computation.
 *
 * For inputs x and y, this struct is meant to hold d, a and b where
 * d is a greatest common divisor (with arbitrary sign) and d = ax + by.
 */

typedef struct gcd_s {
  int d;
  int a;
  int b;
} gcd_t;

/** Extended Greatest Common Divisor
 *
 * Compute d = gcd(x,y), a and b such that a*x + b*y = d.
 * The sign of d may or may not be positive.  If x and y are
 * both 0, the result is undefined.
 */

static void x_gcd(int x, int y, gcd_t* answer) {
  static int d, a, b;
  int q, r, temp;
  if (y == 0) {
    if (x == 0) {
      return;
    } else {
      d = x; a = 1; b = 0;
    }
  } else {
    q = x / y; r = x % y;
    x_gcd(y, r, NULL);
    temp = b; b = a - q*b; a = temp;
    if (answer != NULL) {
      answer->d = d; answer->a = a; answer->b = b;
    }
  }
}

/** \page modP Arithmetic mod p.
 *
 * All computations return an int value in the interval [0, p).
 * Multiplications must be done with 64-bit arithmetic to avoid overflow.
 */


/** Compute the inverse of a number modulo p
 */

int inverse_mod(int p, int x) {
  int a;
  gcd_t xgcd = {.d=0, .a=0, .b=0};
  x_gcd(x, p, &xgcd);
  a = xgcd.d < 0 ? (-xgcd.a) % p : xgcd.a % p;
  return a < 0 ? p + a : a;
}

/** Multiply two numbers mod p.
 *
 * Be careful about overflow!
 */

static inline int multiply_mod(int p, int x, int y) {
  int64_t P = (int64_t)p, X = (int64_t)x, Y = (int64_t)y, answer;
  if (x == 1) {
    answer = Y;
  } else if (x == -1) {
    answer = -Y;
  } else {
    answer = X*Y;
  }
  answer = answer % P;
  return (answer < 0 ? (int)(P + answer) : (int)answer);
}

/** Compute x + ay mod p
 *
 * This is the scalar version of a row operation.
 */

static inline int x_plus_ay_mod(int p, int x, int a, int y) {
  int64_t P = p, A = a, X = x, Y = y, answer;
  if (a == 1) {
    answer = X + Y;
  } else if (a == p - 1) {
    answer = X - Y;
  } else {
    answer = X + A*Y;
  }
  answer = answer % P;
  if (answer < 0) {
    answer += P;
  }
  return (int)answer;
}

/** \page modP Polynomials
 */

/** Allocate or reallocate memory for a Polynomial with specified maximum size.
 *
 * Sets num_terms to 0 and ensure that there is enough memory for up to "size"
 * terms and coefficients.  These arrays are enlarged with realloc if necessary.
 *
 * Note that allocation and deallocation of Polynomial_s structs is not
 * handled by this library.  That should be done by Cython.
 */

bool Poly_alloc(Polynomial_t* P, int size, int rank) {
  int old_size = P->max_size;
  if (size > old_size) {
    P->terms = realloc(P->terms, sizeof(Term_t)*size);
    if (P->terms == NULL) {
      return false;
    }
    P->coefficients = realloc(P->coefficients, sizeof(coeff_t)*size);
    if (P->coefficients == NULL) {
      free(P->terms);
      return false;
    }
    P->max_size = size;
  }
  P->num_terms = 0;
  P->rank = rank;
  return true;
}

/** Free the terms and coefficients of a Polynomial, if not NULL.
 *
 * The pointers are set to NULL after freeing the arrays and num_terms is
 * set to 0.
 */

void Poly_free(Polynomial_t* P) {
  if (P->terms != NULL) {
    free(P->terms);
  }
  if (P->coefficients != NULL) {
    free(P->coefficients);
  }
  P->num_terms = 0;
  P->rank = 0;
  P->terms = NULL;
  P->coefficients = NULL;
}

/** Copy a Polynomial's data into another Polynomial
 */
void Poly_copy(Polynomial_t* src, Polynomial_t* dest) {
  int i;
  /* First make sure there is enough room in the destinaion. */
  Poly_alloc(dest, src->num_terms, src->rank);
  for (i=0; i < src->num_terms; i++) {
    dest->terms[i] = src->terms[i];
    dest->coefficients[i] = src->coefficients[i];
  }
  dest->num_terms = src->num_terms;
}

/** Print a Polynomial in a crude way, without variable names.
 */

void Poly_print(Polynomial_t* P, int rank) {
  if (P->num_terms == 0) {
    printf("0\n");
  } else {
    for (int i=0; i<P->num_terms; i++) {
      printf("%d*", P->coefficients[i].value);
      Term_print(&P->terms[i], rank);
    }
    printf("\n");
  }
}

/** Static function to compare two Terms in two Polynomials
 *
 * Compare the pth term of P to the qth term of Q and return an integer which is
 * < 0 if the first one is smaller, > 0 if the second one is smaller and 0 if
 * they are equal.  The 0 term is considered larger than any non-zero term.
 */

static int Poly_compare_terms(Polynomial_t *P, int p, Polynomial_t *Q, int q) {
  if (P->num_terms > 0 && Q->num_terms > 0) {
    int P_index = P->coefficients[p].column_index;
    int Q_index = Q->coefficients[q].column_index;
    if (P_index >= 0 && Q_index >= 0) {
      /* If both column indexes are non-negative, use them for the comparison. */
      return P_index - Q_index;
    } else {
      /* Otherwise do the comparison from scratch. */
      Term_t *P_term = P->terms + p, *Q_term = Q->terms + q;
      int td1, td2, td_cmp;
      td1 = Term_total_degree(P_term, P->rank);
      td2 = Term_total_degree(Q_term, Q->rank);
      td_cmp = td1 - td2;
      return td_cmp == 0 ? Term_revlex_diff(Q_term, P_term, P->rank) : td_cmp;
    }
  } else if (P->num_terms == 0 && Q->num_terms == 0) {
    return 0;
  } else if (P->num_terms == 0) {
    return 1;
  } else {
      return -1;
    }
}

/** Static function to compute P + a*Q for an int a.
 *
 * This is the core of the row operation used to reduce a matrix to echelon form
 * and also handles addition and subtraction (by taking a=1 or a=-1).
 *
 * NOTE: P and Q must point to different polynomials for this to work.
 */

static bool Poly_p_plus_aq(Polynomial_t* P, int a, Polynomial_t* Q, Polynomial_t* answer,
                  int prime, int rank) {
  int size = P->num_terms + Q->num_terms, p = 0, q = 0, N = 0, cmp;
  coeff_t *p_coeff, *q_coeff;
  int value;
  if (! Poly_alloc(answer, size, rank)) {
    return false;
  }
  while (p < P->num_terms && q < Q->num_terms) {
    cmp = Poly_compare_terms(P, p, Q, q);
    if (cmp > 0) { /* deg P > deg Q */
      answer->terms[N] = P->terms[p];
      answer->coefficients[N] = P->coefficients[p];
      N++; p++;
    } else if (cmp < 0) { /* deg P < deg Q */
      q_coeff = Q->coefficients + q;
      answer->terms[N] = Q->terms[q];
      value = multiply_mod(prime, a, q_coeff->value);
      answer->coefficients[N].column_index = q_coeff->column_index;
      answer->coefficients[N].value = value;
      N++; q++;
    } else { /* deg P == deg Q */
      p_coeff = P->coefficients + p;
      q_coeff = Q->coefficients + q;
      value = x_plus_ay_mod(prime, p_coeff->value, a, q_coeff->value);
      if (value != 0) {
	answer->terms[N] = P->terms[p];
	answer->coefficients[N].column_index = p_coeff->column_index;
	answer->coefficients[N].value = value;
	N++;
      }
      p++; q++;
    }
  }
  /* At most one of these two loops will be non-trivial. */
  for (; q < Q->num_terms; q++, N++) {
    answer->terms[N] = Q->terms[q];
    q_coeff = Q->coefficients + q;
    value = multiply_mod(prime, a, q_coeff->value);
    answer->coefficients[N].column_index = q_coeff->column_index;
    answer->coefficients[N].value = value;
  }
  for (; p < P->num_terms; p++, N++) {
    answer->terms[N] = P->terms[p];
    answer->coefficients[N] = P->coefficients[p];
  }
  answer->num_terms = N;
  answer->rank = rank;
  return true;
}

/** Add Polynomials P and Q and store the result in answer.
 *
 * The work is done by Poly_p_plus_aq, but we need to deal with the special case
 * where P and Q are the same Polynomial.
 */

bool Poly_add(Polynomial_t* P, Polynomial_t* Q, Polynomial_t* answer, int prime, int rank) {
  if (P == Q) {
    int N = 0;
    coeff_t p_coeff;
    if (! Poly_alloc(answer, P->num_terms, rank)) {
      return false;
    }
    for (N=0; N < P->num_terms; N++) {
      answer->terms[N] = P->terms[N];
      p_coeff = P->coefficients[N];
      p_coeff.value = x_plus_ay_mod(prime, p_coeff.value, 1, p_coeff.value);
      answer->coefficients[N] = p_coeff;
    }
    answer->num_terms = N;
    answer->rank = rank;
    return true;
  }
  return Poly_p_plus_aq(P, 1, Q, answer, prime, rank);
}

/** Subtract Polynomials P and Q and store the result P - Q in answer.
 *
 * The work is done by Poly_p_plus_aq, but we need to deal with the special case
 * where P and Q are the same Polynomial (by returning a 0 Polynomial).
 */

bool Poly_sub(Polynomial_t* P, Polynomial_t* Q, Polynomial_t* answer, int prime, int rank) {
  if (P == Q) {
    answer->num_terms = 0;
    //Poly_free(answer);
    return true;
  }
  return Poly_p_plus_aq(P, prime - 1, Q, answer, prime, rank);
}

bool Poly_equals(Polynomial_t* P, Polynomial_t *Q) {
  if (P->num_terms != Q->num_terms) {
    return false;
  }
  for (int N = 0; N < P->num_terms; N++) {
    if (! Term_equals(P->terms + N, Q->terms + N)) {
      return false;
    }
    if (P->coefficients[N].value != Q->coefficients[N].value) {
      return false;
    }
  }
  return true;
}

/** Use bisection to find the index of the given term in P->terms for a Polynomial P.
 *
 * Return false if the term is not in P->terms.
 */

static bool find_index(Polynomial_t* P, Term_t* t, int t_td, int rank,
		      int bottom, int top, int* index) {
  int middle;
  int td;
  if (top - bottom == 1) {
    if (Term_equals(t, P->terms + bottom)) {
	*index = bottom;
	return true;
      } else {
	return false;
      }
  }
  middle = (top + bottom) >> 1;
  td = Term_total_degree(P->terms + middle, rank);
  if (td < t_td || (td == t_td && Term_revlex_diff(P->terms + middle, t, rank) > 0)) {
    return find_index(P, t, t_td, rank, bottom, middle, index);
  } else {
    return find_index(P, t, t_td, rank, middle, top, index);
  }
}

/** Return the coefficient of the given term in P.
 *
 * If P->terms does not contain the term, return 0.  This uses find_index to
 * find the coefficient or determine that it does not exist.
 */

int Poly_coeff(Polynomial_t* P, Term_t* t, int rank) {
  int index, t_td = Term_total_degree(t, rank);
  if (P->num_terms == 0) {
    return 0;
  }
  if (find_index(P, t, t_td, rank, 0, P->num_terms, &index)) {
    return P->coefficients[index].value;
  }
  return 0;
}

int Poly_column_index(Polynomial_t* P, Term_t* t, int rank) {
  int index, t_td = Term_total_degree(t, rank);
  if (P->num_terms == 0) {
    return 0;
  }
  if (find_index(P, t, t_td, rank, 0, P->num_terms, &index)) {
    return P->coefficients[index].column_index;
  }
  return 0;
}

/** Multiply a Polynomial by a Term.
 *
 * Much of the F4 algorithm works with "unevaluated products" (t, f) but eventually
 * they need to be evaluated.  This function does the evaluation.
 */

bool Poly_times_term(Polynomial_t *P, Term_t *t, Polynomial_t *answer, int prime, int rank) {
  int N = 0;
  coeff_t p_coeff;
  if (! Poly_alloc(answer, P->num_terms, rank)) {
    return false;
  }
  for (N=0; N < P->num_terms; N++) {
    Term_multiply(t, P->terms + N, answer->terms + N);
    p_coeff = P->coefficients[N];
    answer->coefficients[N] = p_coeff;
  }
  answer->num_terms = N;
  answer->rank = rank;
  return true;
}

/** Multiply a Polynomial by an int.
 */

bool Poly_times_int(Polynomial_t *P, int a, Polynomial_t *answer, int prime, int rank) {
  int N = 0;
  coeff_t p_coeff;
  if (! Poly_alloc(answer, P->num_terms, rank)) {
    return false;
  }
  for (N=0; N < P->num_terms; N++) {
    answer->terms[N] = P->terms[N];
    p_coeff = P->coefficients[N];
    p_coeff.value = multiply_mod(prime, a, p_coeff.value);
    answer->coefficients[N] = p_coeff;
  }
  answer->num_terms = N;
  answer->rank = rank;
  return true;
}

/** Divide a polynomial by the head coefficient.
 *
 * If the pointers P and answer are not identical, new terms and coefficient
 * arrays are allocated for the answer.  Otherwise, the coefficients of P
 * are modified in place.
 */

void Poly_make_monic(Polynomial_t *P, int prime, int rank) {
  int N = 0, a_inverse = inverse_mod(prime, P->coefficients[0].value);
  for (N=0; N < P->num_terms; N++) {
    P->coefficients[N].value = multiply_mod(prime, a_inverse, P->coefficients[N].value);
  }
}

/** A global zero polynomial.
 */

Polynomial_t zero = {.num_terms = 0, .max_size = 0, .terms = NULL, .coefficients = NULL};

/** The basic row operation
 *
 * Assume that f and g are both non-zero and have the same leading term.
 * Kill a term of g by subtracting an integer multiple of f.
 *
 * First subtract a*f from g where a is the coefficient in g of the head term of
 * f.  We assume that f is monic here, so after this row op, the new value of g
 * will not have a term equal to the head term of f.  In the case where f and g
 * have the same head term, this operation may not produce a monic result, so
 * we then divide g - a*f by its leading coefficient.
 */
static inline bool row_op(Polynomial_t *f, Polynomial_t *g, Polynomial_t *answer,
                          int g_coeff, int prime, int rank) {
  /* The coefficient should have been normalized to lie in [0, p).*/
  int a = prime - g_coeff;
  if (! Poly_p_plus_aq(g, a, f, answer, prime, rank)) {
    return false;
  }
  if (answer->coefficients != NULL && answer->coefficients[0].value != 1) {
    int a_inverse = inverse_mod(prime, answer->coefficients[0].value);
    for (int n = 0; n < answer->num_terms; n++) {
      int coeff = answer->coefficients[n].value;
      answer->coefficients[n].value = multiply_mod(prime, a_inverse, coeff);
    }
  }
  return true;
}


/*
 * Sort an array of type Polynomial_t, in place, by head term.  Zero
 * polynomials go to the end;
 *
 */

static int compare_heads(const void* p1, const void* p2) {
  Polynomial_t* P1 = (Polynomial_t*)p1;
  Polynomial_t* P2 = (Polynomial_t*)p2;
  return Poly_compare_terms(P1, 0, P2, 0);
}

static int compare_heads_dec(const void* p1, const void* p2) {
  Polynomial_t* P1 = (Polynomial_t*)p1;
  Polynomial_t* P2 = (Polynomial_t*)p2;
  return Poly_compare_terms(P2, 0, P1, 0);
}

void Poly_sort(Polynomial_t *P, int num_polys, bool increasing) {
  if (increasing) {
    qsort(P, num_polys, sizeof(Polynomial_t), compare_heads);
  } else {
    qsort(P, num_polys, sizeof(Polynomial_t), compare_heads_dec);
  }
}

/** Merge two arrays of monomials.
 */

static bool monomial_merge_two(monomial_array_t M0, monomial_array_t M1,
			       monomial_array_t *answer, int rank) {
  int size = M0.size + M1.size, p = 0, q = 0, N = 0, td_cmp, revlex_cmp;
  monomial_array_t merged;
  merged.monomials = (monomial_t*)malloc(sizeof(monomial_t)*size);
  if (merged.monomials == NULL) {
    answer->size = 0;
    answer->monomials = NULL;
    return false;
  }
  while (p < M0.size && q < M1.size) {
    td_cmp = M0.monomials[p].total_degree - M1.monomials[q].total_degree;
    if (td_cmp == 0) {
        Term_t *T0 = M0.monomials[p].term, *T1 = M1.monomials [q].term;
        revlex_cmp = Term_revlex_diff(T1, T0, rank);
    } else {
      revlex_cmp = 0;
    }
    if (td_cmp > 0 || revlex_cmp > 0) {
      /* M0[p] > M1[q] */
      merged.monomials[N++] = M0.monomials[p++];
    } else if (td_cmp < 0 || revlex_cmp < 0) {
      /* M1[q] > M0[p]*/
      merged.monomials[N++] = M1.monomials[q++];
    } else {
      /* M0[p] == M1[q] */
      merged.monomials[N++] = M0.monomials[p++];
      merged.monomials[N] = M1.monomials[q++];
      /* Mark the duplicate by setting its column_index to a special value. */
      merged.monomials[N++].coefficient->column_index = DUPLICATE;
    }
  }
  /* At most one of these two loops will be non-trivial. */
  while (p < M0.size) {
    merged.monomials[N++] = M0.monomials[p++];
  }
  while (q < M1.size) {
    merged.monomials[N++] = M1.monomials[q++];
  }
  merged.size = N;
  *answer = merged;
  return true;
}

/** Merge many arrays of monomials by divide and conquer.
 */

static bool monomial_merge(monomial_array_t* M, int num_arrays, monomial_array_t *answer,
			   int rank) {
  if (num_arrays == 1) {
    answer->monomials = (monomial_t*)malloc(M[0].size*sizeof(monomial_t));
    if (answer->monomials == NULL) {
      return false;
    }
    memcpy((void*)answer->monomials, M[0].monomials, M[0].size*sizeof(monomial_t));
    answer->size = M[0].size;
    return true;
  }
  if (num_arrays == 2) {
      return monomial_merge_two(M[0], M[1], answer, rank);
  }
  int left = num_arrays / 2, right = num_arrays - left;
  monomial_array_t left_answer, right_answer;
  bool result;
  if (! monomial_merge(M, left, &left_answer, rank)) {
    return false;
  }
  if (! monomial_merge(M + left, right, &right_answer, rank)) {
    free(left_answer.monomials);
    return false;
  }
  result = monomial_merge_two(left_answer, right_answer, answer, rank);
  free(left_answer.monomials);
  free(right_answer.monomials);
  return result;
}

/* Initialize a matrix
 *
 * Inputs an array P of polynomial pointers.  Sorts all of the terms which
 * appear in any of the polynomials and uses the sort position to set the
 * column_index field of each coefficient.  The purpose of this is to speed up
 * comparisons during the row operations.  (When the column_index values are
 * non-negative they are used for comparisons instead of the more expensive
 * comparison of total orders and lex ordering of exponents.  Negative values
 * serve as flags.)
 *
 * The number of columns in the matrix having the input polynomials as its rows
 * is stored in the int referenced by num_columns.
 */

static bool Poly_matrix_init(Polynomial_t **P, int num_rows, int *num_columns,
                             Polynomial_t *matrix, int prime, int rank) {
  int num_monomials = 0, i, j, index;
  monomial_t *pool;
  monomial_array_t monomial_arrays[num_rows], previous, merged;
  for (i = 0; i < num_rows; i++) {
    num_monomials += P[i]->num_terms;
  }
  pool = (monomial_t*)malloc(num_monomials*sizeof(monomial_t));
  if (pool == NULL) {
    return false;
  }
  previous.size = 0; previous.monomials = pool;
  for (i = 0; i < num_rows; i++) {
    monomial_arrays[i].size = P[i]->num_terms;
    monomial_arrays[i].monomials = previous.monomials + previous.size;
    previous = monomial_arrays[i];
  }
  for (i = 0; i < num_rows; i ++) {
    for (j = 0; j < P[i]->num_terms; j++){
      monomial_arrays[i].monomials[j].total_degree = Term_total_degree(P[i]->terms + j, rank);
      monomial_arrays[i].monomials[j].term = P[i]->terms + j;
      monomial_arrays[i].monomials[j].coefficient = P[i]->coefficients + j;
    }
  }
  if (! monomial_merge(monomial_arrays, num_rows, &merged, rank)) {
    printf("merge failed!\n");
  }
  index = 0;
  for (i = merged.size - 1; i >= 0; i--) {
    int saved_index = merged.monomials[i].coefficient->column_index;
    merged.monomials[i].coefficient->column_index = index;
    if (saved_index != DUPLICATE) {
      index++;
    }
  }
  *num_columns = index;
  free(merged.monomials);
  free(pool);
  for (i = 0; i < num_rows; i++) {
    matrix[i] = zero;
    if (!Poly_alloc(matrix + i, index, rank)) {
      for (j = 0; j < i; j++) {
        Poly_free(matrix + i);
        return false;
      }
    }
    Poly_copy(P[i], matrix + i);
  }
  return true;
}

/** Use bisection to find the coefficient of P with a given column index.
 *
 * Return false if no non-zero coefficient of P has the column index.
 * When the column indexes are available this is quite a bit faster.
 */

static bool coeff_in_column(Polynomial_t* P, int column, int bottom, int top,
                            int* coefficient) {
  int middle;
  if (top - bottom == 1) {
    if (column == P->coefficients[bottom].column_index) {
	*coefficient = P->coefficients[bottom].value;
	return true;
      } else {
	return false;
      }
  }
  middle = (top + bottom) >> 1;
  if (P->coefficients[middle].column_index < column) {
    return coeff_in_column(P, column, bottom, middle, coefficient);
  } else {
    return coeff_in_column(P, column, middle, top, coefficient);
  }
}

/** Echelon Form
 *
 * Input an array P of Polynomial pointers, and an array answer of unitialized
 * polynomials.  Both arrays should have size num_rows.  Each Polynomial gets
 * divided by its leading coefficient while being copied into the answer
 * array.  Then row operations are performed on the answer array until each
 * leading term occurs in exactly one row.
 */

bool Poly_echelon(Polynomial_t** P, Polynomial_t* answer, int num_rows, int* num_columns,
                  int prime, int rank) {
  int i, j, coeff;
  Polynomial_t *row_i, *row_j;
  Polynomial_t buffer = zero, tmp;
  int head;
  if (!Poly_matrix_init(P, num_rows, num_columns, answer, prime, rank)) {
    // free stuff ...
    return false;
  }
  if (!Poly_alloc(&buffer, *num_columns, rank)) {
    // free stuff ...
    return false;
  }
  for(i = 0; i < num_rows; i++) {
    Poly_make_monic(P[i], prime, rank);
  }
  /*
   * The best pivoting strategy I have found is to sort by increasing head term
   * once, at the beginning.  Doing subsequent sorts seems to cost more than it
   * saves.
   */
  qsort(answer, num_rows, sizeof(Polynomial_t), compare_heads);
  for (i = 0; i < num_rows; i++) {
    row_i = answer + i;
    if (row_i->num_terms == 0) continue;
    head = row_i->coefficients->column_index;
    for (j = 0; j < num_rows; j++) {
      if (i == j) continue;
      row_j = answer + j;
      if (row_j->num_terms == 0) continue;
      if (coeff_in_column(row_j, head, 0, row_j->num_terms, &coeff)) {
        if (! row_op(row_i, row_j, &buffer, coeff, prime, rank)) {
          return false;
        }
        tmp = answer[j];
        answer[j] = buffer;
        buffer = tmp;
      }
    }
  }
  Poly_free(&buffer);
  /*
   * While we are here in C land, lets sort the result by decreasing head term.
   */
  qsort(answer, num_rows, sizeof(Polynomial_t), compare_heads_dec);
  /*
   * Free unneeded memory and unset the column indexes so they don't cause
   * confusion when using the rows later.
   */
  for (i = 0; i < num_rows; i++) {
    row_i = answer + i;
    row_i->terms = realloc(row_i->terms, sizeof(Term_t)*row_i->num_terms);
    row_i->coefficients = realloc(row_i->coefficients, sizeof(coeff_t)*row_i->num_terms);
    if (row_i->num_terms == 0) continue;
    for (j = 0; j <  row_i->num_terms;  j++) {
      row_i->coefficients[j].column_index = INDEX_UNSET;
    }
  }
  return true;
}

// This should be constructed as part of the echelon form computation.

/*
 * Allocate an array and fill it with all of the distinct terms, in descending
 * order, that appear in the input array of Polynomials.
 */

bool Poly_terms(Polynomial_t *P, int num_polys, Term_t **answer, int* answer_size,
		int rank) {
  if (num_polys == 1) {
    *answer = (Term_t*)malloc(P[0].num_terms*sizeof(Term_t));
    if (*answer == NULL) {
      return false;
    }
    memcpy((void*)*answer, P[0].terms, P[0].num_terms*sizeof(Term_t));
    *answer_size = P[0].num_terms;
    return true;
  }
  if (num_polys == 2) {
      return Term_merge(P[0].terms, P[1].terms, P[0].num_terms, P[1].num_terms,
			answer, answer_size, rank);
  }
  int left = num_polys / 2, right = num_polys - left;
  Term_t *left_answer, *right_answer;
  int left_size, right_size;
  bool result;
  if (! Poly_terms(P, left, &left_answer, &left_size, rank)) {
    return false;
  }
  if (! Poly_terms(P + left, right, &right_answer, &right_size, rank)) {
    free(left_answer);
    return false;
  }
  result = Term_merge(left_answer, right_answer, left_size, right_size,
  		      answer, answer_size, rank);
  free(left_answer);
  free(right_answer);
  return result;
}
