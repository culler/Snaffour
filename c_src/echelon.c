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

/** Echelon forms
 * 
 * This file contains functions used by the echelon form computation, and functions
 * specific to Polynomials of compact flavor.
 */

#include "snaffour.h"

/** A global zero polynomial.
 */

Polynomial_t zero = {.num_terms = 0,
		     .max_size = 0,
		     .terms = NULL,
		     .coefficients = NULL};

static inline int compact_multiply_mod(int64_t prime64, int x, int y,
				       int64_t mu64) {
  int64_t x64 = (int64_t)x, y64 = (int64_t)y, answer64;
  answer64 = M_REDUCE(x64*y64, mu64, prime64);
  if (answer64 >= prime64) {
    answer64 -= prime64;
  }
  return (int)answer64;
}

static inline int compact_x_plus_ay_mod(int64_t prime64, int x, int a, int y,
					int64_t mu64) {
  int64_t x64 = x, y64 = y, a64 = a, answer64;
  answer64 = M_REDUCE(a64*y64, mu64, prime64);
  if (answer64 >= prime64) {
    answer64 -= prime64;
  }
  answer64 += x64;
  if (answer64 >= prime64) {
    answer64 -= prime64;
  }
  return (int)answer64;
}

/** Compress a Polynomial which is in transitional state.
 *
 * This assumes that the source is almost a Polynomial of normal flavor, but
 * its column indexes have been set to indexes into a table containing all of
 * its terms.  The destination should have been initialized to zero, and will
 * be changed into a Polynomial of compact flavor.
 */
static inline bool Poly_compress(Polynomial_t* src, Polynomial_t* dest,
                          Term_t* table, int prime, int mu, int R_squared) {
  int i, value;
  dest->table = table;
  /* First make sure there is enough room in the destination. */
  if (!Poly_alloc(dest, src->num_terms, src->rank)) {
    return false;
  }
  for (i=0; i < src->num_terms; i++) {
    value = src->coefficients[i].value;
    value = compact_multiply_mod(prime, value, R_squared, mu);
    dest->coefficients[i].column_index = src->coefficients[i].column_index;
    dest->coefficients[i].value = value;
  }
  dest->num_terms = src->num_terms;
  return true;
}

/** Decompress a compact Polynomial, converting it to normal flavor.
 */

static inline bool Poly_decompress(Polynomial_t* P, int prime, int mu) {
  int i, value;
  int64_t prime64 = prime, mu64 = mu;
  if (P->num_terms > 0) {
    P->terms = (Term_t*)malloc(P->num_terms*sizeof(Term_t));
    if (P->terms == NULL){
      return false;
    }
  }
  for (i = 0; i < P->num_terms; i++) {
    P->terms[i] = P->table[P->coefficients[i].column_index];
    value = compact_multiply_mod(prime64, P->coefficients[i].value, 1, mu64);
    P->coefficients[i].column_index = INDEX_UNSET;
    P->coefficients[i].value = value;
  }
  P->table = NULL;
  return true;
}

static inline bool Poly_p_plus_aq_compact(Polynomial_t* P, int a, Polynomial_t* Q,
                                  Polynomial_t* answer, int prime, int rank, int mu) {
  int size = P->num_terms + Q->num_terms, p = 0, q = 0, N = 0, cmp, new_value;
  coeff_t p_coeff, q_coeff;
  int64_t prime64 = prime, mu64 = mu;
  if (! Poly_alloc(answer, size, rank)) {
    return false;
  }
  answer->table = P->table;
  p_coeff = P->coefficients[0];
  q_coeff = Q->coefficients[0];
  while (p < P->num_terms && q < Q->num_terms) {
    cmp = p_coeff.column_index - q_coeff.column_index;
    if (cmp > 0) { /* deg P > deg Q */
      answer->coefficients[N++] = p_coeff;
      p_coeff = P->coefficients[++p];
    } else if (cmp < 0) { /* deg P < deg Q */
      new_value = compact_multiply_mod(prime64, a, q_coeff.value, mu64);
      answer->coefficients[N].column_index = q_coeff.column_index;
      answer->coefficients[N++].value = new_value;
      q_coeff = Q->coefficients[++q];
    } else { /* deg P == deg Q */
      new_value = compact_x_plus_ay_mod(prime64, p_coeff.value, a, q_coeff.value, mu64);
      if (new_value != 0) {
	answer->coefficients[N].column_index = p_coeff.column_index;
	answer->coefficients[N++].value = new_value;
      }
      p_coeff = P->coefficients[++p];
      q_coeff = Q->coefficients[++q];
    }
  }
  /* At most one of these two loops will be non-trivial. */
  for (; q < Q->num_terms; q++, N++) {
    q_coeff = Q->coefficients[q];
    new_value = compact_multiply_mod(prime64, a, q_coeff.value, mu64);
    answer->coefficients[N].column_index = q_coeff.column_index;
    answer->coefficients[N].value = new_value;
  }
  for (; p < P->num_terms; p++, N++) {
    answer->coefficients[N] = P->coefficients[p];
  }
  answer->num_terms = N;
  answer->rank = rank;
  return true;
}

/** The basic row operation
 *
 * Assume that f and g are both non-zero, that f is monic, and that the head term of
 * f appears in g.  Kill that term of g by subtracting a scalar multiple of f.
 *
 * First subtract a*f from g where a is the coefficient in g of the head term of
 * f.  Since f is monic, after this subtraction, the result will not have a term
 * equal to the head term of f.  However, in the case where the cancelled term
 * is the head term of g, i.e. f and g have the same head term, this operation
 * may not produce a monic result. So in that case we then divide g - a*f by its
 * leading coefficient.
 */

static inline bool row_op(Polynomial_t *f, Polynomial_t *g, Polynomial_t *answer,
                          int g_coeff, int prime, int rank, int mu, int R_cubed,
                          int R_mod_p) {
  /* The coefficient should have been normalized to lie in [0, p).*/
  /* Note that p - M(X) = M(p - X) = M(-X). */
  int a = prime - g_coeff;
  if (! Poly_p_plus_aq_compact(g, a, f, answer, prime, rank, mu)) {
    return false;
  }
  /* Make sure the new row is monic. This is always called with non-ero mu!*/
  if (answer->coefficients != NULL && answer->coefficients[0].value != R_mod_p) {
    /* Use the Montgomery inverse.
     * This does several divisions for each row op -- think about that!
     */
    int a_inverse = inverse_mod(prime, answer->coefficients[0].value);
    a_inverse = compact_multiply_mod(prime, a_inverse, R_cubed, mu);
    for (int n = 0; n < answer->num_terms; n++) {
      int coeff = answer->coefficients[n].value;
      answer->coefficients[n].value = compact_multiply_mod(prime, a_inverse, coeff, mu);
    }
  }
  return true;
}

/** Static function to compare two Terms in two Polynomials of compact flavor.
 *
 * Compare the pth term of P to the qth term of Q and return an integer which is
 * < 0 if the first one is smaller, > 0 if the second one is smaller and 0 if
 * they are equal.  The 0 term is considered larger than any non-zero term.
 */

static inline int compare_heads(const void* p1, const void* p2) {
  Polynomial_t* P1 = (Polynomial_t*)p1;
  Polynomial_t* P2 = (Polynomial_t*)p2;
  return P1->coefficients->column_index - P2->coefficients->column_index;
}

static inline int compare_heads_dec(const void* p1, const void* p2) {
  Polynomial_t* P1 = (Polynomial_t*)p1;
  Polynomial_t* P2 = (Polynomial_t*)p2;
  return P2->coefficients->column_index - P1->coefficients->column_index;
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

static bool monomial_merge(monomial_array_t* M, int num_arrays,
                           monomial_array_t *answer, int rank) {
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
  left_answer.monomials = NULL;
  right_answer.monomials = NULL;
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
 * appear in any of the polynomials and uses their sort position to set the
 * column_index field of each coefficient.  This prepares the polynomials for
 * conversion to the compact flavor, which is then carried out.
 *
 * The number of columns in the matrix having the input polynomials as its rows
 * is stored in the int referenced by the num_columns input, and the table of
 * all terms is stored in the term_table input.
 */

static bool Poly_matrix_init(Polynomial_t **P, int num_rows, int *num_columns,
                             Term_t **term_table, Polynomial_t *matrix,
			     int prime, int rank, int mu, int R_squared) {
  int num_monomials = 0, i, j, index;
  monomial_t *pool;
  monomial_array_t monomial_arrays[num_rows], previous, merged;
  Term_t *table = NULL;
  for (i = 0; i < num_rows; i++) {
    num_monomials += P[i]->num_terms;
  }
  if (NULL == (pool = (monomial_t*)malloc(num_monomials*sizeof(monomial_t)))) {
    goto oom;
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
  merged.monomials = NULL;
  if (! monomial_merge(monomial_arrays, num_rows, &merged, rank)) {
    goto oom;
  }
  if (NULL == (table = (Term_t*)malloc(merged.size*sizeof(Term_t)))) {
    goto oom;
  }
  index = 0;
  for (i = merged.size - 1; i >= 0; i--) {
    int saved_index = merged.monomials[i].coefficient->column_index;
    merged.monomials[i].coefficient->column_index = index;
    if (saved_index != DUPLICATE) {
      table[index] = *merged.monomials[i].term;
      index++;
    }
  }
  *num_columns = index;
  if (NULL == (table = realloc(table, index*sizeof(Term_t)))) {
    goto oom;
  }
  *term_table = table;
  free(merged.monomials);
  free(pool);
  for (i = 0; i < num_rows; i++) {
    matrix[i] = zero;
    if (!Poly_compress(P[i], matrix + i, table, prime, mu, R_squared)) {
      for (j = 0; j < i; j++) {
        Poly_free(matrix + i);
      }
      goto oom;
    }
  }
  return true;

 oom:
  free(merged.monomials);
  free(pool);
  free(table);
  return false;
}

/** Use bisection to find the coefficient of P with a given column index.
 *
 * Return false if no non-zero coefficient of P has the column index.
 * When the column indexes are available this is quite a bit faster.
 */

static inline bool coeff_in_column(Polynomial_t* P, int column, int bottom,
                                   int top, int* coefficient) {
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

bool Poly_echelon(Polynomial_t** P, Polynomial_t* answer, int num_rows,
                  int* num_columns, int prime, int rank) {
  int i, j, coeff, mu, R_squared, R_cubed, R_mod_p;
  int64_t prime64 = prime, radix64 = M_RADIX64, R_squared64, R_cubed64;
  Term_t* term_table = NULL;
  Polynomial_t *row_i, *row_j;
  Polynomial_t buffer = zero, tmp;
  int head, last_pivot = -1;
  /* Compute constants needed for the Montgomery representation. */
  /* First, mu, the negative inverse of p mod R.
   * This seems to work, even though M_RADIX is negative as a signed int.
   */
  mu = inverse_mod(M_RADIX, (int)(radix64 - prime64));
  assert(0 <= mu && mu & ~MOD_R == 0); 
  assert((1 + ((int64_t)mu * radix64) % radix64) == 0);
  /* R mod p, i.e. M(1) (used to check if a row is monic). */
  R_mod_p = (int)(radix64 % prime64);
  /* R^2 mod p (used to compute Montgomery representatives). */
  R_squared64 = (radix64 * radix64) % prime64;
  R_squared = (int)R_squared64;
  assert(0 <= R_squared && R_squared < prime);
  assert((R_squared - M_RADIX * M_RADIX) % prime == 0);
  /* R^3 mod p (used to compute Montgomery inverses). */
  R_cubed64 = (R_squared64 * radix64) % prime64;
  R_cubed = (int)R_cubed64;
  assert(0 <= R_cubed && R_cubed < prime);
  assert((R_cubed - M_RADIX * M_RADIX * M_RADIX) % prime == 0);
  
  if (!Poly_matrix_init(P, num_rows, num_columns, &term_table,
                        answer, prime, rank, mu, R_squared)) {
    goto oom;
  }
  buffer.table = term_table;
  if (!Poly_alloc(&buffer, *num_columns, rank)) {
    goto oom;
  }
  for(i = 0; i < num_rows; i++) {
    Poly_make_monic(P[i], prime, rank, mu, R_cubed);
  }
  /*
   * This implementation depends upon sorting the rows by increasing head term.
   */
  qsort(answer, num_rows, sizeof(Polynomial_t), compare_heads);
  for (i = 0; i < num_rows; i++) {
    row_i = answer + i;
    if (row_i->num_terms == 0) continue;
    head = row_i->coefficients->column_index;
    /* Clear above.  Since head terms are non-decreasing, we can skip this step
     * the first time that a new head term is seen because we know that the
     * column is already clear above.
     */
    if (head <= last_pivot) {
      for (j = 0; j < i; j++) {
        row_j = answer + j;
        if (row_j->num_terms == 0) continue;
        if (coeff_in_column(row_j, head, 0, row_j->num_terms, &coeff)) {
          if (! row_op(row_i, row_j, &buffer, coeff, prime, rank, mu, R_cubed, R_mod_p)) {
            return false;
          }
          tmp = answer[j];
          answer[j] = buffer;
          buffer = tmp;
        }
      }
    } else {
      last_pivot = head;
    }
    /* Clear below. */
    for (j = i + 1; j < num_rows; j++) {
      row_j = answer + j;
      if (row_j->num_terms == 0) continue;
      if (coeff_in_column(row_j, head, 0, row_j->num_terms, &coeff)) {
        if (! row_op(row_i, row_j, &buffer, coeff, prime, rank, mu, R_cubed, R_mod_p)) {
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
   * While we are here in C land, let's sort the result by decreasing head term.
   */
  qsort(answer, num_rows, sizeof(Polynomial_t), compare_heads_dec);
  /*
   * Free unneeded memory and convert the row polynomials back to standard flavor.
   */
  for (i = 0; i < num_rows; i++) {
    row_i = answer + i;
    if (row_i->num_terms == 0) {
      Poly_free(row_i);
      continue;
    }
    if (!Poly_decompress(row_i, prime, mu)) {
      for (j = 0; j <= i; j++) {
	Poly_free(answer + j);
      }
      goto oom;
    }
  }
  free(term_table);
  return true;

 oom:
  Poly_free(&buffer);
  free(term_table);
  return false;
}
