
#include <NTL/mat_lzz_p.h>
#include <NTL/new.h>
#include <NTL/vec_long.h>


#include <NTL/BasicThreadPool.h>



#ifdef NTL_HAVE_AVX
#include <immintrin.h>
#endif

NTL_START_IMPL


#define PAR_THRESH_SQ (200)
#define PAR_THRESH (40000)


// *******************************************************
//
// Matrix Window data structure: perhaps some day this
// will be made public.
//
// *******************************************************

struct mat_window_zz_p {
   mat_zz_p &A;
   long r_offset;
   long c_offset;
   long nrows;
   long ncols;

   mat_window_zz_p(mat_zz_p& _A) : 
   A(_A), r_offset(0), c_offset(0), nrows(A.NumRows()), ncols(A.NumCols()) { }

   mat_window_zz_p(const mat_window_zz_p& w, long r1, long c1, long r2, long c2) :
   A(w.A) 
   {
      if (r1 < 0 || c1 < 0 || r2 < r1 || c2 < c1 || r2-r1 > w.nrows || c2-c1 > w.ncols)
         LogicError("mat_window_zz_p: bad args");

      r_offset = w.r_offset + r1;
      c_offset = w.c_offset + c1;
      nrows = r2-r1;
      ncols = c2-c1;
   }

   zz_p * operator[](long i) const { return &A[i+r_offset][c_offset]; }
   // DIRT: this assumes A[i].elts() is always non-null, which will
   // always be the case for matrices because of the semantics of FixLength.
   // Also, in the applications, the number of columns is always non-zero.

   long NumRows() const { return nrows; }
   long NumCols() const { return ncols; }

};


struct const_mat_window_zz_p {
   const mat_zz_p &A;
   long r_offset;
   long c_offset;
   long nrows;
   long ncols;

   const_mat_window_zz_p(const mat_zz_p& _A) : 
   A(_A), r_offset(0), c_offset(0), nrows(A.NumRows()), ncols(A.NumCols()) { }

   const_mat_window_zz_p(const mat_window_zz_p& w) :
   A(w.A), r_offset(w.r_offset), c_offset(w.c_offset), nrows(w.nrows), ncols(w.ncols) { }

   const_mat_window_zz_p(const const_mat_window_zz_p& w, long r1, long c1, long r2, long c2) :
   A(w.A) 
   {
      if (r1 < 0 || c1 < 0 || r2 < r1 || c2 < c1 || r2-r1 > w.nrows || c2-c1 > w.ncols)
         LogicError("const_mat_window_zz_p: bad args");

      r_offset = w.r_offset + r1;
      c_offset = w.c_offset + c1;
      nrows = r2-r1;
      ncols = c2-c1;
   }

   const zz_p * operator[](long i) const { return &A[i+r_offset][c_offset]; }
   // DIRT: this assumes A[i].elts() is always non-null, which will
   // always be the case for matrices because of the semantics of FixLength.
   // Also, in the applications, the number of columns is always non-zero.

   long NumRows() const { return nrows; }
   long NumCols() const { return ncols; }

};

void add(const mat_window_zz_p& X, 
         const const_mat_window_zz_p& A, const const_mat_window_zz_p& B)  
{  
   long n = A.NumRows();  
   long m = A.NumCols();  
  
   if (B.NumRows() != n || B.NumCols() != m)   
      LogicError("matrix add: dimension mismatch");  

   if (X.NumRows() != n || X.NumCols() != m)   
      LogicError("matrix add: dimension mismatch");  
  
   long p = zz_p::modulus();
  
   for (long i = 0; i < n; i++) {   
      zz_p *x = &X[i][0]; 
      const zz_p *a = &A[i][0]; 
      const zz_p *b = &B[i][0]; 
      for (long j = 0; j < m; j++) {  
         x[j].LoopHole() = AddMod(rep(a[j]), rep(b[j]), p);
      }
   }
}  

void sub(const mat_window_zz_p& X, 
         const const_mat_window_zz_p& A, const const_mat_window_zz_p& B)  
{  
   long n = A.NumRows();  
   long m = A.NumCols();  
  
   if (B.NumRows() != n || B.NumCols() != m)   
      LogicError("matrix sub: dimension mismatch");  

   if (X.NumRows() != n || X.NumCols() != m)   
      LogicError("matrix sub: dimension mismatch");  
  
   long p = zz_p::modulus();
  
   for (long i = 0; i < n; i++) {   
      zz_p *x = &X[i][0]; 
      const zz_p *a = &A[i][0]; 
      const zz_p *b = &B[i][0]; 
      for (long j = 0; j < m; j++) {  
         x[j].LoopHole() = SubMod(rep(a[j]), rep(b[j]), p);
      }
   }
}  


void clear(const mat_window_zz_p& X)
{
   long n = X.NumRows();  
   long m = X.NumCols();  

   for (long i = 0; i < n; i++)
      for (long j = 0; j < m; j++)
         clear(X[i][j]);
}



// ***********************************************************





  
void add(mat_zz_p& X, const mat_zz_p& A, const mat_zz_p& B)  
{  
   long n = A.NumRows();  
   long m = A.NumCols();  
  
   if (B.NumRows() != n || B.NumCols() != m)   
      LogicError("matrix add: dimension mismatch");  
  
   X.SetDims(n, m);  

   long p = zz_p::modulus();
  
   for (long i = 0; i < n; i++) {   
      zz_p *x = &X[i][0]; 
      const zz_p *a = A[i].elts();
      const zz_p *b = B[i].elts();
      for (long j = 0; j < m; j++) {  
         x[j].LoopHole() = AddMod(rep(a[j]), rep(b[j]), p);
      }
   }
}  
  
void sub(mat_zz_p& X, const mat_zz_p& A, const mat_zz_p& B)  
{  
   long n = A.NumRows();  
   long m = A.NumCols();  
  
   if (B.NumRows() != n || B.NumCols() != m)  
      LogicError("matrix sub: dimension mismatch");  
  
   X.SetDims(n, m);  

   long p = zz_p::modulus();
  
   for (long i = 0; i < n; i++) {   
      zz_p *x = &X[i][0]; 
      const zz_p *a = A[i].elts();
      const zz_p *b = B[i].elts();
      for (long j = 0; j < m; j++) {  
         x[j].LoopHole() = SubMod(rep(a[j]), rep(b[j]), p);
      }
   }
  
}  





void diag(mat_zz_p& X, long n, zz_p d)  
{  
   X.SetDims(n, n);  
   long i, j;  
  
   for (i = 1; i <= n; i++)  
      for (j = 1; j <= n; j++)  
         if (i == j)  
            X(i, j) = d;  
         else  
            clear(X(i, j));  
} 

long IsDiag(const mat_zz_p& A, long n, zz_p d)
{
   if (A.NumRows() != n || A.NumCols() != n)
      return 0;

   long i, j;

   for (i = 1; i <= n; i++)
      for (j = 1; j <= n; j++)
         if (i != j) {
            if (!IsZero(A(i, j))) return 0;
         }
         else {
            if (A(i, j) != d) return 0;
         }

   return 1;
}

void negate(mat_zz_p& X, const mat_zz_p& A)
{
   long n = A.NumRows();
   long m = A.NumCols();


   X.SetDims(n, m);

   long p = zz_p::modulus();
  
   for (long i = 0; i < n; i++) {   
      zz_p *x = &X[i][0]; 
      const zz_p *a = A[i].elts();
      for (long j = 0; j < m; j++) {  
         x[j].LoopHole() = NegateMod(rep(a[j]), p);
      }
   }
}

long IsZero(const mat_zz_p& a)
{
   long n = a.NumRows();
   long i;

   for (i = 0; i < n; i++)
      if (!IsZero(a[i]))
         return 0;

   return 1;
}

void clear(mat_zz_p& x)
{
   long n = x.NumRows();
   long i;
   for (i = 0; i < n; i++)
      clear(x[i]);
}

  
void ident(mat_zz_p& X, long n)  
{  
   X.SetDims(n, n);  
   long i, j;  
  
   for (i = 1; i <= n; i++)  
      for (j = 1; j <= n; j++)  
         if (i == j)  
            set(X(i, j));  
         else  
            clear(X(i, j));  
} 


long IsIdent(const mat_zz_p& A, long n)
{
   if (A.NumRows() != n || A.NumCols() != n)
      return 0;

   long i, j;

   for (i = 1; i <= n; i++)
      for (j = 1; j <= n; j++)
         if (i != j) {
            if (!IsZero(A(i, j))) return 0;
         }
         else {
            if (!IsOne(A(i, j))) return 0;
         }

   return 1;
}
            

void transpose(mat_zz_p& X, const mat_zz_p& A)
{
   long n = A.NumRows();
   long m = A.NumCols();

   long i, j;

   if (&X == & A) {
      if (n == m)
         for (i = 1; i <= n; i++)
            for (j = i+1; j <= n; j++)
               swap(X(i, j), X(j, i));
      else {
         mat_zz_p tmp;
         tmp.SetDims(m, n);
         for (i = 1; i <= n; i++)
            for (j = 1; j <= m; j++)
               tmp(j, i) = A(i, j);
         X.kill();
         X = tmp;
      }
   }
   else {
      X.SetDims(m, n);
      for (i = 1; i <= n; i++)
         for (j = 1; j <= m; j++)
            X(j, i) = A(i, j);
   }
}
   



void relaxed_power(mat_zz_p& X, const mat_zz_p& A, const ZZ& e, bool relax)
{
   if (A.NumRows() != A.NumCols()) LogicError("power: non-square matrix");

   if (e == 0) {
      ident(X, A.NumRows());
      return;
   }

   mat_zz_p T1, T2;
   long i, k;

   k = NumBits(e);
   T1 = A;

   for (i = k-2; i >= 0; i--) {
      sqr(T2, T1);
      if (bit(e, i))
         mul(T1, T2, A);
      else
         T1 = T2;
   }

   if (e < 0)
      relaxed_inv(X, T1, relax);
   else
      X = T1;
}



// ******************************************************************
//
// matrix-vector multiplication code
//
// ******************************************************************
  





void mul(vec_zz_p& x, const vec_zz_p& a, const mat_zz_p& B)
{
   long l = a.length();
   long m = B.NumCols();
  
   if (l != B.NumRows())  
      LogicError("matrix mul: dimension mismatch");  

   if (m == 0) { 

      x.SetLength(0);
      
   }
   else if (m == 1) {

      long p = zz_p::modulus();
      mulmod_t pinv = zz_p::ModulusInverse();

      long acc, tmp;
      long k;

      acc = 0;  
      for(k = 1; k <= l; k++) {  
         tmp = MulMod(rep(a(k)), rep(B(k,1)), p, pinv);  
         acc = AddMod(acc, tmp, p);  
      } 

      x.SetLength(1);
      x(1).LoopHole()  = acc;
          
   }
   else {  // m > 1.  precondition and EXEC_RANGE


      long p = zz_p::modulus();
      mulmod_t pinv = zz_p::ModulusInverse();

      NTL_TLS_LOCAL(vec_long, mul_aux_vec);
      vec_long::Watcher watch_mul_aux_vec(mul_aux_vec);
      mul_aux_vec.SetLength(m);
      long *acc = mul_aux_vec.elts();

      const zz_p* ap = a.elts();

      for (long j = 0; j < m; j++) acc[j] = 0;

      const bool seq = double(l)*double(m) < PAR_THRESH;

      NTL_GEXEC_RANGE(seq, m, first, last) {

         for (long k = 0;  k < l; k++) {
            long aa = rep(ap[k]);
            if (aa != 0) {
               const zz_p* bp = B[k].elts();
               long T1;
               mulmod_precon_t aapinv = PrepMulModPrecon(aa, p, pinv);
   
               for (long j = first; j < last; j++) {
                  T1 = MulModPrecon(rep(bp[j]), aa, p, aapinv);
                  acc[j] = AddMod(acc[j], T1, p);
               }
            } 
         }

      } NTL_GEXEC_RANGE_END

      x.SetLength(m);
      zz_p *xp = x.elts();
      for (long j = 0; j < m; j++)
         xp[j].LoopHole() = acc[j];    
   }
}

  
void mul_aux(vec_zz_p& x, const mat_zz_p& A, const vec_zz_p& b)
{
   long n = A.NumRows();
   long l = A.NumCols();

   if (l != b.length())
      LogicError("matrix mul: dimension mismatch");

   x.SetLength(n);
   zz_p* xp = x.elts();

   long p = zz_p::modulus();
   const zz_p* bp = b.elts();

   const bool seq = double(n)*double(l) < PAR_THRESH;


#ifdef NTL_HAVE_LL_TYPE

   if (cast_unsigned(l) <= (~(0UL))/cast_unsigned(p-1) &&
       cast_unsigned(l)*cast_unsigned(p-1) <= (~(0UL))/cast_unsigned(p-1))  {

      sp_reduce_struct red_struct = zz_p::red_struct();

      NTL_GEXEC_RANGE(seq, n, first, last) {

         for (long i = first; i < last; i++) {
            xp[i].LoopHole() = InnerProd_L(A[i].elts(), bp, l, p, red_struct);
         }

      } NTL_GEXEC_RANGE_END
   }
   else {
      sp_ll_reduce_struct ll_red_struct = zz_p::ll_red_struct();

      NTL_GEXEC_RANGE(seq, n, first, last) {

         for (long i = first; i < last; i++) {
            xp[i].LoopHole() = InnerProd_LL(A[i].elts(), bp, l, p, ll_red_struct);
         }

      } NTL_GEXEC_RANGE_END

   }

#else

   mulmod_t pinv = zz_p::ModulusInverse();

   if (n <= 1) {

      for (long i = 0; i < n; i++) {
         long acc = 0;
         const zz_p* ap = A[i].elts();

         for (long k = 0; k < l; k++) {
            long tmp = MulMod(rep(ap[k]), rep(bp[k]), p, pinv);
            acc = AddMod(acc, tmp, p);
         }

         xp[i].LoopHole() = acc;
      }

   }
   else {

      NTL_TLS_LOCAL(Vec<mulmod_precon_t>, precon_vec);
      Vec<mulmod_precon_t>::Watcher watch_precon_vec(precon_vec);
      precon_vec.SetLength(l);
      mulmod_precon_t *bpinv = precon_vec.elts();

      for (long k = 0; k < l; k++)
         bpinv[k] = PrepMulModPrecon(rep(bp[k]), p, pinv);

      
      NTL_GEXEC_RANGE(seq, n, first, last) {
         for (long i = first; i < last; i++) {
            long acc = 0;
            const zz_p* ap = A[i].elts();
           
            for (long k = 0; k < l; k++) {
               long tmp = MulModPrecon(rep(ap[k]), rep(bp[k]), p, bpinv[k]);
               acc = AddMod(acc, tmp, p);
            }
           
            xp[i].LoopHole() = acc;
         } 
      } NTL_GEXEC_RANGE_END

   }

#endif
}
  
void mul(vec_zz_p& x, const mat_zz_p& A, const vec_zz_p& b)  
{  
   if (&b == &x || A.position1(x) != -1) {
      vec_zz_p tmp;
      mul_aux(tmp, A, b);
      x = tmp;
   }
   else
      mul_aux(x, A, b);

}  


void mul(mat_zz_p& X, const mat_zz_p& A, zz_p b)
{
   long n = A.NumRows();
   long m = A.NumCols();

   X.SetDims(n, m);


   if (n == 0 || m == 0 || (n == 1 && m == 1)) {
      long i, j;

      for (i = 0; i < n; i++)
         for (j = 0; j < m; j++)
            mul(X[i][j], A[i][j], b);

   }
   else {
      
      long p = zz_p::modulus();
      mulmod_t pinv = zz_p::ModulusInverse();
      long bb = rep(b);
      mulmod_precon_t bpinv = PrepMulModPrecon(bb, p, pinv);

      const bool seq = double(n)*double(m) < PAR_THRESH;
      
      NTL_GEXEC_RANGE(seq, n, first, last) 
      long i, j;
      for (i = first; i < last; i++) {
         const zz_p *ap = A[i].elts();
         zz_p *xp = X[i].elts();

         for (j = 0; j < m; j++)
            xp[j].LoopHole() = MulModPrecon(rep(ap[j]), bb, p, bpinv);
      }
      NTL_GEXEC_RANGE_END


   }
}

void mul(mat_zz_p& X, const mat_zz_p& A, long b_in)
{
   zz_p b;
   b = b_in;
   mul(X, A, b);
} 


// ******************************************************************
//
// Code shared by block-matrix code
//
// ******************************************************************

#define MAT_BLK_SZ (32)


#ifdef NTL_HAVE_LL_TYPE

#ifdef NTL_HAVE_AVX

#define MAX_DBL_INT ((1L << NTL_DOUBLE_PRECISION)-1)
// max int representable exactly as a double
// this assumes NTL_DBL_PRECISION <= NTL_BITS_PER_LONG-2, which is
// checked in the code that tests for HAVE_AVX, but we check it here as
// well

#if (NTL_DBL_PRECISION > NTL_BITS_PER_LONG-2)
#error "NTL_DBL_PRECISION > NTL_BITS_PER_LONG-2"
#endif


// MUL_ADD(a, b, c): a += b*c
#ifdef NTL_HAVE_FMA
#define MUL_ADD(a, b, c) a = _mm256_fmadd_pd(b, c, a) 
#else
#define MUL_ADD(a, b, c) a = _mm256_add_pd(a, _mm256_mul_pd(b, c))
#endif

#if 0
static
void muladd1_by_32(double *x, const double *a, const double *b, long n)
{
   __m256d avec, bvec;


   __m256d acc0=_mm256_load_pd(x + 0*4);
   __m256d acc1=_mm256_load_pd(x + 1*4);
   __m256d acc2=_mm256_load_pd(x + 2*4);
   __m256d acc3=_mm256_load_pd(x + 3*4);
   __m256d acc4=_mm256_load_pd(x + 4*4);
   __m256d acc5=_mm256_load_pd(x + 5*4);
   __m256d acc6=_mm256_load_pd(x + 6*4);
   __m256d acc7=_mm256_load_pd(x + 7*4);


   for (long i = 0; i < n; i++) {
      avec = _mm256_broadcast_sd(a); a++;


      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc0, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc1, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc2, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc3, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc4, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc5, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc6, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc7, avec, bvec);
   }


   _mm256_store_pd(x + 0*4, acc0);
   _mm256_store_pd(x + 1*4, acc1);
   _mm256_store_pd(x + 2*4, acc2);
   _mm256_store_pd(x + 3*4, acc3);
   _mm256_store_pd(x + 4*4, acc4);
   _mm256_store_pd(x + 5*4, acc5);
   _mm256_store_pd(x + 6*4, acc6);
   _mm256_store_pd(x + 7*4, acc7);
}

#else

static
void muladd1_by_32(double *x, const double *a, const double *b, long n)
{
   __m256d acc0=_mm256_load_pd(x + 0*4);
   __m256d acc1=_mm256_load_pd(x + 1*4);
   __m256d acc2=_mm256_load_pd(x + 2*4);
   __m256d acc3=_mm256_load_pd(x + 3*4);
   __m256d acc4=_mm256_load_pd(x + 4*4);
   __m256d acc5=_mm256_load_pd(x + 5*4);
   __m256d acc6=_mm256_load_pd(x + 6*4);
   __m256d acc7=_mm256_load_pd(x + 7*4);

   long i = 0;
   for (; i <= n-4; i +=4) {

      // the following code sequences are a bit faster than
      // just doing 4 _mm256_broadcast_sd's
      // it requires a to point to aligned storage, however

#if 1
     // this one seems slightly faster
      __m256d a0101 = _mm256_broadcast_pd((const __m128d*)(a+0));
      __m256d a2323 = _mm256_broadcast_pd((const __m128d*)(a+2));
#else
      __m256d avec = _mm256_load_pd(a);
      __m256d a0101 = _mm256_permute2f128_pd(avec, avec, 0);
      __m256d a2323 = _mm256_permute2f128_pd(avec, avec, 0x11);

#endif

      __m256d avec0 = _mm256_permute_pd(a0101, 0);
      __m256d avec1 = _mm256_permute_pd(a0101, 0xf);
      __m256d avec2 = _mm256_permute_pd(a2323, 0);
      __m256d avec3 = _mm256_permute_pd(a2323, 0xf);

      a += 4;

      __m256d bvec;

      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc0, avec0, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc1, avec0, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc2, avec0, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc3, avec0, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc4, avec0, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc5, avec0, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc6, avec0, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc7, avec0, bvec);

      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc0, avec1, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc1, avec1, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc2, avec1, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc3, avec1, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc4, avec1, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc5, avec1, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc6, avec1, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc7, avec1, bvec);

      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc0, avec2, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc1, avec2, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc2, avec2, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc3, avec2, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc4, avec2, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc5, avec2, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc6, avec2, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc7, avec2, bvec);

      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc0, avec3, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc1, avec3, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc2, avec3, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc3, avec3, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc4, avec3, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc5, avec3, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc6, avec3, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc7, avec3, bvec);
   }

   for (; i < n; i++) {
      __m256d avec = _mm256_broadcast_sd(a); a++;
      __m256d bvec;

      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc0, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc1, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc2, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc3, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc4, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc5, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc6, avec, bvec);
      bvec = _mm256_load_pd(b); b += 4; MUL_ADD(acc7, avec, bvec);
   }


   _mm256_store_pd(x + 0*4, acc0);
   _mm256_store_pd(x + 1*4, acc1);
   _mm256_store_pd(x + 2*4, acc2);
   _mm256_store_pd(x + 3*4, acc3);
   _mm256_store_pd(x + 4*4, acc4);
   _mm256_store_pd(x + 5*4, acc5);
   _mm256_store_pd(x + 6*4, acc6);
   _mm256_store_pd(x + 7*4, acc7);
}

#endif

// experiment: process two rows at a time
#if 1
static
void muladd2_by_32(double *x, const double *a, const double *b, long n)
{
   __m256d avec0, avec1, bvec;
   __m256d acc00, acc01, acc02, acc03;
   __m256d acc10, acc11, acc12, acc13;
 

   // round 0

   acc00=_mm256_load_pd(x + 0*4 + 0*MAT_BLK_SZ);
   acc01=_mm256_load_pd(x + 1*4 + 0*MAT_BLK_SZ);
   acc02=_mm256_load_pd(x + 2*4 + 0*MAT_BLK_SZ);
   acc03=_mm256_load_pd(x + 3*4 + 0*MAT_BLK_SZ);

   acc10=_mm256_load_pd(x + 0*4 + 1*MAT_BLK_SZ);
   acc11=_mm256_load_pd(x + 1*4 + 1*MAT_BLK_SZ);
   acc12=_mm256_load_pd(x + 2*4 + 1*MAT_BLK_SZ);
   acc13=_mm256_load_pd(x + 3*4 + 1*MAT_BLK_SZ);

   for (long i = 0; i < n; i++) {
      avec0 = _mm256_broadcast_sd(&a[i]); 
      avec1 = _mm256_broadcast_sd(&a[i+MAT_BLK_SZ]); 

      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+0*4]); MUL_ADD(acc00, avec0, bvec); MUL_ADD(acc10, avec1, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+1*4]); MUL_ADD(acc01, avec0, bvec); MUL_ADD(acc11, avec1, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+2*4]); MUL_ADD(acc02, avec0, bvec); MUL_ADD(acc12, avec1, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+3*4]); MUL_ADD(acc03, avec0, bvec); MUL_ADD(acc13, avec1, bvec);
   }


   _mm256_store_pd(x + 0*4 + 0*MAT_BLK_SZ, acc00);
   _mm256_store_pd(x + 1*4 + 0*MAT_BLK_SZ, acc01);
   _mm256_store_pd(x + 2*4 + 0*MAT_BLK_SZ, acc02);
   _mm256_store_pd(x + 3*4 + 0*MAT_BLK_SZ, acc03);

   _mm256_store_pd(x + 0*4 + 1*MAT_BLK_SZ, acc10);
   _mm256_store_pd(x + 1*4 + 1*MAT_BLK_SZ, acc11);
   _mm256_store_pd(x + 2*4 + 1*MAT_BLK_SZ, acc12);
   _mm256_store_pd(x + 3*4 + 1*MAT_BLK_SZ, acc13);

   // round 1

   acc00=_mm256_load_pd(x + 4*4 + 0*MAT_BLK_SZ);
   acc01=_mm256_load_pd(x + 5*4 + 0*MAT_BLK_SZ);
   acc02=_mm256_load_pd(x + 6*4 + 0*MAT_BLK_SZ);
   acc03=_mm256_load_pd(x + 7*4 + 0*MAT_BLK_SZ);

   acc10=_mm256_load_pd(x + 4*4 + 1*MAT_BLK_SZ);
   acc11=_mm256_load_pd(x + 5*4 + 1*MAT_BLK_SZ);
   acc12=_mm256_load_pd(x + 6*4 + 1*MAT_BLK_SZ);
   acc13=_mm256_load_pd(x + 7*4 + 1*MAT_BLK_SZ);

   for (long i = 0; i < n; i++) {
      avec0 = _mm256_broadcast_sd(&a[i]); 
      avec1 = _mm256_broadcast_sd(&a[i+MAT_BLK_SZ]); 

      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+0*4+MAT_BLK_SZ/2]); MUL_ADD(acc00, avec0, bvec); MUL_ADD(acc10, avec1, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+1*4+MAT_BLK_SZ/2]); MUL_ADD(acc01, avec0, bvec); MUL_ADD(acc11, avec1, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+2*4+MAT_BLK_SZ/2]); MUL_ADD(acc02, avec0, bvec); MUL_ADD(acc12, avec1, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+3*4+MAT_BLK_SZ/2]); MUL_ADD(acc03, avec0, bvec); MUL_ADD(acc13, avec1, bvec);
   }


   _mm256_store_pd(x + 4*4 + 0*MAT_BLK_SZ, acc00);
   _mm256_store_pd(x + 5*4 + 0*MAT_BLK_SZ, acc01);
   _mm256_store_pd(x + 6*4 + 0*MAT_BLK_SZ, acc02);
   _mm256_store_pd(x + 7*4 + 0*MAT_BLK_SZ, acc03);

   _mm256_store_pd(x + 4*4 + 1*MAT_BLK_SZ, acc10);
   _mm256_store_pd(x + 5*4 + 1*MAT_BLK_SZ, acc11);
   _mm256_store_pd(x + 6*4 + 1*MAT_BLK_SZ, acc12);
   _mm256_store_pd(x + 7*4 + 1*MAT_BLK_SZ, acc13);

}

#else

static
void muladd2_by_32(double *x, const double *a, const double *b, long n)
{
   long i, j;
   __m256d bvec;
   __m256d acc00, acc01, acc02, acc03;
   __m256d acc10, acc11, acc12, acc13;


   for (j = 0; j < 2; j++) {

      acc00=_mm256_load_pd(x + 0*4 + 0*MAT_BLK_SZ + j*(MAT_BLK_SZ/2));
      acc01=_mm256_load_pd(x + 1*4 + 0*MAT_BLK_SZ + j*(MAT_BLK_SZ/2));
      acc02=_mm256_load_pd(x + 2*4 + 0*MAT_BLK_SZ + j*(MAT_BLK_SZ/2));
      acc03=_mm256_load_pd(x + 3*4 + 0*MAT_BLK_SZ + j*(MAT_BLK_SZ/2));

      acc10=_mm256_load_pd(x + 0*4 + 1*MAT_BLK_SZ + j*(MAT_BLK_SZ/2));
      acc11=_mm256_load_pd(x + 1*4 + 1*MAT_BLK_SZ + j*(MAT_BLK_SZ/2));
      acc12=_mm256_load_pd(x + 2*4 + 1*MAT_BLK_SZ + j*(MAT_BLK_SZ/2));
      acc13=_mm256_load_pd(x + 3*4 + 1*MAT_BLK_SZ + j*(MAT_BLK_SZ/2));

      for (i = 0; i <= n-4; i+=4) {
	 __m256d a0_0101 = _mm256_broadcast_pd((const __m128d*)(a+i+0));
	 __m256d a0_2323 = _mm256_broadcast_pd((const __m128d*)(a+i+2));
	 __m256d avec00 = _mm256_permute_pd(a0_0101, 0);
	 __m256d avec01 = _mm256_permute_pd(a0_0101, 0xf);
	 __m256d avec02 = _mm256_permute_pd(a0_2323, 0);
	 __m256d avec03 = _mm256_permute_pd(a0_2323, 0xf);

	 __m256d a1_0101 = _mm256_broadcast_pd((const __m128d*)(a+i+0+MAT_BLK_SZ));
	 __m256d a1_2323 = _mm256_broadcast_pd((const __m128d*)(a+i+2+MAT_BLK_SZ));
	 __m256d avec10 = _mm256_permute_pd(a1_0101, 0);
	 __m256d avec11 = _mm256_permute_pd(a1_0101, 0xf);
	 __m256d avec12 = _mm256_permute_pd(a1_2323, 0);
	 __m256d avec13 = _mm256_permute_pd(a1_2323, 0xf);

	 bvec = _mm256_load_pd(&b[(i+0)*MAT_BLK_SZ+0*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc00, avec00, bvec); MUL_ADD(acc10, avec10, bvec);
	 bvec = _mm256_load_pd(&b[(i+0)*MAT_BLK_SZ+1*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc01, avec00, bvec); MUL_ADD(acc11, avec10, bvec);
	 bvec = _mm256_load_pd(&b[(i+0)*MAT_BLK_SZ+2*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc02, avec00, bvec); MUL_ADD(acc12, avec10, bvec);
	 bvec = _mm256_load_pd(&b[(i+0)*MAT_BLK_SZ+3*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc03, avec00, bvec); MUL_ADD(acc13, avec10, bvec);

	 bvec = _mm256_load_pd(&b[(i+1)*MAT_BLK_SZ+0*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc00, avec01, bvec); MUL_ADD(acc10, avec11, bvec);
	 bvec = _mm256_load_pd(&b[(i+1)*MAT_BLK_SZ+1*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc01, avec01, bvec); MUL_ADD(acc11, avec11, bvec);
	 bvec = _mm256_load_pd(&b[(i+1)*MAT_BLK_SZ+2*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc02, avec01, bvec); MUL_ADD(acc12, avec11, bvec);
	 bvec = _mm256_load_pd(&b[(i+1)*MAT_BLK_SZ+3*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc03, avec01, bvec); MUL_ADD(acc13, avec11, bvec);

	 bvec = _mm256_load_pd(&b[(i+2)*MAT_BLK_SZ+0*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc00, avec02, bvec); MUL_ADD(acc10, avec12, bvec);
	 bvec = _mm256_load_pd(&b[(i+2)*MAT_BLK_SZ+1*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc01, avec02, bvec); MUL_ADD(acc11, avec12, bvec);
	 bvec = _mm256_load_pd(&b[(i+2)*MAT_BLK_SZ+2*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc02, avec02, bvec); MUL_ADD(acc12, avec12, bvec);
	 bvec = _mm256_load_pd(&b[(i+2)*MAT_BLK_SZ+3*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc03, avec02, bvec); MUL_ADD(acc13, avec12, bvec);

	 bvec = _mm256_load_pd(&b[(i+3)*MAT_BLK_SZ+0*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc00, avec03, bvec); MUL_ADD(acc10, avec13, bvec);
	 bvec = _mm256_load_pd(&b[(i+3)*MAT_BLK_SZ+1*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc01, avec03, bvec); MUL_ADD(acc11, avec13, bvec);
	 bvec = _mm256_load_pd(&b[(i+3)*MAT_BLK_SZ+2*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc02, avec03, bvec); MUL_ADD(acc12, avec13, bvec);
	 bvec = _mm256_load_pd(&b[(i+3)*MAT_BLK_SZ+3*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc03, avec03, bvec); MUL_ADD(acc13, avec13, bvec);
      }

      for (; i < n; i++) {
	 __m256d avec0 = _mm256_broadcast_sd(&a[i]); 
	 __m256d avec1 = _mm256_broadcast_sd(&a[i+MAT_BLK_SZ]); 

	 bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+0*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc00, avec0, bvec); MUL_ADD(acc10, avec1, bvec);
	 bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+1*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc01, avec0, bvec); MUL_ADD(acc11, avec1, bvec);
	 bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+2*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc02, avec0, bvec); MUL_ADD(acc12, avec1, bvec);
	 bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+3*4+j*(MAT_BLK_SZ/2)]); MUL_ADD(acc03, avec0, bvec); MUL_ADD(acc13, avec1, bvec);
      }


      _mm256_store_pd(x + 0*4 + 0*MAT_BLK_SZ + j*(MAT_BLK_SZ/2), acc00);
      _mm256_store_pd(x + 1*4 + 0*MAT_BLK_SZ + j*(MAT_BLK_SZ/2), acc01);
      _mm256_store_pd(x + 2*4 + 0*MAT_BLK_SZ + j*(MAT_BLK_SZ/2), acc02);
      _mm256_store_pd(x + 3*4 + 0*MAT_BLK_SZ + j*(MAT_BLK_SZ/2), acc03);

      _mm256_store_pd(x + 0*4 + 1*MAT_BLK_SZ + j*(MAT_BLK_SZ/2), acc10);
      _mm256_store_pd(x + 1*4 + 1*MAT_BLK_SZ + j*(MAT_BLK_SZ/2), acc11);
      _mm256_store_pd(x + 2*4 + 1*MAT_BLK_SZ + j*(MAT_BLK_SZ/2), acc12);
      _mm256_store_pd(x + 3*4 + 1*MAT_BLK_SZ + j*(MAT_BLK_SZ/2), acc13);

   }
}
#endif



// experiment: process three rows at a time
// NOTE: this makes things slower on an AVX1 platform --- not enough registers
// it could be faster on AVX2/FMA, where there should be enough registers

static
void muladd3_by_32(double *x, const double *a, const double *b, long n)
{
   __m256d avec0, avec1, avec2, bvec;
   __m256d acc00, acc01, acc02, acc03;
   __m256d acc10, acc11, acc12, acc13;
   __m256d acc20, acc21, acc22, acc23;
 

   // round 0

   acc00=_mm256_load_pd(x + 0*4 + 0*MAT_BLK_SZ);
   acc01=_mm256_load_pd(x + 1*4 + 0*MAT_BLK_SZ);
   acc02=_mm256_load_pd(x + 2*4 + 0*MAT_BLK_SZ);
   acc03=_mm256_load_pd(x + 3*4 + 0*MAT_BLK_SZ);

   acc10=_mm256_load_pd(x + 0*4 + 1*MAT_BLK_SZ);
   acc11=_mm256_load_pd(x + 1*4 + 1*MAT_BLK_SZ);
   acc12=_mm256_load_pd(x + 2*4 + 1*MAT_BLK_SZ);
   acc13=_mm256_load_pd(x + 3*4 + 1*MAT_BLK_SZ);

   acc20=_mm256_load_pd(x + 0*4 + 2*MAT_BLK_SZ);
   acc21=_mm256_load_pd(x + 1*4 + 2*MAT_BLK_SZ);
   acc22=_mm256_load_pd(x + 2*4 + 2*MAT_BLK_SZ);
   acc23=_mm256_load_pd(x + 3*4 + 2*MAT_BLK_SZ);

   for (long i = 0; i < n; i++) {
      avec0 = _mm256_broadcast_sd(&a[i]); 
      avec1 = _mm256_broadcast_sd(&a[i+MAT_BLK_SZ]); 
      avec2 = _mm256_broadcast_sd(&a[i+2*MAT_BLK_SZ]); 

      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+0*4]); MUL_ADD(acc00, avec0, bvec); MUL_ADD(acc10, avec1, bvec); MUL_ADD(acc20, avec2, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+1*4]); MUL_ADD(acc01, avec0, bvec); MUL_ADD(acc11, avec1, bvec); MUL_ADD(acc21, avec2, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+2*4]); MUL_ADD(acc02, avec0, bvec); MUL_ADD(acc12, avec1, bvec); MUL_ADD(acc22, avec2, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+3*4]); MUL_ADD(acc03, avec0, bvec); MUL_ADD(acc13, avec1, bvec); MUL_ADD(acc23, avec2, bvec);
   }


   _mm256_store_pd(x + 0*4 + 0*MAT_BLK_SZ, acc00);
   _mm256_store_pd(x + 1*4 + 0*MAT_BLK_SZ, acc01);
   _mm256_store_pd(x + 2*4 + 0*MAT_BLK_SZ, acc02);
   _mm256_store_pd(x + 3*4 + 0*MAT_BLK_SZ, acc03);

   _mm256_store_pd(x + 0*4 + 1*MAT_BLK_SZ, acc10);
   _mm256_store_pd(x + 1*4 + 1*MAT_BLK_SZ, acc11);
   _mm256_store_pd(x + 2*4 + 1*MAT_BLK_SZ, acc12);
   _mm256_store_pd(x + 3*4 + 1*MAT_BLK_SZ, acc13);

   _mm256_store_pd(x + 0*4 + 2*MAT_BLK_SZ, acc20);
   _mm256_store_pd(x + 1*4 + 2*MAT_BLK_SZ, acc21);
   _mm256_store_pd(x + 2*4 + 2*MAT_BLK_SZ, acc22);
   _mm256_store_pd(x + 3*4 + 2*MAT_BLK_SZ, acc23);

   // round 1

   acc00=_mm256_load_pd(x + 4*4 + 0*MAT_BLK_SZ);
   acc01=_mm256_load_pd(x + 5*4 + 0*MAT_BLK_SZ);
   acc02=_mm256_load_pd(x + 6*4 + 0*MAT_BLK_SZ);
   acc03=_mm256_load_pd(x + 7*4 + 0*MAT_BLK_SZ);

   acc10=_mm256_load_pd(x + 4*4 + 1*MAT_BLK_SZ);
   acc11=_mm256_load_pd(x + 5*4 + 1*MAT_BLK_SZ);
   acc12=_mm256_load_pd(x + 6*4 + 1*MAT_BLK_SZ);
   acc13=_mm256_load_pd(x + 7*4 + 1*MAT_BLK_SZ);

   acc20=_mm256_load_pd(x + 4*4 + 2*MAT_BLK_SZ);
   acc21=_mm256_load_pd(x + 5*4 + 2*MAT_BLK_SZ);
   acc22=_mm256_load_pd(x + 6*4 + 2*MAT_BLK_SZ);
   acc23=_mm256_load_pd(x + 7*4 + 2*MAT_BLK_SZ);

   for (long i = 0; i < n; i++) {
      avec0 = _mm256_broadcast_sd(&a[i]); 
      avec1 = _mm256_broadcast_sd(&a[i+MAT_BLK_SZ]); 
      avec2 = _mm256_broadcast_sd(&a[i+2*MAT_BLK_SZ]); 

      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+0*4+MAT_BLK_SZ/2]); MUL_ADD(acc00, avec0, bvec); MUL_ADD(acc10, avec1, bvec); MUL_ADD(acc20, avec2, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+1*4+MAT_BLK_SZ/2]); MUL_ADD(acc01, avec0, bvec); MUL_ADD(acc11, avec1, bvec); MUL_ADD(acc21, avec2, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+2*4+MAT_BLK_SZ/2]); MUL_ADD(acc02, avec0, bvec); MUL_ADD(acc12, avec1, bvec); MUL_ADD(acc22, avec2, bvec);
      bvec = _mm256_load_pd(&b[i*MAT_BLK_SZ+3*4+MAT_BLK_SZ/2]); MUL_ADD(acc03, avec0, bvec); MUL_ADD(acc13, avec1, bvec); MUL_ADD(acc23, avec2, bvec);
   }


   _mm256_store_pd(x + 4*4 + 0*MAT_BLK_SZ, acc00);
   _mm256_store_pd(x + 5*4 + 0*MAT_BLK_SZ, acc01);
   _mm256_store_pd(x + 6*4 + 0*MAT_BLK_SZ, acc02);
   _mm256_store_pd(x + 7*4 + 0*MAT_BLK_SZ, acc03);

   _mm256_store_pd(x + 4*4 + 1*MAT_BLK_SZ, acc10);
   _mm256_store_pd(x + 5*4 + 1*MAT_BLK_SZ, acc11);
   _mm256_store_pd(x + 6*4 + 1*MAT_BLK_SZ, acc12);
   _mm256_store_pd(x + 7*4 + 1*MAT_BLK_SZ, acc13);

   _mm256_store_pd(x + 4*4 + 2*MAT_BLK_SZ, acc20);
   _mm256_store_pd(x + 5*4 + 2*MAT_BLK_SZ, acc21);
   _mm256_store_pd(x + 6*4 + 2*MAT_BLK_SZ, acc22);
   _mm256_store_pd(x + 7*4 + 2*MAT_BLK_SZ, acc23);

}

static inline
void muladd_all_by_32(long first, long last, double *x, const double *a, const double *b, long n)
{
   long i = first;
#ifdef NTL_HAVE_FMA
   // processing three rows at a time is faster
   for (; i <= last-3; i+=3)
      muladd3_by_32(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b, n);
   for (; i < last; i++)
      muladd1_by_32(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b, n);
#else
   // process only two rows at a time: not enough registers :-(
   for (; i <= last-2; i+=2)
      muladd2_by_32(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b, n);
   for (; i < last; i++)
      muladd1_by_32(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b, n);
#endif
}


// this assumes n is a multiple of 16
static inline
void muladd_interval(double * NTL_RESTRICT x, double * NTL_RESTRICT y, double c, long n)
{
   __m256d xvec0, xvec1, xvec2, xvec3;
   __m256d yvec0, yvec1, yvec2, yvec3;

   __m256d cvec = _mm256_broadcast_sd(&c);

   for (long i = 0; i < n; i += 16, x += 16, y += 16) {
      xvec0 = _mm256_load_pd(x+0*4);
      xvec1 = _mm256_load_pd(x+1*4);
      xvec2 = _mm256_load_pd(x+2*4);
      xvec3 = _mm256_load_pd(x+3*4);

      yvec0 = _mm256_load_pd(y+0*4);
      yvec1 = _mm256_load_pd(y+1*4);
      yvec2 = _mm256_load_pd(y+2*4);
      yvec3 = _mm256_load_pd(y+3*4);

      MUL_ADD(xvec0, yvec0, cvec);
      MUL_ADD(xvec1, yvec1, cvec);
      MUL_ADD(xvec2, yvec2, cvec);
      MUL_ADD(xvec3, yvec3, cvec);

      _mm256_store_pd(x + 0*4, xvec0);
      _mm256_store_pd(x + 1*4, xvec1);
      _mm256_store_pd(x + 2*4, xvec2);
      _mm256_store_pd(x + 3*4, xvec3);
   }
}

// this one is more general: does not assume that n is a
// multiple of 16
static inline
void muladd_interval1(double * NTL_RESTRICT x, double * NTL_RESTRICT y, double c, long n)
{

   __m256d xvec0, xvec1, xvec2, xvec3;
   __m256d yvec0, yvec1, yvec2, yvec3;
   __m256d cvec;

   if (n >= 4)
      cvec = _mm256_broadcast_sd(&c);

   long i=0;
   for (; i <= n-16; i += 16, x += 16, y += 16) {
      xvec0 = _mm256_load_pd(x+0*4);
      xvec1 = _mm256_load_pd(x+1*4);
      xvec2 = _mm256_load_pd(x+2*4);
      xvec3 = _mm256_load_pd(x+3*4);

      yvec0 = _mm256_load_pd(y+0*4);
      yvec1 = _mm256_load_pd(y+1*4);
      yvec2 = _mm256_load_pd(y+2*4);
      yvec3 = _mm256_load_pd(y+3*4);

      MUL_ADD(xvec0, yvec0, cvec);
      MUL_ADD(xvec1, yvec1, cvec);
      MUL_ADD(xvec2, yvec2, cvec);
      MUL_ADD(xvec3, yvec3, cvec);

      _mm256_store_pd(x + 0*4, xvec0);
      _mm256_store_pd(x + 1*4, xvec1);
      _mm256_store_pd(x + 2*4, xvec2);
      _mm256_store_pd(x + 3*4, xvec3);
   }

   for (; i <= n-4; i += 4, x += 4, y += 4) {
      xvec0 = _mm256_load_pd(x+0*4);
      yvec0 = _mm256_load_pd(y+0*4);
      MUL_ADD(xvec0, yvec0, cvec);
      _mm256_store_pd(x + 0*4, xvec0);
   }

   for (; i < n; i++, x++, y++) {
      *x += (*y)*c;
   }
}

#define AVX_PD_SZ (4)

// experimental: assumes n is a multiple of 4 in the range [0..32]
#if 1
static inline
void muladd_interval2(double * NTL_RESTRICT x, double * NTL_RESTRICT y, double c, long n)
{
   n /= 4;
   if (n <= 0 || n > 8) return;

   x += n*4;
   y += n*4;

   // n in [1..8]

   __m256d xvec, yvec, cvec;

   cvec = _mm256_broadcast_sd(&c);

   switch (n) {
   case 8: xvec = _mm256_load_pd(x-8*4); yvec = _mm256_load_pd(y-8*4); MUL_ADD(xvec, yvec, cvec); _mm256_store_pd(x-8*4, xvec);
   case 7: xvec = _mm256_load_pd(x-7*4); yvec = _mm256_load_pd(y-7*4); MUL_ADD(xvec, yvec, cvec); _mm256_store_pd(x-7*4, xvec);
   case 6: xvec = _mm256_load_pd(x-6*4); yvec = _mm256_load_pd(y-6*4); MUL_ADD(xvec, yvec, cvec); _mm256_store_pd(x-6*4, xvec);
   case 5: xvec = _mm256_load_pd(x-5*4); yvec = _mm256_load_pd(y-5*4); MUL_ADD(xvec, yvec, cvec); _mm256_store_pd(x-5*4, xvec);
   case 4: xvec = _mm256_load_pd(x-4*4); yvec = _mm256_load_pd(y-4*4); MUL_ADD(xvec, yvec, cvec); _mm256_store_pd(x-4*4, xvec);
   case 3: xvec = _mm256_load_pd(x-3*4); yvec = _mm256_load_pd(y-3*4); MUL_ADD(xvec, yvec, cvec); _mm256_store_pd(x-3*4, xvec);
   case 2: xvec = _mm256_load_pd(x-2*4); yvec = _mm256_load_pd(y-2*4); MUL_ADD(xvec, yvec, cvec); _mm256_store_pd(x-2*4, xvec);
   case 1: xvec = _mm256_load_pd(x-1*4); yvec = _mm256_load_pd(y-1*4); MUL_ADD(xvec, yvec, cvec); _mm256_store_pd(x-1*4, xvec);
   }
   
}
#else
static inline
void muladd_interval2(double * NTL_RESTRICT x, double * NTL_RESTRICT y, double c, long n)
{
   for (long i = 0; i < n; i++)
      x[i] += y[i]*c;
}
#endif

#endif


#define DO_MUL(a, b) ((unsigned long) (long(a)*long(b)))
//#define DO_MUL(a, b) ((a)*(b))

static
inline void muladd_interval(unsigned long * NTL_RESTRICT x, unsigned long * NTL_RESTRICT y, 
                     unsigned long c, long n)
{
   for (long i = 0; i < n; i++)
      x[i] += DO_MUL(y[i], c);
}

static
void muladd1_by_32(unsigned long *x, const unsigned long *a, const unsigned long *b, 
                   long n)
{
   for (long j = 0; j < MAT_BLK_SZ; j++) {
      unsigned long sum = x[j];
      long i = 0;

      for (; i <= n-4; i += 4) {
         sum += DO_MUL(a[i+0], b[i+0]);
         sum += DO_MUL(a[i+1], b[i+1]);
         sum += DO_MUL(a[i+2], b[i+2]);
         sum += DO_MUL(a[i+3], b[i+3]);
      }

      for (; i < n; i++)
         sum += DO_MUL(a[i], b[i]);

      x[j] = sum;
      b += MAT_BLK_SZ;
   }
}

// experiment with shorter int's
static
void muladd1_by_32(unsigned long *x, const unsigned int *a, const unsigned int *b, 
                   long n)
{
   for (long j = 0; j < MAT_BLK_SZ; j++) {
      unsigned long sum = x[j];
      long i = 0;

      for (; i <= n-4; i += 4) {
         sum += DO_MUL(a[i+0], b[i+0]);
         sum += DO_MUL(a[i+1], b[i+1]);
         sum += DO_MUL(a[i+2], b[i+2]);
         sum += DO_MUL(a[i+3], b[i+3]);
      }

      for (; i < n; i++)
         sum += DO_MUL(a[i], b[i]);

      x[j] = sum;
      b += MAT_BLK_SZ;
   }
}

#if 0
static
void muladd1_by_32_full(unsigned long *x, const unsigned long *a, const unsigned long *b)
{
   for (long j = 0; j < MAT_BLK_SZ; j++) {
      unsigned long sum = x[j];
      long i = 0;

      sum += DO_MUL(a[i+0], b[i+0]);
      sum += DO_MUL(a[i+1], b[i+1]);
      sum += DO_MUL(a[i+2], b[i+2]);
      sum += DO_MUL(a[i+3], b[i+3]);
      sum += DO_MUL(a[i+4], b[i+4]);
      sum += DO_MUL(a[i+5], b[i+5]);
      sum += DO_MUL(a[i+6], b[i+6]);
      sum += DO_MUL(a[i+7], b[i+7]);
      sum += DO_MUL(a[i+8], b[i+8]);
      sum += DO_MUL(a[i+9], b[i+9]);
      sum += DO_MUL(a[i+10], b[i+10]);
      sum += DO_MUL(a[i+11], b[i+11]);
      sum += DO_MUL(a[i+12], b[i+12]);
      sum += DO_MUL(a[i+13], b[i+13]);
      sum += DO_MUL(a[i+14], b[i+14]);
      sum += DO_MUL(a[i+15], b[i+15]);
      sum += DO_MUL(a[i+16], b[i+16]);
      sum += DO_MUL(a[i+17], b[i+17]);
      sum += DO_MUL(a[i+18], b[i+18]);
      sum += DO_MUL(a[i+19], b[i+19]);
      sum += DO_MUL(a[i+20], b[i+20]);
      sum += DO_MUL(a[i+21], b[i+21]);
      sum += DO_MUL(a[i+22], b[i+22]);
      sum += DO_MUL(a[i+23], b[i+23]);
      sum += DO_MUL(a[i+24], b[i+24]);
      sum += DO_MUL(a[i+25], b[i+25]);
      sum += DO_MUL(a[i+26], b[i+26]);
      sum += DO_MUL(a[i+27], b[i+27]);
      sum += DO_MUL(a[i+28], b[i+28]);
      sum += DO_MUL(a[i+29], b[i+29]);
      sum += DO_MUL(a[i+30], b[i+30]);
      sum += DO_MUL(a[i+31], b[i+31]);

      x[j] = sum;
      b += MAT_BLK_SZ;
   }
}
#else

// this version is faster (by about 25%) on a Sandybridge machine

#define ONE_STEP_L(i) \
  sum += DO_MUL(a[i],b[i]);\
  sum_1 += DO_MUL(a[i],b_1[i]);\
  sum_2 += DO_MUL(a[i],b_2[i]);\
  sum_3 += DO_MUL(a[i],b_3[i])\


static
void muladd1_by_32_full(unsigned long *x, const unsigned long *a, const unsigned long *b)
{
   for (long j = 0; j < MAT_BLK_SZ; j+=4) {

      unsigned long sum = x[j];
      unsigned long sum_1 = x[j+1];
      unsigned long sum_2 = x[j+2];
      unsigned long sum_3 = x[j+3];

      const unsigned long *b_1 = b+MAT_BLK_SZ;
      const unsigned long *b_2 = b+2*MAT_BLK_SZ;
      const unsigned long *b_3 = b+3*MAT_BLK_SZ;

      ONE_STEP_L(0);
      ONE_STEP_L(1);
      ONE_STEP_L(2);
      ONE_STEP_L(3);
      ONE_STEP_L(4);
      ONE_STEP_L(5);
      ONE_STEP_L(6);
      ONE_STEP_L(7);
      ONE_STEP_L(8);
      ONE_STEP_L(9);
      ONE_STEP_L(10);
      ONE_STEP_L(11);
      ONE_STEP_L(12);
      ONE_STEP_L(13);
      ONE_STEP_L(14);
      ONE_STEP_L(15);
      ONE_STEP_L(16);
      ONE_STEP_L(17);
      ONE_STEP_L(18);
      ONE_STEP_L(19);
      ONE_STEP_L(20);
      ONE_STEP_L(21);
      ONE_STEP_L(22);
      ONE_STEP_L(23);
      ONE_STEP_L(24);
      ONE_STEP_L(25);
      ONE_STEP_L(26);
      ONE_STEP_L(27);
      ONE_STEP_L(28);
      ONE_STEP_L(29);
      ONE_STEP_L(30);
      ONE_STEP_L(31);

      x[j]   = sum;
      x[j+1] = sum_1;
      x[j+2] = sum_2; 
      x[j+3] = sum_3; 

      b += 4*MAT_BLK_SZ;
   }
}

// experiment with shorter int's
static
void muladd1_by_32_full(unsigned long *x, const unsigned int *a, const unsigned int *b)
{
   for (long j = 0; j < MAT_BLK_SZ; j+=4) {

      unsigned long sum = x[j];
      unsigned long sum_1 = x[j+1];
      unsigned long sum_2 = x[j+2];
      unsigned long sum_3 = x[j+3];

      const unsigned int *b_1 = b+MAT_BLK_SZ;
      const unsigned int *b_2 = b+2*MAT_BLK_SZ;
      const unsigned int *b_3 = b+3*MAT_BLK_SZ;

      ONE_STEP_L(0);
      ONE_STEP_L(1);
      ONE_STEP_L(2);
      ONE_STEP_L(3);
      ONE_STEP_L(4);
      ONE_STEP_L(5);
      ONE_STEP_L(6);
      ONE_STEP_L(7);
      ONE_STEP_L(8);
      ONE_STEP_L(9);
      ONE_STEP_L(10);
      ONE_STEP_L(11);
      ONE_STEP_L(12);
      ONE_STEP_L(13);
      ONE_STEP_L(14);
      ONE_STEP_L(15);
      ONE_STEP_L(16);
      ONE_STEP_L(17);
      ONE_STEP_L(18);
      ONE_STEP_L(19);
      ONE_STEP_L(20);
      ONE_STEP_L(21);
      ONE_STEP_L(22);
      ONE_STEP_L(23);
      ONE_STEP_L(24);
      ONE_STEP_L(25);
      ONE_STEP_L(26);
      ONE_STEP_L(27);
      ONE_STEP_L(28);
      ONE_STEP_L(29);
      ONE_STEP_L(30);
      ONE_STEP_L(31);

      x[j]   = sum;
      x[j+1] = sum_1;
      x[j+2] = sum_2; 
      x[j+3] = sum_3; 

      b += 4*MAT_BLK_SZ;
   }
}

#endif

static inline
void muladd_all_by_32(long first, long last, unsigned long *x, const unsigned int *a, const unsigned int *b, long n)
{
   if (n == MAT_BLK_SZ) {
      for (long i = first; i < last; i++)
         muladd1_by_32_full(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b);
   }
   else {
      for (long i = first; i < last; i++)
         muladd1_by_32(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b, n);
   }
}

static inline
void muladd_all_by_32(long first, long last, unsigned long *x, const unsigned long *a, const unsigned long *b, long n)
{
   if (n == MAT_BLK_SZ) {
      for (long i = first; i < last; i++)
         muladd1_by_32_full(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b);
   }
   else {
      for (long i = first; i < last; i++)
         muladd1_by_32(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b, n);
   }
}

#if (NTL_BITS_PER_INT >= NTL_BITS_PER_LONG/2)

typedef unsigned int uhlong;

#else

typedef unsigned long uhlong;

#endif



// NOTE: the following code sequence will generate imulq 
// instructions on x86_64 machines, which empirically is faster
// than using the mulq instruction or even the mulxq instruction,
// (tested on a Haswell machine).
#define NTL_CAST_ULL(x) ((NTL_ULL_TYPE) (x))
//#define NTL_CAST_ULL(x) ((NTL_ULL_TYPE)(unsigned long)(x))
#define NTL_MUL_ULL(x,y) (NTL_CAST_ULL(x)*NTL_CAST_ULL(y))


// NOTE: the following code is hardcoded for MAT_BLK_SZ == 32.
// Also, we special case NTL_BITS_PER_LONG-NTL_SP_NBITS > 2, which
// allows us to accumulate all 32 products without additional carries.

#if (NTL_BITS_PER_LONG-NTL_SP_NBITS > 2)

static
void muladd1_by_32(long *x, const long *a, const long *b, 
                   long n, long p, sp_ll_reduce_struct ll_red_struct)
{
   for (long j = 0; j < MAT_BLK_SZ; j++) {

      NTL_ULL_TYPE sum = cast_unsigned(x[j]);

#if 0
      for (long i = 0; i < n; i++)
         sum += NTL_MUL_ULL(a[i], b[i]);
#else
      long i=0;
      for(; i <= n-8; i+= 8) {
         sum += NTL_MUL_ULL(a[i+0], b[i+0]);
         sum += NTL_MUL_ULL(a[i+1], b[i+1]);
         sum += NTL_MUL_ULL(a[i+2], b[i+2]);
         sum += NTL_MUL_ULL(a[i+3], b[i+3]);

         sum += NTL_MUL_ULL(a[i+4], b[i+4]);
         sum += NTL_MUL_ULL(a[i+5], b[i+5]);
         sum += NTL_MUL_ULL(a[i+6], b[i+6]);
         sum += NTL_MUL_ULL(a[i+7], b[i+7]);
      }

      for (; i < n; i++)
         sum += NTL_MUL_ULL(a[i], b[i]);
      
#endif

      unsigned long sum0 = sum;
      unsigned long sum1 = sum >> NTL_BITS_PER_LONG;

      long res;
      
      if (ll_red_struct.nbits == NTL_SP_NBITS) 
         res = sp_ll_red_31_normalized(0, sum1, sum0, p, ll_red_struct);
      else
         res =  sp_ll_red_31(0, sum1, sum0, p, ll_red_struct);


      x[j] = res;
      b += MAT_BLK_SZ;
   }
}

#if 0
static
void muladd1_by_32_full(long *x, const long *a, const long *b, 
                        long p, sp_ll_reduce_struct ll_red_struct)
{
   for (long j = 0; j < MAT_BLK_SZ; j++) {

      NTL_ULL_TYPE sum = cast_unsigned(x[j]);

      sum += NTL_MUL_ULL(a[0], b[0]);
      sum += NTL_MUL_ULL(a[1], b[1]);
      sum += NTL_MUL_ULL(a[2], b[2]);
      sum += NTL_MUL_ULL(a[3], b[3]);
      sum += NTL_MUL_ULL(a[4], b[4]);
      sum += NTL_MUL_ULL(a[5], b[5]);
      sum += NTL_MUL_ULL(a[6], b[6]);
      sum += NTL_MUL_ULL(a[7], b[7]);
      sum += NTL_MUL_ULL(a[8], b[8]);
      sum += NTL_MUL_ULL(a[9], b[9]);
      sum += NTL_MUL_ULL(a[10], b[10]);
      sum += NTL_MUL_ULL(a[11], b[11]);
      sum += NTL_MUL_ULL(a[12], b[12]);
      sum += NTL_MUL_ULL(a[13], b[13]);
      sum += NTL_MUL_ULL(a[14], b[14]);
      sum += NTL_MUL_ULL(a[15], b[15]);
      sum += NTL_MUL_ULL(a[16], b[16]);
      sum += NTL_MUL_ULL(a[17], b[17]);
      sum += NTL_MUL_ULL(a[18], b[18]);
      sum += NTL_MUL_ULL(a[19], b[19]);
      sum += NTL_MUL_ULL(a[20], b[20]);
      sum += NTL_MUL_ULL(a[21], b[21]);
      sum += NTL_MUL_ULL(a[22], b[22]);
      sum += NTL_MUL_ULL(a[23], b[23]);
      sum += NTL_MUL_ULL(a[24], b[24]);
      sum += NTL_MUL_ULL(a[25], b[25]);
      sum += NTL_MUL_ULL(a[26], b[26]);
      sum += NTL_MUL_ULL(a[27], b[27]);
      sum += NTL_MUL_ULL(a[28], b[28]);
      sum += NTL_MUL_ULL(a[29], b[29]);
      sum += NTL_MUL_ULL(a[30], b[30]);
      sum += NTL_MUL_ULL(a[31], b[31]);

      unsigned long sum0 = sum;
      unsigned long sum1 = sum >> NTL_BITS_PER_LONG;

      long res;
      
      if (ll_red_struct.nbits == NTL_SP_NBITS) 
         res = sp_ll_red_31_normalized(0, sum1, sum0, p, ll_red_struct);
      else
         res =  sp_ll_red_31(0, sum1, sum0, p, ll_red_struct);


      x[j] = res;
      b += MAT_BLK_SZ;
   }
}

#elif 0

static
void muladd1_by_32_full(long *x, const long *a, const long *b, 
                        long p, sp_ll_reduce_struct ll_red_struct)
{
   for (long j = 0; j < MAT_BLK_SZ; j+=2) {

      NTL_ULL_TYPE sum = cast_unsigned(x[j]);
      NTL_ULL_TYPE sum_ = cast_unsigned(x[j+1]);
      const long *b_ = b+MAT_BLK_SZ;

      sum_ += NTL_MUL_ULL(a[0], b_[0]);      sum += NTL_MUL_ULL(a[0], b[0]);
      sum_ += NTL_MUL_ULL(a[1], b_[1]);      sum += NTL_MUL_ULL(a[1], b[1]);
      sum_ += NTL_MUL_ULL(a[2], b_[2]);      sum += NTL_MUL_ULL(a[2], b[2]);
      sum_ += NTL_MUL_ULL(a[3], b_[3]);      sum += NTL_MUL_ULL(a[3], b[3]);
      sum_ += NTL_MUL_ULL(a[4], b_[4]);      sum += NTL_MUL_ULL(a[4], b[4]);
      sum_ += NTL_MUL_ULL(a[5], b_[5]);      sum += NTL_MUL_ULL(a[5], b[5]);
      sum_ += NTL_MUL_ULL(a[6], b_[6]);      sum += NTL_MUL_ULL(a[6], b[6]);
      sum_ += NTL_MUL_ULL(a[7], b_[7]);      sum += NTL_MUL_ULL(a[7], b[7]);
      sum_ += NTL_MUL_ULL(a[8], b_[8]);      sum += NTL_MUL_ULL(a[8], b[8]);
      sum_ += NTL_MUL_ULL(a[9], b_[9]);      sum += NTL_MUL_ULL(a[9], b[9]);
      sum_ += NTL_MUL_ULL(a[10], b_[10]);      sum += NTL_MUL_ULL(a[10], b[10]);
      sum_ += NTL_MUL_ULL(a[11], b_[11]);      sum += NTL_MUL_ULL(a[11], b[11]);
      sum_ += NTL_MUL_ULL(a[12], b_[12]);      sum += NTL_MUL_ULL(a[12], b[12]);
      sum_ += NTL_MUL_ULL(a[13], b_[13]);      sum += NTL_MUL_ULL(a[13], b[13]);
      sum_ += NTL_MUL_ULL(a[14], b_[14]);      sum += NTL_MUL_ULL(a[14], b[14]);
      sum_ += NTL_MUL_ULL(a[15], b_[15]);      sum += NTL_MUL_ULL(a[15], b[15]);
      sum_ += NTL_MUL_ULL(a[16], b_[16]);      sum += NTL_MUL_ULL(a[16], b[16]);
      sum_ += NTL_MUL_ULL(a[17], b_[17]);      sum += NTL_MUL_ULL(a[17], b[17]);
      sum_ += NTL_MUL_ULL(a[18], b_[18]);      sum += NTL_MUL_ULL(a[18], b[18]);
      sum_ += NTL_MUL_ULL(a[19], b_[19]);      sum += NTL_MUL_ULL(a[19], b[19]);
      sum_ += NTL_MUL_ULL(a[20], b_[20]);      sum += NTL_MUL_ULL(a[20], b[20]);
      sum_ += NTL_MUL_ULL(a[21], b_[21]);      sum += NTL_MUL_ULL(a[21], b[21]);
      sum_ += NTL_MUL_ULL(a[22], b_[22]);      sum += NTL_MUL_ULL(a[22], b[22]);
      sum_ += NTL_MUL_ULL(a[23], b_[23]);      sum += NTL_MUL_ULL(a[23], b[23]);
      sum_ += NTL_MUL_ULL(a[24], b_[24]);      sum += NTL_MUL_ULL(a[24], b[24]);
      sum_ += NTL_MUL_ULL(a[25], b_[25]);      sum += NTL_MUL_ULL(a[25], b[25]);
      sum_ += NTL_MUL_ULL(a[26], b_[26]);      sum += NTL_MUL_ULL(a[26], b[26]);
      sum_ += NTL_MUL_ULL(a[27], b_[27]);      sum += NTL_MUL_ULL(a[27], b[27]);
      sum_ += NTL_MUL_ULL(a[28], b_[28]);      sum += NTL_MUL_ULL(a[28], b[28]);
      sum_ += NTL_MUL_ULL(a[29], b_[29]);      sum += NTL_MUL_ULL(a[29], b[29]);
      sum_ += NTL_MUL_ULL(a[30], b_[30]);      sum += NTL_MUL_ULL(a[30], b[30]);
      sum_ += NTL_MUL_ULL(a[31], b_[31]);      sum += NTL_MUL_ULL(a[31], b[31]);

      unsigned long sum0 = sum;
      unsigned long sum1 = sum >> NTL_BITS_PER_LONG;
      long res;

      unsigned long sum0_ = sum_;
      unsigned long sum1_ = sum_ >> NTL_BITS_PER_LONG;
      long res_;
      
      if (ll_red_struct.nbits == NTL_SP_NBITS) {
         res = sp_ll_red_31_normalized(0, sum1, sum0, p, ll_red_struct);
         res_ = sp_ll_red_31_normalized(0, sum1_, sum0_, p, ll_red_struct);
      }
      else {
         res =  sp_ll_red_31(0, sum1, sum0, p, ll_red_struct);
         res_ =  sp_ll_red_31(0, sum1_, sum0_, p, ll_red_struct);
      }


      x[j] = res;
      x[j+1] = res_;
      b += 2*MAT_BLK_SZ;
   }
}
#elif 1
// This version is consistently fastest on tests on Sandybridge and Haswell

#define ONE_STEP(i) \
  sum += NTL_MUL_ULL(a[i],b[i]);\
  sum_1 += NTL_MUL_ULL(a[i],b_1[i]);\
  sum_2 += NTL_MUL_ULL(a[i],b_2[i]);\
  sum_3 += NTL_MUL_ULL(a[i],b_3[i])\




static
void muladd1_by_32_full(long *x, const long *a, const long *b, 
                        long p, sp_ll_reduce_struct ll_red_struct)
{
   for (long j = 0; j < MAT_BLK_SZ; j+=4) {

      NTL_ULL_TYPE sum = cast_unsigned(x[j]);
      NTL_ULL_TYPE sum_1 = cast_unsigned(x[j+1]);
      NTL_ULL_TYPE sum_2 = cast_unsigned(x[j+2]);
      NTL_ULL_TYPE sum_3 = cast_unsigned(x[j+3]);
      const long *b_1 = b+MAT_BLK_SZ;
      const long *b_2 = b+2*MAT_BLK_SZ;
      const long *b_3 = b+3*MAT_BLK_SZ;

      ONE_STEP(0);
      ONE_STEP(1);
      ONE_STEP(2);
      ONE_STEP(3);
      ONE_STEP(4);
      ONE_STEP(5);
      ONE_STEP(6);
      ONE_STEP(7);
      ONE_STEP(8);
      ONE_STEP(9);
      ONE_STEP(10);
      ONE_STEP(11);
      ONE_STEP(12);
      ONE_STEP(13);
      ONE_STEP(14);
      ONE_STEP(15);
      ONE_STEP(16);
      ONE_STEP(17);
      ONE_STEP(18);
      ONE_STEP(19);
      ONE_STEP(20);
      ONE_STEP(21);
      ONE_STEP(22);
      ONE_STEP(23);
      ONE_STEP(24);
      ONE_STEP(25);
      ONE_STEP(26);
      ONE_STEP(27);
      ONE_STEP(28);
      ONE_STEP(29);
      ONE_STEP(30);
      ONE_STEP(31);

      unsigned long sum0 = sum;
      unsigned long sum1 = sum >> NTL_BITS_PER_LONG;

      unsigned long sum0_1 = sum_1;
      unsigned long sum1_1 = sum_1 >> NTL_BITS_PER_LONG;

      unsigned long sum0_2 = sum_2;
      unsigned long sum1_2 = sum_2 >> NTL_BITS_PER_LONG;

      unsigned long sum0_3 = sum_3;
      unsigned long sum1_3 = sum_3 >> NTL_BITS_PER_LONG;
      
      if (ll_red_struct.nbits == NTL_SP_NBITS) {
         x[j] = sp_ll_red_31_normalized(0, sum1, sum0, p, ll_red_struct);
         x[j+1] = sp_ll_red_31_normalized(0, sum1_1, sum0_1, p, ll_red_struct);
         x[j+2] = sp_ll_red_31_normalized(0, sum1_2, sum0_2, p, ll_red_struct);
         x[j+3] = sp_ll_red_31_normalized(0, sum1_3, sum0_3, p, ll_red_struct);
      }
      else {
         x[j] =  sp_ll_red_31(0, sum1, sum0, p, ll_red_struct);
         x[j+1] =  sp_ll_red_31(0, sum1_1, sum0_1, p, ll_red_struct);
         x[j+2] =  sp_ll_red_31(0, sum1_2, sum0_2, p, ll_red_struct);
         x[j+3] =  sp_ll_red_31(0, sum1_3, sum0_3, p, ll_red_struct);
      }


      b += 4*MAT_BLK_SZ;
   }
}
#else

static
void muladd1_by_32_full(long *x, const long *a, const long *b, 
                        long p, sp_ll_reduce_struct ll_red_struct)
{
   NTL_ULL_TYPE res_vec[MAT_BLK_SZ];

   for (long j = 0; j < MAT_BLK_SZ; j++) {

      NTL_ULL_TYPE sum = cast_unsigned(x[j]);

      sum += NTL_MUL_ULL(a[0], b[0]);
      sum += NTL_MUL_ULL(a[1], b[1]);
      sum += NTL_MUL_ULL(a[2], b[2]);
      sum += NTL_MUL_ULL(a[3], b[3]);
      sum += NTL_MUL_ULL(a[4], b[4]);
      sum += NTL_MUL_ULL(a[5], b[5]);
      sum += NTL_MUL_ULL(a[6], b[6]);
      sum += NTL_MUL_ULL(a[7], b[7]);
      sum += NTL_MUL_ULL(a[8], b[8]);
      sum += NTL_MUL_ULL(a[9], b[9]);
      sum += NTL_MUL_ULL(a[10], b[10]);
      sum += NTL_MUL_ULL(a[11], b[11]);
      sum += NTL_MUL_ULL(a[12], b[12]);
      sum += NTL_MUL_ULL(a[13], b[13]);
      sum += NTL_MUL_ULL(a[14], b[14]);
      sum += NTL_MUL_ULL(a[15], b[15]);
      sum += NTL_MUL_ULL(a[16], b[16]);
      sum += NTL_MUL_ULL(a[17], b[17]);
      sum += NTL_MUL_ULL(a[18], b[18]);
      sum += NTL_MUL_ULL(a[19], b[19]);
      sum += NTL_MUL_ULL(a[20], b[20]);
      sum += NTL_MUL_ULL(a[21], b[21]);
      sum += NTL_MUL_ULL(a[22], b[22]);
      sum += NTL_MUL_ULL(a[23], b[23]);
      sum += NTL_MUL_ULL(a[24], b[24]);
      sum += NTL_MUL_ULL(a[25], b[25]);
      sum += NTL_MUL_ULL(a[26], b[26]);
      sum += NTL_MUL_ULL(a[27], b[27]);
      sum += NTL_MUL_ULL(a[28], b[28]);
      sum += NTL_MUL_ULL(a[29], b[29]);
      sum += NTL_MUL_ULL(a[30], b[30]);
      sum += NTL_MUL_ULL(a[31], b[31]);

      res_vec[j] = sum;


      b += MAT_BLK_SZ;
   }

#if 0
   if (ll_red_struct.nbits == NTL_SP_NBITS) {
      for (long j = 0; j < MAT_BLK_SZ; j++)
         x[j] = sp_ll_red_31_normalized(0, res_vec[j] >> NTL_BITS_PER_LONG, res_vec[j], p, ll_red_struct);

   }
   else {
      for (long j = 0; j < MAT_BLK_SZ; j++)
         x[j] = sp_ll_red_31(0, res_vec[j] >> NTL_BITS_PER_LONG, res_vec[j], p, ll_red_struct);
   }
#else
#define ONE_STEP_redn(j) x[j] = sp_ll_red_31_normalized(0, res_vec[j] >> NTL_BITS_PER_LONG, res_vec[j], p, ll_red_struct)
#define ONE_STEP_red(j) x[j] = sp_ll_red_31(0, res_vec[j] >> NTL_BITS_PER_LONG, res_vec[j], p, ll_red_struct)
   if (ll_red_struct.nbits == NTL_SP_NBITS) {

      ONE_STEP_redn(0);
      ONE_STEP_redn(1);
      ONE_STEP_redn(2);
      ONE_STEP_redn(3);
      ONE_STEP_redn(4);
      ONE_STEP_redn(5);
      ONE_STEP_redn(6);
      ONE_STEP_redn(7);
      ONE_STEP_redn(8);
      ONE_STEP_redn(9);
      ONE_STEP_redn(10);
      ONE_STEP_redn(11);
      ONE_STEP_redn(12);
      ONE_STEP_redn(13);
      ONE_STEP_redn(14);
      ONE_STEP_redn(15);
      ONE_STEP_redn(16);
      ONE_STEP_redn(17);
      ONE_STEP_redn(18);
      ONE_STEP_redn(19);
      ONE_STEP_redn(20);
      ONE_STEP_redn(21);
      ONE_STEP_redn(22);
      ONE_STEP_redn(23);
      ONE_STEP_redn(24);
      ONE_STEP_redn(25);
      ONE_STEP_redn(26);
      ONE_STEP_redn(27);
      ONE_STEP_redn(28);
      ONE_STEP_redn(29);
      ONE_STEP_redn(30);
      ONE_STEP_redn(31);

   }
   else {
      ONE_STEP_red(0);
      ONE_STEP_red(1);
      ONE_STEP_red(2);
      ONE_STEP_red(3);
      ONE_STEP_red(4);
      ONE_STEP_red(5);
      ONE_STEP_red(6);
      ONE_STEP_red(7);
      ONE_STEP_red(8);
      ONE_STEP_red(9);
      ONE_STEP_red(10);
      ONE_STEP_red(11);
      ONE_STEP_red(12);
      ONE_STEP_red(13);
      ONE_STEP_red(14);
      ONE_STEP_red(15);
      ONE_STEP_red(16);
      ONE_STEP_red(17);
      ONE_STEP_red(18);
      ONE_STEP_red(19);
      ONE_STEP_red(20);
      ONE_STEP_red(21);
      ONE_STEP_red(22);
      ONE_STEP_red(23);
      ONE_STEP_red(24);
      ONE_STEP_red(25);
      ONE_STEP_red(26);
      ONE_STEP_red(27);
      ONE_STEP_red(28);
      ONE_STEP_red(29);
      ONE_STEP_red(30);
      ONE_STEP_red(31);
   }
#endif
}



#endif

#else

 
static
void muladd1_by_32(long *x, const long *a, const long *b, 
                   long n, long p, sp_ll_reduce_struct ll_red_struct)
{
   for (long j = 0; j < MAT_BLK_SZ; j++) {

      NTL_ULL_TYPE sum = cast_unsigned(x[j]);

      long i = 0;
      for (; i < n-16; i++)
         sum += NTL_MUL_ULL(a[i], b[i]);

      NTL_ULL_TYPE acc21 = (unsigned long) (sum >> NTL_BITS_PER_LONG);
      unsigned long acc0 = (unsigned long) sum;
      sum = 0;

      for (; i < n; i++)
         sum += NTL_MUL_ULL(a[i], b[i]);

      sum += acc0;
      acc0 = sum;
      acc21 += (unsigned long) (sum >> NTL_BITS_PER_LONG);

      long res;
      
      if (ll_red_struct.nbits == NTL_SP_NBITS) 
         res = sp_ll_red_31_normalized(acc21 >> NTL_BITS_PER_LONG, acc21, acc0, p, ll_red_struct);
      else
         res =  sp_ll_red_31(acc21 >> NTL_BITS_PER_LONG, acc21, acc0, p, ll_red_struct);

      x[j] = res;
      b += MAT_BLK_SZ;
   }
}

static
void muladd1_by_32_full(long *x, const long *a, const long *b, 
                        long p, sp_ll_reduce_struct ll_red_struct)
{
   for (long j = 0; j < MAT_BLK_SZ; j++) {

      NTL_ULL_TYPE sum = cast_unsigned(x[j]);

      sum += NTL_MUL_ULL(a[0], b[0]);
      sum += NTL_MUL_ULL(a[1], b[1]);
      sum += NTL_MUL_ULL(a[2], b[2]);
      sum += NTL_MUL_ULL(a[3], b[3]);
      sum += NTL_MUL_ULL(a[4], b[4]);
      sum += NTL_MUL_ULL(a[5], b[5]);
      sum += NTL_MUL_ULL(a[6], b[6]);
      sum += NTL_MUL_ULL(a[7], b[7]);
      sum += NTL_MUL_ULL(a[8], b[8]);
      sum += NTL_MUL_ULL(a[9], b[9]);
      sum += NTL_MUL_ULL(a[10], b[10]);
      sum += NTL_MUL_ULL(a[11], b[11]);
      sum += NTL_MUL_ULL(a[12], b[12]);
      sum += NTL_MUL_ULL(a[13], b[13]);
      sum += NTL_MUL_ULL(a[14], b[14]);
      sum += NTL_MUL_ULL(a[15], b[15]);

      NTL_ULL_TYPE acc21 = (unsigned long) (sum >> NTL_BITS_PER_LONG);
      unsigned long acc0 = (unsigned long) sum;
      sum = 0;

      sum += NTL_MUL_ULL(a[16], b[16]);
      sum += NTL_MUL_ULL(a[17], b[17]);
      sum += NTL_MUL_ULL(a[18], b[18]);
      sum += NTL_MUL_ULL(a[19], b[19]);
      sum += NTL_MUL_ULL(a[20], b[20]);
      sum += NTL_MUL_ULL(a[21], b[21]);
      sum += NTL_MUL_ULL(a[22], b[22]);
      sum += NTL_MUL_ULL(a[23], b[23]);
      sum += NTL_MUL_ULL(a[24], b[24]);
      sum += NTL_MUL_ULL(a[25], b[25]);
      sum += NTL_MUL_ULL(a[26], b[26]);
      sum += NTL_MUL_ULL(a[27], b[27]);
      sum += NTL_MUL_ULL(a[28], b[28]);
      sum += NTL_MUL_ULL(a[29], b[29]);
      sum += NTL_MUL_ULL(a[30], b[30]);
      sum += NTL_MUL_ULL(a[31], b[31]);

      sum += acc0;
      acc0 = sum;
      acc21 += (unsigned long) (sum >> NTL_BITS_PER_LONG);

      long res;
      
      if (ll_red_struct.nbits == NTL_SP_NBITS) 
         res = sp_ll_red_31_normalized(acc21 >> NTL_BITS_PER_LONG, acc21, acc0, p, ll_red_struct);
      else
         res =  sp_ll_red_31(acc21 >> NTL_BITS_PER_LONG, acc21, acc0, p, ll_red_struct);

      x[j] = res;
      b += MAT_BLK_SZ;
   }
}



#endif


static
void muladd1_by_32_half2(long *x, const long *a, const long *b, 
                        long n, long p, sp_ll_reduce_struct ll_red_struct)
{
   for (long j = 0; j < MAT_BLK_SZ; j++) {

      unsigned long sum[2];
      sum[0] = x[j];
      sum[1] = 0;

      long k=0;
      long i=0;
      for(; i <= n-16; i+= 16) {
         unsigned long lsum = a[i+0]*b[i+0];
         lsum += a[i+1]*b[i+1];
         lsum += a[i+2]*b[i+2];
         lsum += a[i+3]*b[i+3];
         lsum += a[i+4]*b[i+4];
         lsum += a[i+5]*b[i+5];
         lsum += a[i+6]*b[i+6];
         lsum += a[i+7]*b[i+7];
         lsum += a[i+8]*b[i+8];
         lsum += a[i+9]*b[i+9];
         lsum += a[i+10]*b[i+10];
         lsum += a[i+11]*b[i+11];
         lsum += a[i+12]*b[i+12];
         lsum += a[i+13]*b[i+13];
         lsum += a[i+14]*b[i+14];
         lsum += a[i+15]*b[i+15];
         sum[k++] += lsum;
      }

      if (i < n) {
         unsigned long lsum = a[i]*b[i];
	 for (i++; i < n; i++)
	    lsum += a[i]*b[i];
         sum[k++] += lsum;
      }

      
      long t0 = sp_ll_red_21(0, sum[0], p, ll_red_struct);
      long t1 = sp_ll_red_21(0, sum[1], p, ll_red_struct);
      x[j] = AddMod(t0, t1, p);

      b += MAT_BLK_SZ;
   }
}



// NOTE: oddly, this is slightly faster than the half2 routine, which
// I would have thought would be faster
// DIRT: this assumes MAT_BLK_SZ < (1L << NTL_BITS_PER_LONG/2),
// which will hold unconditionally for MAT_BLK_SZ < 2^16.
static
void muladd1_by_32_half1(long *x, const long *a, const long *b, 
                        long n, long p, sp_ll_reduce_struct ll_red_struct)
{
   for (long j = 0; j < MAT_BLK_SZ; j++) {

      NTL_ULL_TYPE sum = cast_unsigned(x[j]);

      long i=0;
      for(; i <= n-4; i+= 4) {
         unsigned long lsum = a[i+0]*b[i+0];
         lsum += a[i+1]*b[i+1];
         lsum += a[i+2]*b[i+2];
         lsum += a[i+3]*b[i+3];
         sum += lsum;
      }

      if (i < n) {
         unsigned long lsum = a[i]*b[i];
	 for (i++; i < n; i++)
	    lsum += a[i]*b[i];
         sum += lsum;
      }

      unsigned long sum0 = sum;
      unsigned long sum1 = sum >> NTL_BITS_PER_LONG;
      x[j] = sp_ll_red_21(sum1, sum0, p, ll_red_struct);

      b += MAT_BLK_SZ;
   }
}


static inline
void muladd_all_by_32(long first, long last, long *x, const long *a, const long *b, long n,
                      long p, sp_ll_reduce_struct ll_red_struct)
{
   if ((p-1) >= (1L << ((NTL_BITS_PER_LONG/2)-1))) {
      if (n == MAT_BLK_SZ) {
	 for (long i = first; i < last; i++)
	    muladd1_by_32_full(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b, p, ll_red_struct);
      }
      else {
	 for (long i = first; i < last; i++)
	    muladd1_by_32(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b, n, p, ll_red_struct);
      }
   }
   else {
      for (long i = first; i < last; i++)
	 muladd1_by_32_half1(x + i*MAT_BLK_SZ, a + i*MAT_BLK_SZ, b, n, p, ll_red_struct);
   }
}



#endif



static
inline void muladd_interval(long * NTL_RESTRICT x, long * NTL_RESTRICT y, 
                     long c, long n, long p, mulmod_t pinv)
{
   mulmod_precon_t cpinv = PrepMulModPrecon(c, p, pinv);
   for (long i = 0; i < n; i++) {
      long t = MulModPrecon(y[i], c, p, cpinv);
      x[i] = AddMod(x[i], t, p);
   }
}


// ******************************************************************
//
// General matrix multiplication code
//
// ******************************************************************





static
void basic_mul(const mat_window_zz_p& X, 
               const const_mat_window_zz_p& A, const const_mat_window_zz_p& B)  
{  
   long n = A.NumRows();  
   long l = A.NumCols();  
   long m = B.NumCols();  
  
   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();

   const bool seq = double(n)*double(l)*double(m) < PAR_THRESH;

   NTL_GEXEC_RANGE(seq, n, first, last) {

      for (long i = first; i < last; i++) {
         long j, k;
         const zz_p* ap = &A[i][0];
   
         zz_p *xp = &X[i][0];
         for (j = 0; j < m; j++) xp[j].LoopHole() = 0;
   
         for (k = 0;  k < l; k++) {   
            long aa = rep(ap[k]);
            if (aa != 0) {
               const zz_p* bp = &B[k][0];
               long T1;
               mulmod_precon_t aapinv = PrepMulModPrecon(aa, p, pinv);
   
               for (j = 0; j < m; j++) {
                  T1 = MulModPrecon(rep(bp[j]), aa, p, aapinv);
                  xp[j].LoopHole() = AddMod(rep(xp[j]), T1, p);
               } 
            }
         }
      }

   } NTL_GEXEC_RANGE_END
}  




#ifdef NTL_HAVE_LL_TYPE

static
void alt_mul_L(const mat_window_zz_p& X, 
               const const_mat_window_zz_p& A, const const_mat_window_zz_p& B)  
{  
   long n = A.NumRows();  
   long l = A.NumCols();  
   long m = B.NumCols();  
  
   long p = zz_p::modulus();
   sp_reduce_struct red_struct = zz_p::red_struct();

   const bool seq = double(n)*double(l)*double(m) < PAR_THRESH;

   NTL_GEXEC_RANGE(seq, m, first, last) {

      Vec<long> B_col;
      B_col.SetLength(l);
      long *bp = B_col.elts();

      long i, j, k;

      for (j = first; j < last; j++) {
         for (k = 0; k < l; k++) bp[k] = rep(B[k][j]);
   
         for (i = 0; i < n; i++) {
            const zz_p *ap = &A[i][0];
            X[i][j].LoopHole() = InnerProd_L(bp, ap, l, p, red_struct);
         }
      }

   } NTL_GEXEC_RANGE_END
}  


static
void alt_mul_LL(const mat_window_zz_p& X, 
                const const_mat_window_zz_p& A, const const_mat_window_zz_p& B)  
{  
   long n = A.NumRows();  
   long l = A.NumCols();  
   long m = B.NumCols();  
  
   long p = zz_p::modulus();
   sp_ll_reduce_struct ll_red_struct = zz_p::ll_red_struct();

   const bool seq = double(n)*double(l)*double(m) < PAR_THRESH;

   NTL_GEXEC_RANGE(seq, m, first, last) {

      Vec<long> B_col;
      B_col.SetLength(l);
      long *bp = B_col.elts();

      long i, j, k;

      for (j = first; j < last; j++) {
         for (k = 0; k < l; k++) bp[k] = rep(B[k][j]);
   
         for (i = 0; i < n; i++) {
            const zz_p *ap = &A[i][0];
            X[i][j].LoopHole() = InnerProd_LL(bp, ap, l, p, ll_red_struct);
         }
      }

   } NTL_GEXEC_RANGE_END
}  


#ifdef NTL_HAVE_AVX

static
void blk_mul_DD(const mat_window_zz_p& X, 
                const const_mat_window_zz_p& A, const const_mat_window_zz_p& B)  
{  
   long n = A.NumRows();  
   long l = A.NumCols();  
   long m = B.NumCols();  

   long p = zz_p::modulus();
   sp_reduce_struct red_struct = zz_p::red_struct();

   UniqueArray< AlignedArray<double> > A_buf;
   long npanels = (l+MAT_BLK_SZ-1)/MAT_BLK_SZ;
   A_buf.SetLength(npanels);

   for (long kk = 0, panel = 0; kk < l; kk += MAT_BLK_SZ, panel++) {
      long k_max = min(kk+MAT_BLK_SZ, l);

      A_buf[panel].SetLength(n * MAT_BLK_SZ);
      double *abp = &A_buf[panel][0];

      for (long i = 0; i < n; i++, abp += MAT_BLK_SZ) {
         const zz_p *ap1 = &A[i][0];
         for (long k = kk; k < k_max; k++) {
            abp[k-kk] = rep(ap1[k]);
         }
         for (long k = k_max; k < kk+MAT_BLK_SZ; k++) {
            abp[k-kk] = 0;
         }
      }
   }

   long nxpanels = (m+MAT_BLK_SZ-1)/MAT_BLK_SZ;

   const bool seq = double(n)*double(l)*double(m) < PAR_THRESH;

   NTL_GEXEC_RANGE(seq, nxpanels, first, last) 
   NTL_IMPORT(n)
   NTL_IMPORT(l)
   NTL_IMPORT(m)
   NTL_IMPORT(p)
   NTL_IMPORT(red_struct)

   AlignedArray<double> B_rec;
   B_rec.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
   double *brec = B_rec.get();

   AlignedArray<double> X_buf;
   X_buf.SetLength(n*MAT_BLK_SZ);
   double *xbp = X_buf.get();

   long jj, kk;
   long i, j, k;
   long panel;
   long xpanel;

   for (xpanel = first, jj = first*MAT_BLK_SZ; xpanel < last; 
        xpanel++, jj += MAT_BLK_SZ) {

      long j_max = min(jj+MAT_BLK_SZ, m);

      for (i = 0; i < n*MAT_BLK_SZ; i++) xbp[i] = 0;

      long red_trigger = (MAX_DBL_INT-(p-1))/((p-1)*(p-1));
      long red_count = red_trigger;

      for (kk = 0, panel = 0; kk < l; kk += MAT_BLK_SZ, panel++) {
         long k_max = min(kk+MAT_BLK_SZ, l);

         for (k = kk; k < k_max; k++) {
            const zz_p *bp = &B[k][0];
            for (j = jj; j < j_max; j++) 
               brec[(k-kk)*MAT_BLK_SZ+(j-jj)] = rep(bp[j]);
            for (j = j_max; j < jj+MAT_BLK_SZ; j++) 
               brec[(k-kk)*MAT_BLK_SZ+(j-jj)] = 0;
         }


         if (red_count-MAT_BLK_SZ < 0) {
            red_count = red_trigger;
            for (i = 0; i < n*MAT_BLK_SZ; i++) 
               xbp[i] = rem((unsigned long)(long)xbp[i], p, red_struct);
         }

         red_count = red_count-MAT_BLK_SZ;

         const double *abp = &A_buf[panel][0];

         muladd_all_by_32(0, n, xbp, abp, brec, k_max-kk);
      }

      
      for (i = 0; i < n; i++) {
         zz_p *xp = &X[i][0];
         for (j = jj; j < j_max; j++)
            xp[j].LoopHole() = 
              rem((unsigned long)(long)xbp[i*MAT_BLK_SZ + (j-jj)], p, red_struct);
      }
   }

   NTL_GEXEC_RANGE_END
}  

#endif


static
void blk_mul_LL(const mat_window_zz_p& X, 
                const const_mat_window_zz_p& A, const const_mat_window_zz_p& B)  
{  
   long n = A.NumRows();  
   long l = A.NumCols();  
   long m = B.NumCols();  

   long p = zz_p::modulus();
   sp_ll_reduce_struct ll_red_struct = zz_p::ll_red_struct();

   Vec< Vec<long> > A_buf;
   Vec<long *> abufp;
   long npanels = (l+MAT_BLK_SZ-1)/MAT_BLK_SZ;
   A_buf.SetLength(npanels);
   abufp.SetLength(npanels);

   for (long kk = 0, panel = 0; kk < l; kk += MAT_BLK_SZ, panel++) {
      long k_max = min(kk+MAT_BLK_SZ, l);

      A_buf[panel].SetLength(n * MAT_BLK_SZ);
      long *abp = A_buf[panel].elts();
      abufp[panel] = abp;

      for (long i = 0; i < n; i++, abp += MAT_BLK_SZ) {
         const zz_p *ap1 = &A[i][0];
         for (long k = kk; k < k_max; k++) {
            abp[k-kk] = rep(ap1[k]);
         }
         for (long k = k_max; k < kk+MAT_BLK_SZ; k++) {
            abp[k-kk] = 0;
         }
      }
   }

   long nxpanels = (m+MAT_BLK_SZ-1)/MAT_BLK_SZ;

   const bool seq = double(n)*double(l)*double(m) < PAR_THRESH;

   NTL_GEXEC_RANGE(seq, nxpanels, first, last) 
   NTL_IMPORT(n)
   NTL_IMPORT(l)
   NTL_IMPORT(m)
   NTL_IMPORT(p)
   NTL_IMPORT(ll_red_struct)

   UniqueArray<long> B_rec;
   B_rec.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
   long *brec = B_rec.get();

   UniqueArray<long> X_buf;
   X_buf.SetLength(n*MAT_BLK_SZ);
   long *xbp = X_buf.get();

   long jj, kk;
   long i, j, k;
   long panel;
   long xpanel;

   for (xpanel = first, jj = first*MAT_BLK_SZ; xpanel < last; 
        xpanel++, jj += MAT_BLK_SZ) {

      long j_max = min(jj+MAT_BLK_SZ, m);

      for (i = 0; i < n*MAT_BLK_SZ; i++) xbp[i] = 0;

      for (kk = 0, panel = 0; kk < l; kk += MAT_BLK_SZ, panel++) {
         long k_max = min(kk+MAT_BLK_SZ, l);

         // fill brec, transposed

         for (k = kk; k < k_max; k++) {
            const zz_p *bp = &B[k][0];
            for (j = jj; j < j_max; j++) 
               brec[(k-kk)+(j-jj)*MAT_BLK_SZ] = rep(bp[j]);
            for (j = j_max; j < jj+MAT_BLK_SZ; j++) 
               brec[(k-kk)+(j-jj)*MAT_BLK_SZ] = 0;
         }

         const long *abp = abufp[panel];
         muladd_all_by_32(0, n, xbp, abp, brec, k_max-kk, p, ll_red_struct);
      }

      
      for (i = 0; i < n; i++) {
         zz_p *xp = &X[i][0];
         for (j = jj; j < j_max; j++)
            xp[j].LoopHole() =  xbp[i*MAT_BLK_SZ + (j-jj)];
      }
   }

   NTL_GEXEC_RANGE_END
}  


static
void blk_mul_L(const mat_window_zz_p& X, 
               const const_mat_window_zz_p& A, const const_mat_window_zz_p& B)  
{  
   long n = A.NumRows();  
   long l = A.NumCols();  
   long m = B.NumCols();  

   long p = zz_p::modulus();
   sp_reduce_struct red_struct = zz_p::red_struct();

   Vec< Vec<uhlong> > A_buf;
   Vec<uhlong*> abufp;
   long npanels = (l+MAT_BLK_SZ-1)/MAT_BLK_SZ;
   A_buf.SetLength(npanels);
   abufp.SetLength(npanels);

   for (long kk = 0, panel = 0; kk < l; kk += MAT_BLK_SZ, panel++) {
      long k_max = min(kk+MAT_BLK_SZ, l);

      A_buf[panel].SetLength(n * MAT_BLK_SZ);
      uhlong *abp = A_buf[panel].elts();
      abufp[panel] = abp;

      for (long i = 0; i < n; i++, abp += MAT_BLK_SZ) {
         const zz_p *ap1 = &A[i][0];
         for (long k = kk; k < k_max; k++) {
            abp[k-kk] = rep(ap1[k]);
         }
         for (long k = k_max; k < kk+MAT_BLK_SZ; k++) {
            abp[k-kk] = 0;
         }
      }
   }

   long nxpanels = (m+MAT_BLK_SZ-1)/MAT_BLK_SZ;

   const bool seq = double(n)*double(l)*double(m) < PAR_THRESH;

   NTL_GEXEC_RANGE(seq, nxpanels, first, last) 
   NTL_IMPORT(n)
   NTL_IMPORT(l)
   NTL_IMPORT(m)
   NTL_IMPORT(p)
   NTL_IMPORT(red_struct)

   UniqueArray<uhlong> B_rec;
   B_rec.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
   uhlong *brec = B_rec.get();

   UniqueArray<unsigned long> X_buf;
   X_buf.SetLength(n*MAT_BLK_SZ);
   unsigned long *xbp = X_buf.get();

   long jj, kk;
   long i, j, k;
   long panel;
   long xpanel;

   for (xpanel = first, jj = first*MAT_BLK_SZ; xpanel < last; 
        xpanel++, jj += MAT_BLK_SZ) {

      long j_max = min(jj+MAT_BLK_SZ, m);

      for (i = 0; i < n*MAT_BLK_SZ; i++) xbp[i] = 0;

      unsigned long ured_trigger = 
         (~(0UL)-cast_unsigned(p-1))/(cast_unsigned(p-1)*cast_unsigned(p-1));
      // NOTE: corner case at p == 2: need unsigned long to prevent overflow

      long red_trigger = min(cast_unsigned(NTL_MAX_LONG), ured_trigger);

      long red_count = red_trigger;

      for (kk = 0, panel = 0; kk < l; kk += MAT_BLK_SZ, panel++) {
         long k_max = min(kk+MAT_BLK_SZ, l);

         // fill brec, transposed

         for (k = kk; k < k_max; k++) {
            const zz_p *bp = &B[k][0];
            for (j = jj; j < j_max; j++) 
               brec[(k-kk)+(j-jj)*MAT_BLK_SZ] = rep(bp[j]);
            for (j = j_max; j < jj+MAT_BLK_SZ; j++) 
               brec[(k-kk)+(j-jj)*MAT_BLK_SZ] = 0;
         }

         if (red_count-MAT_BLK_SZ < 0) {
            red_count = red_trigger;
            for (i = 0; i < n*MAT_BLK_SZ; i++) 
               xbp[i] = rem(xbp[i], p, red_struct);
         }

         red_count = red_count-MAT_BLK_SZ;

         const uhlong *abp = abufp[panel];

         muladd_all_by_32(0, n, xbp, abp, brec, k_max-kk);
      }

      
      for (i = 0; i < n; i++) {
         zz_p *xp = &X[i][0];
         for (j = jj; j < j_max; j++)
            xp[j].LoopHole() = 
              rem(xbp[i*MAT_BLK_SZ + (j-jj)], p, red_struct);
      }
   }

   NTL_GEXEC_RANGE_END
}  


#endif




static
void mul_base (const mat_window_zz_p& X, 
               const const_mat_window_zz_p& A, const const_mat_window_zz_p& B)  
{
   long n = A.NumRows();  
   long l = A.NumCols();  
   long m = B.NumCols();  

   if (n == 0 || l == 0 || m == 0) {
      clear(X);
      return;
   }


#ifndef NTL_HAVE_LL_TYPE

   basic_mul(X, A, B);

#else

   if (l < 32) {
      //cerr << "basic_mul\n";
      basic_mul(X, A, B);
      return;
   }

   long p = zz_p::modulus();
   
   if (n/MAT_BLK_SZ < 4 || l/MAT_BLK_SZ < 4 || m/MAT_BLK_SZ < 4) {
      if (cast_unsigned(l) <= (~(0UL))/cast_unsigned(p-1) &&
          cast_unsigned(l)*cast_unsigned(p-1) <= (~(0UL))/cast_unsigned(p-1)) {
         //cerr << "alt_mul_L\n";
         alt_mul_L(X, A, B);
      }
      else {
         //cerr << "alt_mul_LL\n";
         alt_mul_LL(X, A, B);
      }

      return;
   }

   {
      if (NTL_OVERFLOW(n, MAT_BLK_SZ, 0)) ResourceError("number too big");
      if (NTL_OVERFLOW(l, MAT_BLK_SZ, 0)) ResourceError("number too big");
      if (NTL_OVERFLOW(m, MAT_BLK_SZ, 0)) ResourceError("number too big");

      long V = MAT_BLK_SZ*4;

#ifdef NTL_HAVE_AVX
      if (p-1 <= MAX_DBL_INT &&
          V <= (MAX_DBL_INT-(p-1))/(p-1) &&
          V*(p-1) <= (MAX_DBL_INT-(p-1))/(p-1)) {

         // cerr << "block_mul_DD\n";
         blk_mul_DD(X, A, B);
      }
      else 
#endif
           if (cast_unsigned(V) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1) &&
               cast_unsigned(V)*cast_unsigned(p-1) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1))  {

         //cerr << "blk_mul_L\n";
         blk_mul_L(X, A, B);

      }
      else {
  
         //cerr << "blk_mul_LL\n";
         blk_mul_LL(X, A, B);
      }
   }

#endif


}

// The following implementation of Strassen is derived directly 
// from the implementation in FLINT v2.5.2 (see http://www.flintlib.org),
// although a number of details have changed.
// I include the original copyright notice from the file nmod_mat/mul_strassen.c
// in the FLINT distribution.

/*=============================================================================

    This file is part of FLINT.

    FLINT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    FLINT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FLINT; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

=============================================================================*/
/******************************************************************************

    Copyright (C) 2008, Martin Albrecht
    Copyright (C) 2008, 2009 William Hart.
    Copyright (C) 2010, Fredrik Johansson

******************************************************************************/




void mul_strassen(const mat_window_zz_p& C, 
                  const const_mat_window_zz_p& A, const const_mat_window_zz_p& B)  
{
    long a, b, c;
    long anr, anc, bnr, bnc;


    a = A.NumRows();
    b = A.NumCols();
    c = B.NumCols();


    bool use_DD = false;
    // this code determines if mul_base triggers blk_mul_DD,
    // in which case a higher crossover is used

#if (defined(NTL_HAVE_LL_TYPE) && defined(NTL_HAVE_AVX))
    {
       long V = MAT_BLK_SZ*4;
       long p = zz_p::modulus();

       if (p-1 <= MAX_DBL_INT &&
           V <= (MAX_DBL_INT-(p-1))/(p-1) &&
           V*(p-1) <= (MAX_DBL_INT-(p-1))/(p-1)) 
       {
          use_DD = true;
       }
    }
#endif

    long nt = AvailableThreads();

    long xover;
    // now we set the crossover -- it is kind of a heauristic
    // mess based on nt and use_DD...I've run some tests to 
    // make sure these settings are reasonable, but a more
    // rational approach would be preferable

    if (nt > 1) {
       if (use_DD || nt > 8192/(2*MAT_BLK_SZ)) 
          xover = 8192;
       else
          xover = max(800, nt*2*MAT_BLK_SZ);
    }
    else {
       if (use_DD) 
          xover = 800;
       else
          xover = 448;
    }

    if (a <= xover  || b <= xover || c <= xover)
    {
        mul_base(C, A, B);
        return;
    }

    anr = a / 2;
    anc = b / 2;
    bnr = anc;
    bnc = c / 2;

    const_mat_window_zz_p A11(A, 0, 0, anr, anc);
    const_mat_window_zz_p A12(A, 0, anc, anr, 2*anc);
    const_mat_window_zz_p A21(A, anr, 0, 2*anr, anc);
    const_mat_window_zz_p A22(A, anr, anc, 2*anr, 2*anc);

    const_mat_window_zz_p B11(B, 0, 0, bnr, bnc);
    const_mat_window_zz_p B12(B, 0, bnc, bnr, 2*bnc);
    const_mat_window_zz_p B21(B, bnr, 0, 2*bnr, bnc);
    const_mat_window_zz_p B22(B, bnr, bnc, 2*bnr, 2*bnc);

    mat_window_zz_p C11(C, 0, 0, anr, bnc);
    mat_window_zz_p C12(C, 0, bnc, anr, 2*bnc);
    mat_window_zz_p C21(C, anr, 0, 2*anr, bnc);
    mat_window_zz_p C22(C, anr, bnc, 2*anr, 2*bnc);

    mat_zz_p X1_store;
    X1_store.SetDims(anr, max(bnc, anc));

    mat_window_zz_p X1a(X1_store, 0, 0, anr, anc);
    mat_window_zz_p X1b(X1_store, 0, 0, anr, bnc);
 
    mat_zz_p X2;
    X2.SetDims(anc, bnc);

    /*
        See Jean-Guillaume Dumas, Clement Pernet, Wei Zhou; "Memory
        efficient scheduling of Strassen-Winograd's matrix multiplication
        algorithm"; http://arxiv.org/pdf/0707.2347v3 for reference on the
        used operation scheduling.
    */

    sub(X1a, A11, A21);
    sub(X2, B22, B12);
    mul_strassen(C21, X1a, X2);

    add(X1a, A21, A22);
    sub(X2, B12, B11);
    mul_strassen(C22, X1a, X2);

    sub(X1a, X1a, A11);
    sub(X2, B22, X2);
    mul_strassen(C12, X1a, X2);

    sub(X1a, A12, X1a);
    mul_strassen(C11, X1a, B22);


    mul_strassen(X1b, A11, B11);

    add(C12, X1b, C12);
    add(C21, C12, C21);
    add(C12, C12, C22);
    add(C22, C21, C22);
    add(C12, C12, C11);
    sub(X2, X2, B21);
    mul_strassen(C11, A22, X2);

    X2.kill();

    sub(C21, C21, C11);
    mul_strassen(C11, A12, B21);

    add(C11, X1b, C11);

    X1_store.kill();

    if (c > 2*bnc) /* A by last col of B -> last col of C */
    {
        const_mat_window_zz_p Bc(B, 0, 2*bnc, b, c);
        mat_window_zz_p Cc(C, 0, 2*bnc, a, c);

        mul_strassen(Cc, A, Bc);
    }

    if (a > 2*anr) /* last row of A by B -> last row of C */
    {
        const_mat_window_zz_p Ar(A, 2*anr, 0, a, b);
        mat_window_zz_p Cr(C, 2*anr, 0, a, c);
        mul_strassen(Cr, Ar, B);
    }

    if (b > 2*anc) /* last col of A by last row of B -> C */
    {
        const_mat_window_zz_p Ac(A, 0, 2*anc, 2*anr, b);
        const_mat_window_zz_p Br(B, 2*bnr, 0, b, 2*bnc);
        mat_window_zz_p Cb(C, 0, 0, 2*anr, 2*bnc);

        // Cb += Ac*Br
        mat_zz_p tmp;
        tmp.SetDims(Cb.NumRows(), Cb.NumCols());
        mul_strassen(tmp, Ac, Br);
        add(Cb, Cb, tmp);
    }
}







static
void mul_aux(mat_zz_p& X, const mat_zz_p& A, const mat_zz_p& B)
{
   long n = A.NumRows();  
   long l = A.NumCols();  
   long m = B.NumCols();  
  
   if (l != B.NumRows())  
      LogicError("matrix mul: dimension mismatch");  

   X.SetDims(n, m); 

   if (n == 0 || l == 0 || m == 0) {
      clear(X);
      return;
   }

   mul_strassen(X, A, B);
}


void mul(mat_zz_p& X, const mat_zz_p& A, const mat_zz_p& B)
{
   if (&X == &A || &X == &B) {  
      mat_zz_p tmp;  
      mul_aux(tmp, A, B);  
      X = tmp;  
   }  
   else  
      mul_aux(X, A, B);  
}


// ******************************************************************
//
// Matrix inversion code
//
// ******************************************************************

static
long relaxed_InvModStatus(long& x, long a, long n, bool relax)
{
   if (relax) {
      return InvModStatus(x, a, n);
   }
   else {
      x = InvMod(a, n);
      return 0;
   }
}

static
void basic_inv(zz_p& d, mat_zz_p& X, const mat_zz_p& A, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("inv: nonsquare matrix");

   if (n == 0) {
      set(d);
      X.SetDims(0, 0);
      return;
   }


   Mat<long> M;
   conv(M, A);
   // scratch space

   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations

   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();

   bool seq = n < PAR_THRESH_SQ;

   bool pivoting = false;

   for (long k = 0; k < n; k++) {
      long pos = -1;
      long pivot_inv;
      for (long i = k; i < n; i++) {
         // NOTE: by using InvModStatus, this code will work
         // for prime-powers as well as primes
         long pivot = M[i][k];
         if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
            pos = i;
            break;
         }
      }

      if (pos != -1) {
         if (k != pos) {
            swap(M[pos], M[k]);
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;
         }

         det = MulMod(det, M[k][k], p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 
            long * NTL_RESTRICT y = &M[k][0];
            for (long j = 0; j < n; j++) 
               y[j] = MulModPrecon(y[j], t1, p, t1pinv);

            y[k] = pivot_inv;
         }



         NTL_GEXEC_RANGE(seq, n, first, last)  
         NTL_IMPORT(p)
         NTL_IMPORT(n)
         NTL_IMPORT(k)
         long * NTL_RESTRICT y = &M[k][0]; 
         for (long i = first; i < last; i++) {
            if (i == k) continue; // skip row k

            long * NTL_RESTRICT x = &M[i][0]; 
            long t1 = x[k];
            t1 = NegateMod(t1, p);
            x[k] = 0;
            if (t1 == 0) continue;

            // add t1 * row k to row i
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 

            for (long j = 0; j < n; j++) {
               long t2 = MulModPrecon(y[j], t1, p, t1pinv);
               x[j] = AddMod(x[j], t2, p);
            }
         }
         NTL_GEXEC_RANGE_END
      }
      else {
         clear(d);
         return;
      }
   }

   if (pivoting) {
      // pivot colums, using reverse swap sequence

      for (long i = 0; i < n; i++) {
         long * NTL_RESTRICT x = &M[i][0]; 

         for (long k = n-1; k >= 0; k--) {
            long pos = P[k];
            if (pos != k) _ntl_swap(x[pos], x[k]);
         }
      }
   }
   
   X.SetDims(n, n);
   for (long i = 0; i < n; i++)
      for (long j = 0; j < n; j++)
         X[i][j].LoopHole() = M[i][j];

   d.LoopHole() = det;
}



#ifdef NTL_HAVE_LL_TYPE



static
void alt_inv_L(zz_p& d, mat_zz_p& X, const mat_zz_p& A, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("inv: nonsquare matrix");

   if (n == 0) {
      set(d);
      X.SetDims(0, 0);
      return;
   }


   Mat<unsigned long> M;
   conv(M, A);
   // scractch space

   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations

   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_reduce_struct red_struct = zz_p::red_struct();

   

   bool seq = n < PAR_THRESH_SQ;

   bool pivoting = false;

   unsigned long ured_trigger = 
      (~(0UL)-cast_unsigned(p-1))/(cast_unsigned(p-1)*cast_unsigned(p-1));
   // NOTE: corner case at p == 2: need unsigned long to prevent overflow

   long red_trigger = min(cast_unsigned(NTL_MAX_LONG), ured_trigger);

   long red_count = red_trigger;


   for (long k = 0; k < n; k++) {
      bool cleanup = false;

      if (red_count-1 < 0) {
         red_count = red_trigger;
         cleanup = true;
      }

      red_count = red_count-1;

      long pos = -1;
      long pivot;
      long pivot_inv;

      for (long i = k; i < n; i++) {
         // NOTE: by using InvModStatus, this code will work
         // for prime-powers as well as primes
         pivot = rem(M[i][k], p, red_struct);
         if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
            pos = i;
            break;
         }
      }

      if (pos != -1) {
         if (k != pos) {
            swap(M[pos], M[k]);
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;
         }

         det = MulMod(det, pivot, p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); // t1*pinv;
            unsigned long * NTL_RESTRICT y = &M[k][0];
            for (long j = 0; j < n; j++) {
               long t2 = rem(y[j], p, red_struct);
               y[j] = MulModPrecon(t2, t1, p, t1pinv);
            }

            y[k] = pivot_inv;
         }


         NTL_GEXEC_RANGE(seq, n, first, last)  
         NTL_IMPORT(p)
         NTL_IMPORT(n)
         NTL_IMPORT(k)
         NTL_IMPORT(red_struct)
         unsigned long * NTL_RESTRICT y = &M[k][0]; 
         if (cleanup) {
            for (long i = first; i < last; i++) {
               if (i == k) continue;
               // skip row k: the data won't change, but it
               // technically is a race condition in a multi-theaded
               // execution, and it would violate the "restrict"
               // contract

               unsigned long * NTL_RESTRICT x = &M[i][0]; 
               for (long j = 0; j < n; j++) {
                  x[j] = rem(x[j], p, red_struct);
               }
            }
         }


         for (long i = first; i < last; i++) {
            if (i == k) continue; // skip row k

            unsigned long * NTL_RESTRICT x = &M[i][0]; 
            long t1 = rem(x[k], p, red_struct);
            t1 = NegateMod(t1, p);
            x[k] = 0;
            if (t1 == 0) continue;

            // add t1 * row k to row i
            unsigned long ut1 = t1;
            long j;
            for (j = 0; j <= n-4; j+=4) {
               unsigned long xj0 = x[j+0] + DO_MUL(y[j+0], ut1);
               unsigned long xj1 = x[j+1] + DO_MUL(y[j+1], ut1);
               unsigned long xj2 = x[j+2] + DO_MUL(y[j+2], ut1);
               unsigned long xj3 = x[j+3] + DO_MUL(y[j+3], ut1);
               x[j+0] = xj0;
               x[j+1] = xj1;
               x[j+2] = xj2;
               x[j+3] = xj3;
            }
            for (; j < n; j++) {
               x[j] += DO_MUL(y[j], ut1);
            }
         }
         NTL_GEXEC_RANGE_END
      }
      else {
         clear(d);
         return;
      }
   }

   if (pivoting) {
      // pivot colums, using reverse swap sequence

      for (long i = 0; i < n; i++) {
         unsigned long * NTL_RESTRICT x = &M[i][0]; 

         for (long k = n-1; k >= 0; k--) {
            long pos = P[k];
            if (pos != k) _ntl_swap(x[pos], x[k]);
         }
      }
   }

   X.SetDims(n, n);
   for (long i = 0; i < n; i++)
      for (long j = 0; j < n; j++)
         X[i][j].LoopHole() = rem(M[i][j], p, red_struct);

   d.LoopHole() = det;
}





#ifdef NTL_HAVE_AVX

static
void alt_inv_DD(zz_p& d, mat_zz_p& X, const mat_zz_p& A, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("inv: nonsquare matrix");

   if (n == 0) {
      set(d);
      X.SetDims(0, 0);
      return;
   }

   Vec< AlignedArray<double> > M;
   M.SetLength(n);
   for (long i = 0; i < n; i++) M[i].SetLength(n);

   for (long i = 0; i < n; i++) {
      for (long j = 0; j < n; j++) 
         M[i][j] = rep(A[i][j]);
   }


   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations

   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_reduce_struct red_struct = zz_p::red_struct();

   

   bool seq = n < PAR_THRESH_SQ;

   bool pivoting = false;

   long red_trigger = (MAX_DBL_INT-(p-1))/((p-1)*(p-1));
   long red_count = red_trigger;

   for (long k = 0; k < n; k++) {
      bool cleanup = false;

      if (red_count-1 < 0) {
         red_count = red_trigger;
         cleanup = true;
      }

      red_count = red_count-1;

      long pos = -1;
      long pivot;
      long pivot_inv;



      for (long i = k; i < n; i++) {
         // NOTE: by using InvModStatus, this code will work
         // for prime-powers as well as primes
         pivot = rem((unsigned long)(long)M[i][k], p, red_struct);
         if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
            pos = i;
            break;
         }
      }

      if (pos != -1) {
         if (k != pos) {
            swap(M[pos], M[k]);
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;
         }

         det = MulMod(det, pivot, p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); // t1*pinv;
            double * NTL_RESTRICT y = &M[k][0];
            for (long j = 0; j < n; j++) {
               long t2 = rem((unsigned long)(long)y[j], p, red_struct);
               y[j] = MulModPrecon(t2, t1, p, t1pinv);
            }

            y[k] = pivot_inv;
         }


         NTL_GEXEC_RANGE(seq, n, first, last)  
         NTL_IMPORT(p)
         NTL_IMPORT(n)
         NTL_IMPORT(k)
         NTL_IMPORT(red_struct)
         double * NTL_RESTRICT y = &M[k][0]; 
         if (cleanup) {
            for (long i = first; i < last; i++) {
               if (i == k) continue;
               // skip row k: the data won't change, but it
               // technically is a race condition in a multi-theaded
               // execution, and it would violate the "restrict"
               // contract

               double * NTL_RESTRICT x = &M[i][0]; 
               for (long j = 0; j < n; j++) {
                  x[j] = rem((unsigned long)(long)x[j], p, red_struct);
               }
            }
         }


         for (long i = first; i < last; i++) {
            if (i == k) continue; // skip row k

            double * NTL_RESTRICT x = &M[i][0]; 
            long t1 = rem((unsigned long)(long)x[k], p, red_struct);
            t1 = NegateMod(t1, p);
            x[k] = 0;
            if (t1 == 0) continue;

            // add t1 * row k to row i
            double ut1 = t1;
            muladd_interval1(x, y, ut1, n);
         }
         NTL_GEXEC_RANGE_END
      }
      else {
         clear(d);
         return;
      }
   }


   if (pivoting) {
      // pivot colums, using reverse swap sequence

      for (long i = 0; i < n; i++) {
         double * NTL_RESTRICT x = &M[i][0]; 

         for (long k = n-1; k >= 0; k--) {
            long pos = P[k];
            if (pos != k) _ntl_swap(x[pos], x[k]);
         }
      }
   }


   X.SetDims(n, n);
   for (long i = 0; i < n; i++)
      for (long j = 0; j < n; j++)
         X[i][j].LoopHole() = rem((unsigned long)(long)M[i][j], p, red_struct);

   d.LoopHole() = det;
}

#endif





#ifdef NTL_HAVE_AVX

static
void blk_inv_DD(zz_p& d, mat_zz_p& X, const mat_zz_p& A, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("inv: nonsquare matrix");

   if (n == 0) {
      set(d);
      X.SetDims(0, 0);
      return;
   }

   if (NTL_OVERFLOW(n, MAT_BLK_SZ, 0)) ResourceError("dimension too large");

   long npanels = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;
   

   Vec< AlignedArray<double> > M;
   M.SetLength(npanels);
   for (long panel = 0; panel < npanels; panel++) {
      M[panel].SetLength(n*MAT_BLK_SZ);
      double *panelp = &M[panel][0];

      for (long r = 0; r < n*MAT_BLK_SZ; r++) panelp[r] = 0;
   }

   // copy A into panels
   for (long jj = 0, panel = 0; jj < n; jj += MAT_BLK_SZ, panel++) {
      long j_max = min(jj+MAT_BLK_SZ, n);
      double *panelp = &M[panel][0];

      for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
         const zz_p *ap = A[i].elts() + jj;

         for (long j = jj; j < j_max; j++)
            panelp[j-jj] = rep(ap[j-jj]);
      }
   }
            
   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations


   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_reduce_struct red_struct = zz_p::red_struct();


   bool seq = double(n)*double(n)*double(MAT_BLK_SZ) < PAR_THRESH;

   bool pivoting = false;

   long red_trigger = (MAX_DBL_INT-(p-1))/((p-1)*(p-1));
   long red_count = red_trigger;

   for (long kk = 0, kpanel = 0; kk < n; kk += MAT_BLK_SZ, kpanel++) {
      long k_max = min(kk+MAT_BLK_SZ, n);

      bool cleanup = false;

      if (red_count-MAT_BLK_SZ < 0) {
         red_count = red_trigger;
         cleanup = true;
      }

      red_count = red_count-MAT_BLK_SZ;
      double * NTL_RESTRICT kpanelp = &M[kpanel][0];

      if (cleanup) {
         for (long r = 0; r < n*MAT_BLK_SZ; r++) 
            kpanelp[r] = rem((unsigned long)(long)kpanelp[r], p, red_struct);
      }

      for (long k = kk; k < k_max; k++) {

         long pos = -1;
         long pivot;
         long pivot_inv;

         for (long i = k; i < n; i++) {
            // NOTE: by using InvModStatus, this code will work
            // for prime-powers as well as primes
            pivot = rem((unsigned long)(long)kpanelp[i*MAT_BLK_SZ+(k-kk)], p, red_struct);
            if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
               pos = i;
               break;
            }
         }

         if (pos == -1) {
            clear(d);
            return;
         }

         double * NTL_RESTRICT y = &kpanelp[k*MAT_BLK_SZ];
         if (k != pos) {
            // swap rows pos and k
            double * NTL_RESTRICT x = &kpanelp[pos*MAT_BLK_SZ];
            for (long j = 0; j < MAT_BLK_SZ; j++) _ntl_swap(x[j], y[j]);
            
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;
         }

         det = MulMod(det, pivot, p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 
            for (long j = 0; j < MAT_BLK_SZ; j++) {
               long t2 = rem((unsigned long)(long)y[j], p, red_struct);
               y[j] = MulModPrecon(t2, t1, p, t1pinv);
            }

            y[k-kk] = pivot_inv;
         }

         for (long i = 0; i < n; i++) {
            if (i == k) continue; // skip row k

            double * NTL_RESTRICT x = &kpanelp[i*MAT_BLK_SZ];
            long t1 = rem((unsigned long)(long)x[k-kk], p, red_struct);
            t1 = NegateMod(t1, p);
            x[k-kk] = 0;
            if (t1 == 0) continue;

            // add t1 * row k to row i
            double ut1 = t1;
            muladd_interval(x, y, ut1, MAT_BLK_SZ);
         }
      }


      // finished processing current kpanel
      // next, reduce and apply to all other kpanels

      for (long r = 0; r < n*MAT_BLK_SZ; r++) 
         kpanelp[r] = rem((unsigned long)(long)kpanelp[r], p, red_struct);

      // special processing: subtract 1 off of diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = SubMod((long)kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);


      NTL_GEXEC_RANGE(seq, npanels, first, last)  
      NTL_IMPORT(p)
      NTL_IMPORT(n)
      NTL_IMPORT(red_struct)
      NTL_IMPORT(kpanel)
      NTL_IMPORT(kpanelp)
      NTL_IMPORT(kk)
      NTL_IMPORT(k_max)


      AlignedArray<double> buf_store;
      buf_store.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
      double *buf = &buf_store[0];

      for (long jpanel = first; jpanel < last; jpanel++) {
         if (jpanel == kpanel) continue;

         double * NTL_RESTRICT jpanelp = &M[jpanel][0];

         if (cleanup) {
            for (long r = 0; r < n*MAT_BLK_SZ; r++) 
               jpanelp[r] = rem((unsigned long)(long)jpanelp[r], p, red_struct);
         }

         // perform swaps
         for (long k = kk; k < k_max; k++) {
            long pos = P[k];
            if (pos != k) {
               // swap rows pos and k
               double * NTL_RESTRICT pos_p = &jpanelp[pos*MAT_BLK_SZ];
               double * NTL_RESTRICT k_p = &jpanelp[k*MAT_BLK_SZ];
               for (long j = 0; j < MAT_BLK_SZ; j++)
                  _ntl_swap(pos_p[j], k_p[j]);
            }
         }

         // copy block number kpanel (the one on the diagonal)  into buf

         for (long i = 0; i < (k_max-kk)*MAT_BLK_SZ; i++)
            buf[i] = rem((unsigned long)(long)jpanelp[kk*MAT_BLK_SZ+i], p, red_struct);

         // jpanel += kpanel*buf

         muladd_all_by_32(0, n, jpanelp, kpanelp, buf, k_max-kk);
      }
                  
      NTL_GEXEC_RANGE_END

      // special processing: add 1 back to the diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = AddMod((long)kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);

   }

   if (pivoting) {
      // pivot colums, using reverse swap sequence

      for (long k = n-1; k >= 0; k--) {
         long pos = P[k];
         if (pos != k) { 
            // swap columns pos and k

            double * NTL_RESTRICT x = &M[pos / MAT_BLK_SZ][pos % MAT_BLK_SZ];
            double * NTL_RESTRICT y = &M[k / MAT_BLK_SZ][k % MAT_BLK_SZ];
            for (long i = 0; i < n; i++) {
               _ntl_swap(x[i*MAT_BLK_SZ], y[i*MAT_BLK_SZ]);
            }
         }
      }
   }


   // copy panels into X
   X.SetDims(n, n);
   for (long jj = 0, panel = 0; jj < n; jj += MAT_BLK_SZ, panel++) {
      long j_max = min(jj+MAT_BLK_SZ, n);
      double *panelp = &M[panel][0];

      for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
         zz_p *xp = X[i].elts() + jj;

         for (long j = jj; j < j_max; j++)
            xp[j-jj].LoopHole() = rem((unsigned long)(long)panelp[j-jj], p, red_struct);
      }
   }

   d.LoopHole() = det;

}

#endif



static
void blk_inv_L(zz_p& d, mat_zz_p& X, const mat_zz_p& A, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("inv: nonsquare matrix");

   if (n == 0) {
      set(d);
      X.SetDims(0, 0);
      return;
   }

   if (NTL_OVERFLOW(n, MAT_BLK_SZ, 0)) ResourceError("dimension too large");

   long npanels = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;

   Vec< UniqueArray<unsigned long> > M;
   M.SetLength(npanels);
   for (long panel = 0; panel < npanels; panel++) {
      M[panel].SetLength(n*MAT_BLK_SZ);
      unsigned long *panelp = &M[panel][0];

      for (long r = 0; r < n*MAT_BLK_SZ; r++) panelp[r] = 0;
   }
   
   // copy A into panels
   for (long jj = 0, panel = 0; jj < n; jj += MAT_BLK_SZ, panel++) {
      long j_max = min(jj+MAT_BLK_SZ, n);
      unsigned long *panelp = &M[panel][0];

      for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
         const zz_p *ap = A[i].elts() + jj;

         for (long j = jj; j < j_max; j++)
            panelp[j-jj] = rep(ap[j-jj]);
      }
   }
            
   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations


   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_reduce_struct red_struct = zz_p::red_struct();


   bool seq = double(n)*double(n)*double(MAT_BLK_SZ) < PAR_THRESH;

   bool pivoting = false;

   unsigned long ured_trigger = 
      (~(0UL)-cast_unsigned(p-1))/(cast_unsigned(p-1)*cast_unsigned(p-1));
   // NOTE: corner case at p == 2: need unsigned long to prevent overflow

   long red_trigger = min(cast_unsigned(NTL_MAX_LONG), ured_trigger);

   long red_count = red_trigger;

   for (long kk = 0, kpanel = 0; kk < n; kk += MAT_BLK_SZ, kpanel++) {
      long k_max = min(kk+MAT_BLK_SZ, n);

      bool cleanup = false;

      if (red_count-MAT_BLK_SZ < 0) {
         red_count = red_trigger;
         cleanup = true;
      }

      red_count = red_count-MAT_BLK_SZ;
      unsigned long * NTL_RESTRICT kpanelp = &M[kpanel][0];

      if (cleanup) {
         for (long r = 0; r < n*MAT_BLK_SZ; r++) 
            kpanelp[r] = rem(kpanelp[r], p, red_struct);
      }

      for (long k = kk; k < k_max; k++) {

         long pos = -1;
         long pivot;
         long pivot_inv;

         for (long i = k; i < n; i++) {
            // NOTE: by using InvModStatus, this code will work
            // for prime-powers as well as primes
            pivot = rem(kpanelp[i*MAT_BLK_SZ+(k-kk)], p, red_struct);
            if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
               pos = i;
               break;
            }
         }

         if (pos == -1) {
            clear(d);
            return;
         }

         unsigned long * NTL_RESTRICT y = &kpanelp[k*MAT_BLK_SZ];
         if (k != pos) {
            // swap rows pos and k
            unsigned long * NTL_RESTRICT x = &kpanelp[pos*MAT_BLK_SZ];
            for (long j = 0; j < MAT_BLK_SZ; j++) _ntl_swap(x[j], y[j]);
            
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;
         }

         det = MulMod(det, pivot, p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 
            for (long j = 0; j < MAT_BLK_SZ; j++) {
               long t2 = rem(y[j], p, red_struct);
               y[j] = MulModPrecon(t2, t1, p, t1pinv);
            }

            y[k-kk] = pivot_inv;
         }

         for (long i = 0; i < n; i++) {
            if (i == k) continue; // skip row k

            unsigned long * NTL_RESTRICT x = &kpanelp[i*MAT_BLK_SZ];
            long t1 = rem(x[k-kk], p, red_struct);
            t1 = NegateMod(t1, p);
            x[k-kk] = 0;
            if (t1 == 0) continue;

            // add t1 * row k to row i
            unsigned long ut1 = t1;
            muladd_interval(x, y, ut1, MAT_BLK_SZ);
         }
      }


      // finished processing current kpanel
      // next, reduce and apply to all other kpanels

      for (long r = 0; r < n*MAT_BLK_SZ; r++) 
         kpanelp[r] = rem(kpanelp[r], p, red_struct);

      // special processing: subtract 1 off of diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = SubMod((long)kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);


      NTL_GEXEC_RANGE(seq, npanels, first, last)  
      NTL_IMPORT(p)
      NTL_IMPORT(n)
      NTL_IMPORT(red_struct)
      NTL_IMPORT(kpanel)
      NTL_IMPORT(kpanelp)
      NTL_IMPORT(kk)
      NTL_IMPORT(k_max)


      UniqueArray<unsigned long> buf_store;
      buf_store.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
      unsigned long *buf = &buf_store[0];

      for (long jpanel = first; jpanel < last; jpanel++) {
         if (jpanel == kpanel) continue;

         unsigned long * NTL_RESTRICT jpanelp = &M[jpanel][0];

         if (cleanup) {
            for (long r = 0; r < n*MAT_BLK_SZ; r++) 
               jpanelp[r] = rem(jpanelp[r], p, red_struct);
         }

         // perform swaps
         for (long k = kk; k < k_max; k++) {
            long pos = P[k];
            if (pos != k) {
               // swap rows pos and k
               unsigned long * NTL_RESTRICT pos_p = &jpanelp[pos*MAT_BLK_SZ];
               unsigned long * NTL_RESTRICT k_p = &jpanelp[k*MAT_BLK_SZ];
               for (long j = 0; j < MAT_BLK_SZ; j++)
                  _ntl_swap(pos_p[j], k_p[j]);
            }
         }

         // copy block number kpanel (the one on the diagonal)  into buf
         // here, we transpose it

         for (long k = kk; k < k_max; k++) 
            for (long j = 0; j < MAT_BLK_SZ; j++)
               buf[j*MAT_BLK_SZ + (k-kk)] = 
                  rem(jpanelp[k*MAT_BLK_SZ+j], p, red_struct);

         // jpanel += kpanel*buf

         muladd_all_by_32(0, n, jpanelp, kpanelp, buf, k_max-kk);
      }
                  
      NTL_GEXEC_RANGE_END

      // special processing: add 1 back to the diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = AddMod((long)kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);

   }

   if (pivoting) {
      // pivot colums, using reverse swap sequence

      for (long k = n-1; k >= 0; k--) {
         long pos = P[k];
         if (pos != k) { 
            // swap columns pos and k

            unsigned long * NTL_RESTRICT x = &M[pos / MAT_BLK_SZ][pos % MAT_BLK_SZ];
            unsigned long * NTL_RESTRICT y = &M[k / MAT_BLK_SZ][k % MAT_BLK_SZ];
            for (long i = 0; i < n; i++) {
               _ntl_swap(x[i*MAT_BLK_SZ], y[i*MAT_BLK_SZ]);
            }
         }
      }
   }

   // copy panels into X
   X.SetDims(n, n);
   for (long jj = 0, panel = 0; jj < n; jj += MAT_BLK_SZ, panel++) {
      long j_max = min(jj+MAT_BLK_SZ, n);
      unsigned long *panelp = &M[panel][0];

      for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
         zz_p *xp = X[i].elts() + jj;

         for (long j = jj; j < j_max; j++)
            xp[j-jj].LoopHole() = rem(panelp[j-jj], p, red_struct);
      }
   }

   d.LoopHole() = det;

}








static
void blk_inv_LL(zz_p& d, mat_zz_p& X, const mat_zz_p& A, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("inv: nonsquare matrix");

   if (n == 0) {
      set(d);
      X.SetDims(0, 0);
      return;
   }

   if (NTL_OVERFLOW(n, MAT_BLK_SZ, 0)) ResourceError("dimension too big");

   long npanels = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;

   Vec< UniqueArray<long> > M;
   M.SetLength(npanels);
   for (long panel = 0; panel < npanels; panel++) {
      M[panel].SetLength(n*MAT_BLK_SZ);
      long *panelp = &M[panel][0];

      for (long r = 0; r < n*MAT_BLK_SZ; r++) panelp[r] = 0;
   }
   

   // copy A into panels
   for (long jj = 0, panel = 0; jj < n; jj += MAT_BLK_SZ, panel++) {
      long j_max = min(jj+MAT_BLK_SZ, n);
      long *panelp = &M[panel][0];

      for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
         const zz_p *ap = A[i].elts() + jj;

         for (long j = jj; j < j_max; j++)
            panelp[j-jj] = rep(ap[j-jj]);
      }
   }
            
   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations


   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_ll_reduce_struct ll_red_struct = zz_p::ll_red_struct();


   bool seq = double(n)*double(n)*double(MAT_BLK_SZ) < PAR_THRESH;

   bool pivoting = false;

   for (long kk = 0, kpanel = 0; kk < n; kk += MAT_BLK_SZ, kpanel++) {
      long k_max = min(kk+MAT_BLK_SZ, n);

      long * NTL_RESTRICT kpanelp = &M[kpanel][0];


      for (long k = kk; k < k_max; k++) {

         long pos = -1;
         long pivot;
         long pivot_inv;

         for (long i = k; i < n; i++) {
            // NOTE: by using InvModStatus, this code will work
            // for prime-powers as well as primes
            pivot = kpanelp[i*MAT_BLK_SZ+(k-kk)];
            if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
               pos = i;
               break;
            }
         }

         if (pos == -1) {
            clear(d);
            return;
         }

         long * NTL_RESTRICT y = &kpanelp[k*MAT_BLK_SZ];
         if (k != pos) {
            // swap rows pos and k
            long * NTL_RESTRICT x = &kpanelp[pos*MAT_BLK_SZ];
            for (long j = 0; j < MAT_BLK_SZ; j++) _ntl_swap(x[j], y[j]);
            
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;
         }

         det = MulMod(det, pivot, p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 
            for (long j = 0; j < MAT_BLK_SZ; j++) {
               y[j] = MulModPrecon(y[j], t1, p, t1pinv);
            }

            y[k-kk] = pivot_inv;
         }

         for (long i = 0; i < n; i++) {
            if (i == k) continue; // skip row k

            long * NTL_RESTRICT x = &kpanelp[i*MAT_BLK_SZ];
            long t1 = x[k-kk];
            t1 = NegateMod(t1, p);
            x[k-kk] = 0;
            if (t1 == 0) continue;

            // add t1 * row k to row i
            long ut1 = t1;
            muladd_interval(x, y, ut1, MAT_BLK_SZ, p, pinv);
         }
      }


      // finished processing current kpanel
      // next, reduce and apply to all other kpanels

      // special processing: subtract 1 off of diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = SubMod(kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);


      NTL_GEXEC_RANGE(seq, npanels, first, last)  
      NTL_IMPORT(p)
      NTL_IMPORT(n)
      NTL_IMPORT(ll_red_struct)
      NTL_IMPORT(kpanel)
      NTL_IMPORT(kpanelp)
      NTL_IMPORT(kk)
      NTL_IMPORT(k_max)


      UniqueArray<long> buf_store;
      buf_store.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
      long *buf = &buf_store[0];

      for (long jpanel = first; jpanel < last; jpanel++) {
         if (jpanel == kpanel) continue;

         long * NTL_RESTRICT jpanelp = &M[jpanel][0];

         // perform swaps
         for (long k = kk; k < k_max; k++) {
            long pos = P[k];
            if (pos != k) {
               // swap rows pos and k
               long * NTL_RESTRICT pos_p = &jpanelp[pos*MAT_BLK_SZ];
               long * NTL_RESTRICT k_p = &jpanelp[k*MAT_BLK_SZ];
               for (long j = 0; j < MAT_BLK_SZ; j++)
                  _ntl_swap(pos_p[j], k_p[j]);
            }
         }

         // copy block number kpanel (the one on the diagonal)  into buf
         // here, we transpose it

         for (long k = kk; k < k_max; k++) 
            for (long j = 0; j < MAT_BLK_SZ; j++)
               buf[j*MAT_BLK_SZ + (k-kk)] = 
                  jpanelp[k*MAT_BLK_SZ+j];


         // jpanel += kpanel*buf

         muladd_all_by_32(0, n, jpanelp, kpanelp, buf, k_max-kk, p, ll_red_struct);
      }
                  
      NTL_GEXEC_RANGE_END

      // special processing: add 1 back to the diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = AddMod(kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);

   }

   if (pivoting) {
      // pivot colums, using reverse swap sequence

      for (long k = n-1; k >= 0; k--) {
         long pos = P[k];
         if (pos != k) { 
            // swap columns pos and k

            long * NTL_RESTRICT x = &M[pos / MAT_BLK_SZ][pos % MAT_BLK_SZ];
            long * NTL_RESTRICT y = &M[k / MAT_BLK_SZ][k % MAT_BLK_SZ];
            for (long i = 0; i < n; i++) {
               _ntl_swap(x[i*MAT_BLK_SZ], y[i*MAT_BLK_SZ]);
            }
         }
      }
   }

   // copy panels into X
   X.SetDims(n, n);
   for (long jj = 0, panel = 0; jj < n; jj += MAT_BLK_SZ, panel++) {
      long j_max = min(jj+MAT_BLK_SZ, n);
      long *panelp = &M[panel][0];

      for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
         zz_p *xp = X[i].elts() + jj;

         for (long j = jj; j < j_max; j++)
            xp[j-jj].LoopHole() = panelp[j-jj];
      }
   }

   d.LoopHole() = det;

}



#endif



void relaxed_inv(zz_p& d, mat_zz_p& X, const mat_zz_p& A, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("inv: nonsquare matrix");

#ifndef NTL_HAVE_LL_TYPE

   basic_inv(d, X, A, relax);

#else

   long p = zz_p::modulus();

   if (n < 16) {
      //cerr << "basic_inv\n";
      basic_inv(d, X, A, relax);
   }
   else if (n/MAT_BLK_SZ < 4) {
      long V = 64;

#ifdef NTL_HAVE_AVX
      if (p-1 <= MAX_DBL_INT &&
          V <= (MAX_DBL_INT-(p-1))/(p-1) &&
          V*(p-1) <= (MAX_DBL_INT-(p-1))/(p-1)) {

         //cerr << "alt_inv_DD\n";
         alt_inv_DD(d, X, A, relax);
      }
      else 
#endif
           if (cast_unsigned(V) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1) &&
               cast_unsigned(V)*cast_unsigned(p-1) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1))  {

         //cerr << "alt_inv_L\n";
         alt_inv_L(d, X, A, relax);

      }
      else {
  
         //cerr << "basic_inv\n";
         basic_inv(d, X, A, relax);
      }
   }
   else {
      long V = 4*MAT_BLK_SZ;

#ifdef NTL_HAVE_AVX
      if (p-1 <= MAX_DBL_INT &&
          V <= (MAX_DBL_INT-(p-1))/(p-1) &&
          V*(p-1) <= (MAX_DBL_INT-(p-1))/(p-1)) {

         //cerr << "blk_inv_DD\n";
         blk_inv_DD(d, X, A, relax);
      }
      else 
#endif
           if (cast_unsigned(V) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1) &&
               cast_unsigned(V)*cast_unsigned(p-1) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1))  {

         //cerr << "blk_inv_L\n";
         blk_inv_L(d, X, A, relax);

      }
      else {
  
         //cerr << "blk_inv_LL\n";
         blk_inv_LL(d, X, A, relax);
      }
   
   }

#endif



}



// ******************************************************************
//
// Triangularizing square matrices, with applications
// to solving linear systems and computing determinants.
// Should be about 3x faster than the matrix inverse
// algorithms.
//
// ******************************************************************


static
void basic_tri(zz_p& d, const mat_zz_p& A, const vec_zz_p *bp, 
               vec_zz_p *xp, bool trans, bool relax)
{
   long n = A.NumRows();

   // adjust
   if (A.NumCols() != n)
      LogicError("tri: nonsquare matrix");

   // adjust
   if (bp && bp->length() != n)
      LogicError("tri: dimension mismatch");

   // adjust
   if (bp && !xp)
      LogicError("tri: bad args");

   if (n == 0) {
      set(d);
      // adjust
      if (xp) xp->SetLength(0);
      return;
   }

   // adjust (several lines)
   // scratch space
   Mat<long> M;
   if (!trans) {
      conv(M, A);
   }
   else {
      M.SetDims(n, n);
      for (long i = 0; i < n; i++)
         for (long j = 0; j < n; j++)
            M[i][j] = rep(A[j][i]); 
   }

   Vec<long> bv;
   if (bp) conv(bv, *bp);
   // end adjust


   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations

   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();


   bool pivoting = false;

   for (long k = 0; k < n; k++) {
      long pos = -1;
      long pivot_inv;
      for (long i = k; i < n; i++) {
         // NOTE: by using InvModStatus, this code will work
         // for prime-powers as well as primes
         long pivot = M[i][k];
         if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
            pos = i;
            break;
         }
      }

      if (pos != -1) {
         if (k != pos) {
            swap(M[pos], M[k]);
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;

            // adjust
            if (bp) _ntl_swap(bv[pos], bv[k]);
         }

         det = MulMod(det, M[k][k], p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 
            long * NTL_RESTRICT y = &M[k][0];
            // adjust
            for (long j = k+1; j < n; j++) 
               y[j] = MulModPrecon(y[j], t1, p, t1pinv);

            // adjust // y[k] = pivot_inv;

            // adjust
            if (bp) bv[k] = MulModPrecon(bv[k], t1, p, t1pinv);
         }



         // adjust
         bool seq = n-(k+1) < PAR_THRESH_SQ;
         NTL_GEXEC_RANGE(seq, n-(k+1), first, last)  
         NTL_IMPORT(p)
         NTL_IMPORT(n)
         NTL_IMPORT(k)
         long * NTL_RESTRICT y = &M[k][0]; 

         // adjust
         for (long ii = first; ii < last; ii++) {
            long i = ii + k+1;

            long * NTL_RESTRICT x = &M[i][0]; 
            long t1 = x[k];
            t1 = NegateMod(t1, p);
            // adjust // x[k] = 0;
            if (t1 == 0) continue;

            // add t1 * row k to row i
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 

            // adjust
            for (long j = k+1; j < n; j++) {
               long t2 = MulModPrecon(y[j], t1, p, t1pinv);
               x[j] = AddMod(x[j], t2, p);
            }

            // adjust
            if (bp)
            {
               long t2 = MulModPrecon(bv[k], t1, p, t1pinv);
               bv[i] = AddMod(bv[i], t2, p);
            }
         }
         NTL_GEXEC_RANGE_END
      }
      else {
         clear(d);
         return;
      }
   }


   // adjust
   if (bp) {
      xp->SetLength(n);
      zz_p *X = xp->elts();

      for (long i = n-1; i >= 0; i--) {
         long t1 = 0;
         for (long j = i+1; j < n; j++) {
            long t2 = MulMod(rep(X[j]), M[i][j], p);
            t1 = AddMod(t1, t2, p);
         }
         X[i].LoopHole() = SubMod(bv[i], t1, p);
      }
   }

   d.LoopHole() = det;
}




#ifdef NTL_HAVE_LL_TYPE



static
void alt_tri_L(zz_p& d, const mat_zz_p& A, const vec_zz_p *bp, 
               vec_zz_p *xp, bool trans, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("tri: nonsquare matrix");

   // adjust
   if (bp && bp->length() != n)
      LogicError("tri: dimension mismatch");

   // adjust
   if (bp && !xp)
      LogicError("tri: bad args");

   if (n == 0) {
      set(d);
      if (xp) xp->SetLength(0);
      return;
   }


   // scratch space
   Mat<unsigned long> M;
   if (!trans) {
      conv(M, A);
   }
   else {
      M.SetDims(n, n);
      for (long i = 0; i < n; i++)
         for (long j = 0; j < n; j++)
            M[i][j] = rep(A[j][i]); 
   }

   Vec<long> bv;
   if (bp) conv(bv, *bp);

   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations

   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_reduce_struct red_struct = zz_p::red_struct();

   

   bool pivoting = false;

   unsigned long ured_trigger = 
      (~(0UL)-cast_unsigned(p-1))/(cast_unsigned(p-1)*cast_unsigned(p-1));
   // NOTE: corner case at p == 2: need unsigned long to prevent overflow

   long red_trigger = min(cast_unsigned(NTL_MAX_LONG), ured_trigger);

   long red_count = red_trigger;


   for (long k = 0; k < n; k++) {
      bool cleanup = false;

      if (red_count-1 < 0) {
         red_count = red_trigger;
         cleanup = true;
      }

      red_count = red_count-1;

      long pos = -1;
      long pivot;
      long pivot_inv;

      for (long i = k; i < n; i++) {
         // NOTE: by using InvModStatus, this code will work
         // for prime-powers as well as primes
         pivot = rem(M[i][k], p, red_struct);
         if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
            pos = i;
            break;
         }
      }

      if (pos != -1) {
         if (k != pos) {
            swap(M[pos], M[k]);
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;

            if (bp) _ntl_swap(bv[pos], bv[k]);
         }

         det = MulMod(det, pivot, p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); // t1*pinv;
            unsigned long * NTL_RESTRICT y = &M[k][0];
            for (long j = k+1; j < n; j++) {
               long t2 = rem(y[j], p, red_struct);
               y[j] = MulModPrecon(t2, t1, p, t1pinv);
            }

            if (bp) bv[k] = MulModPrecon(bv[k], t1, p, t1pinv);
         }


    
         bool seq = n-(k+1) < PAR_THRESH_SQ;
         NTL_GEXEC_RANGE(seq, n-(k+1), first, last)  
         NTL_IMPORT(p)
         NTL_IMPORT(n)
         NTL_IMPORT(k)
         NTL_IMPORT(red_struct)
         unsigned long * NTL_RESTRICT y = &M[k][0]; 
         if (cleanup) {
            for (long ii = first; ii < last; ii++) {
               long i = ii + k+1;

               unsigned long * NTL_RESTRICT x = &M[i][0]; 
               for (long j = k+1; j < n; j++) {
                  x[j] = rem(x[j], p, red_struct);
               }
            }
         }


         for (long ii = first; ii < last; ii++) {
            long i = ii + k+1;

            unsigned long * NTL_RESTRICT x = &M[i][0]; 
            long t1 = rem(x[k], p, red_struct);
            t1 = NegateMod(t1, p);
            if (t1 == 0) continue;

            // add t1 * row k to row i
            unsigned long ut1 = t1;
            long j;
            for (j = k+1; j <= n-4; j+=4) {
               unsigned long xj0 = x[j+0] + DO_MUL(y[j+0], ut1);
               unsigned long xj1 = x[j+1] + DO_MUL(y[j+1], ut1);
               unsigned long xj2 = x[j+2] + DO_MUL(y[j+2], ut1);
               unsigned long xj3 = x[j+3] + DO_MUL(y[j+3], ut1);
               x[j+0] = xj0;
               x[j+1] = xj1;
               x[j+2] = xj2;
               x[j+3] = xj3;
            }
            for (; j < n; j++) {
               x[j] += DO_MUL(y[j], ut1);
            }

            if (bp)
            {
               long t2 = MulMod(bv[k], t1, p);
               bv[i] = AddMod(bv[i], t2, p);
            }
         }
         NTL_GEXEC_RANGE_END
      }
      else {
         clear(d);
         return;
      }
   }



   if (bp) {
      xp->SetLength(n);
      zz_p *X = xp->elts();

      for (long i = n-1; i >= 0; i--) {
         long t1 = 0;
         for (long j = i+1; j < n; j++) {
            long t0 = rem(M[i][j], p, red_struct);
            long t2 = MulMod(rep(X[j]), t0, p);
            t1 = AddMod(t1, t2, p);
         }
         X[i].LoopHole() = SubMod(bv[i], t1, p);
      }
   }

   d.LoopHole() = det;
}




#ifdef NTL_HAVE_AVX

static
void alt_tri_DD(zz_p& d, const mat_zz_p& A, const vec_zz_p *bp, 
               vec_zz_p *xp, bool trans, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("tri: nonsquare matrix");

   // adjust
   if (bp && bp->length() != n)
      LogicError("tri: dimension mismatch");

   // adjust
   if (bp && !xp)
      LogicError("tri: bad args");

   if (n == 0) {
      set(d);
      if (xp) xp->SetLength(0);
      return;
   }


   // scratch space

   Vec< AlignedArray<double> > M;
   M.SetLength(n);
   for (long i = 0; i < n; i++) M[i].SetLength(n);
   if (!trans) {
      for (long i = 0; i < n; i++)
         for (long j = 0; j < n; j++)
            M[i][j] = rep(A[i][j]); 
   }
   else {
      for (long i = 0; i < n; i++)
         for (long j = 0; j < n; j++)
            M[i][j] = rep(A[j][i]); 
   }

   Vec<long> bv;
   if (bp) conv(bv, *bp);

   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations

   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_reduce_struct red_struct = zz_p::red_struct();

   

   bool pivoting = false;

   long red_trigger = (MAX_DBL_INT-(p-1))/((p-1)*(p-1));
   long red_count = red_trigger;

   for (long k = 0; k < n; k++) {
      bool cleanup = false;

      if (red_count-1 < 0) {
         red_count = red_trigger;
         cleanup = true;
      }

      red_count = red_count-1;

      long pos = -1;
      long pivot;
      long pivot_inv;

      for (long i = k; i < n; i++) {
         // NOTE: by using InvModStatus, this code will work
         // for prime-powers as well as primes
         pivot = rem((unsigned long)(long)M[i][k], p, red_struct);
         if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
            pos = i;
            break;
         }
      }

      if (pos != -1) {
         if (k != pos) {
            swap(M[pos], M[k]);
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;

            if (bp) _ntl_swap(bv[pos], bv[k]);
         }

         det = MulMod(det, pivot, p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); // t1*pinv;
            double * NTL_RESTRICT y = &M[k][0];
            for (long j = k+1; j < n; j++) {
               long t2 = rem((unsigned long)(long)y[j], p, red_struct);
               y[j] = MulModPrecon(t2, t1, p, t1pinv);
            }

            if (bp) bv[k] = MulModPrecon(bv[k], t1, p, t1pinv);
         }


    
         bool seq = n-(k+1) < PAR_THRESH_SQ;
         NTL_GEXEC_RANGE(seq, n-(k+1), first, last)  
         NTL_IMPORT(p)
         NTL_IMPORT(n)
         NTL_IMPORT(k)
         NTL_IMPORT(red_struct)
         double * NTL_RESTRICT y = &M[k][0]; 
         if (cleanup) {
            for (long ii = first; ii < last; ii++) {
               long i = ii + k+1;

               double * NTL_RESTRICT x = &M[i][0]; 
               for (long j = k+1; j < n; j++) {
                  x[j] = rem((unsigned long)(long)x[j], p, red_struct);
               }
            }
         }

         long align_boundary = 
            min((((k+1)+(NTL_AVX_DBL_ALIGN-1))/NTL_AVX_DBL_ALIGN)*NTL_AVX_DBL_ALIGN, n);


         for (long ii = first; ii < last; ii++) {
            long i = ii + k+1;

            double * NTL_RESTRICT x = &M[i][0]; 
            long t1 = rem((unsigned long)(long)x[k], p, red_struct);
            t1 = NegateMod(t1, p);
            if (t1 == 0) continue;

            // add t1 * row k to row i
            double ut1 = t1;
            for (long j = k+1; j < align_boundary; j++) x[j] += y[j]*ut1;
            muladd_interval1(x+align_boundary, y+align_boundary, ut1, n-align_boundary);

            if (bp)
            {
               long t2 = MulMod(bv[k], t1, p);
               bv[i] = AddMod(bv[i], t2, p);
            }
         }
         NTL_GEXEC_RANGE_END
      }
      else {
         clear(d);
         return;
      }
   }



   if (bp) {
      xp->SetLength(n);
      zz_p *X = xp->elts();

      for (long i = n-1; i >= 0; i--) {
         long t1 = 0;
         for (long j = i+1; j < n; j++) {
            long t0 = rem((unsigned long)(long)M[i][j], p, red_struct);
            long t2 = MulMod(rep(X[j]), t0, p);
            t1 = AddMod(t1, t2, p);
         }
         X[i].LoopHole() = SubMod(bv[i], t1, p);
      }
   }

   d.LoopHole() = det;
}


#endif




#ifdef NTL_HAVE_AVX

static
void blk_tri_DD(zz_p& d, const mat_zz_p& A, const vec_zz_p *bp, 
               vec_zz_p *xp, bool trans, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("tri: nonsquare matrix");

   if (bp && bp->length() != n)
      LogicError("tri: dimension mismatch");

   if (bp && !xp)
      LogicError("tri: bad args");

   if (n == 0) {
      set(d);
      if (xp) xp->SetLength(0);
      return;
   }

   if (NTL_OVERFLOW(n, MAT_BLK_SZ, 0)) ResourceError("dimension too large");

   long npanels = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;
   
   Vec< AlignedArray<double> > M;
   M.SetLength(npanels);
   for (long panel = 0; panel < npanels; panel++) {
      M[panel].SetLength(n*MAT_BLK_SZ);
      double *panelp = &M[panel][0];

      for (long r = 0; r < n*MAT_BLK_SZ; r++) panelp[r] = 0;
   }

   if (trans) {
      // copy A transposed into panels
      for (long i = 0; i < n; i++) {
         const zz_p *row = &A[i][0];
         double *col = &M[i/MAT_BLK_SZ][i%MAT_BLK_SZ];
         for (long j = 0; j < n; j++) 
            col[j*MAT_BLK_SZ] = rep(row[j]);
      }
   }
   else {
      // copy A into panels
      for (long jj = 0, panel = 0; jj < n; jj += MAT_BLK_SZ, panel++) {
         long j_max = min(jj+MAT_BLK_SZ, n);
         double *panelp = &M[panel][0];

         for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
            const zz_p *ap = A[i].elts() + jj;

            for (long j = jj; j < j_max; j++)
               panelp[j-jj] = rep(ap[j-jj]);
         }
      }
   }

   Vec<long> bv;
   if (bp) conv(bv, *bp);
            
   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations


   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_reduce_struct red_struct = zz_p::red_struct();


   bool pivoting = false;

   long red_trigger = (MAX_DBL_INT-(p-1))/((p-1)*(p-1));
   long red_count = red_trigger;

   for (long kk = 0, kpanel = 0; kk < n; kk += MAT_BLK_SZ, kpanel++) {
      long k_max = min(kk+MAT_BLK_SZ, n);

      bool cleanup = false;

      if (red_count-MAT_BLK_SZ < 0) {
         red_count = red_trigger;
         cleanup = true;
      }

      red_count = red_count-MAT_BLK_SZ;
      double * NTL_RESTRICT kpanelp = &M[kpanel][0];

      if (cleanup) {
         for (long r = kk*MAT_BLK_SZ; r < n*MAT_BLK_SZ; r++) 
            kpanelp[r] = rem((unsigned long)(long)kpanelp[r], p, red_struct);
      }

      for (long k = kk; k < k_max; k++) {

         long pos = -1;
         long pivot;
         long pivot_inv;

         for (long i = k; i < n; i++) {
            // NOTE: by using InvModStatus, this code will work
            // for prime-powers as well as primes
            pivot = rem((unsigned long)(long)kpanelp[i*MAT_BLK_SZ+(k-kk)], p, red_struct);
            if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
               pos = i;
               break;
            }
         }

         if (pos == -1) {
            clear(d);
            return;
         }

         double * NTL_RESTRICT y = &kpanelp[k*MAT_BLK_SZ];
         if (k != pos) {
            // swap rows pos and k
            double * NTL_RESTRICT x = &kpanelp[pos*MAT_BLK_SZ];
            for (long j = 0; j < MAT_BLK_SZ; j++) _ntl_swap(x[j], y[j]);
            
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;

            if (bp) _ntl_swap(bv[pos], bv[k]);
         }

         det = MulMod(det, pivot, p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 
            for (long j = 0; j < MAT_BLK_SZ; j++) {
               long t2 = rem((unsigned long)(long)y[j], p, red_struct);
               y[j] = MulModPrecon(t2, t1, p, t1pinv);
            }

            y[k-kk] = pivot_inv;

            if (bp) bv[k] = MulModPrecon(bv[k], t1, p, t1pinv);
         }

         for (long i = kk; i < n; i++) {
            if (i == k) continue; // skip row k

            double * NTL_RESTRICT x = &kpanelp[i*MAT_BLK_SZ];
            long t1 = rem((unsigned long)(long)x[k-kk], p, red_struct);
            t1 = NegateMod(t1, p);
            x[k-kk] = 0;
            if (t1 == 0) continue;

            // add t1 * row k to row i
            double ut1 = t1;
            muladd_interval(x, y, ut1, MAT_BLK_SZ);
            if (bp)
            {
               long t2 = MulMod(bv[k], t1, p);
               bv[i] = AddMod(bv[i], t2, p);
            }
         }
      }


      // finished processing current kpanel
      // next, reduce and apply to all other kpanels

      for (long r = kk*MAT_BLK_SZ; r < n*MAT_BLK_SZ; r++) 
         kpanelp[r] = rem((unsigned long)(long)kpanelp[r], p, red_struct);

      // special processing: subtract 1 off of diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = SubMod((long)kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);


      bool seq = double(npanels-(kpanel+1))*double(n)*double(MAT_BLK_SZ)*double(MAT_BLK_SZ) < PAR_THRESH;

      NTL_GEXEC_RANGE(seq, npanels-(kpanel+1), first, last)  
      NTL_IMPORT(p)
      NTL_IMPORT(n)
      NTL_IMPORT(red_struct)
      NTL_IMPORT(kpanel)
      NTL_IMPORT(kpanelp)
      NTL_IMPORT(kk)
      NTL_IMPORT(k_max)


      AlignedArray<double> buf_store;
      buf_store.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
      double *buf = &buf_store[0];

      for (long index = first; index < last; index++) {
         long jpanel = index + kpanel+1;

         double * NTL_RESTRICT jpanelp = &M[jpanel][0];

         if (cleanup) {
            for (long r = kk*MAT_BLK_SZ; r < n*MAT_BLK_SZ; r++) 
               jpanelp[r] = rem((unsigned long)(long)jpanelp[r], p, red_struct);
         }

         // perform swaps
         for (long k = kk; k < k_max; k++) {
            long pos = P[k];
            if (pos != k) {
               // swap rows pos and k
               double * NTL_RESTRICT pos_p = &jpanelp[pos*MAT_BLK_SZ];
               double * NTL_RESTRICT k_p = &jpanelp[k*MAT_BLK_SZ];
               for (long j = 0; j < MAT_BLK_SZ; j++)
                  _ntl_swap(pos_p[j], k_p[j]);
            }
         }

         // copy block number kpanel (the one on the diagonal)  into buf

         for (long i = 0; i < (k_max-kk)*MAT_BLK_SZ; i++)
            buf[i] = rem((unsigned long)(long)jpanelp[kk*MAT_BLK_SZ+i], p, red_struct);

         // jpanel += kpanel*buf

         muladd_all_by_32(kk, n, jpanelp, kpanelp, buf, k_max-kk);
      }
                  
      NTL_GEXEC_RANGE_END

      // special processing: add 1 back to the diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = AddMod((long)kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);

   }

   if (bp) {
      xp->SetLength(n);
      zz_p *X = xp->elts();

      for (long i = n-1; i >= 0; i--) {
         long t1 = 0;
         long start_panel = ((i+1)+MAT_BLK_SZ-1)/MAT_BLK_SZ;
         for (long jj = MAT_BLK_SZ*start_panel, panel = start_panel; 
             jj < n; jj += MAT_BLK_SZ, panel++) {
            long j_max = min(jj+MAT_BLK_SZ, n);
            double *row = &M[panel][i*MAT_BLK_SZ];
            for (long j = jj; j < j_max; j++) {
               long t0 = rem((unsigned long)(long)row[j-jj], p, red_struct);
               long t2 = MulMod(rep(X[j]), t0, p);
               t1 = AddMod(t1, t2, p);
            }
         }
         X[i].LoopHole() = SubMod(bv[i], t1, p);
      }
   }

   d.LoopHole() = det;

}

#endif


static
void blk_tri_L(zz_p& d, const mat_zz_p& A, const vec_zz_p *bp, 
               vec_zz_p *xp, bool trans, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("tri: nonsquare matrix");

   if (bp && bp->length() != n)
      LogicError("tri: dimension mismatch");

   if (bp && !xp)
      LogicError("tri: bad args");

   if (n == 0) {
      set(d);
      if (xp) xp->SetLength(0);
      return;
   }

   if (NTL_OVERFLOW(n, MAT_BLK_SZ, 0)) ResourceError("dimension too large");

   long npanels = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;
   
   Vec< UniqueArray<unsigned long> > M;
   M.SetLength(npanels);
   for (long panel = 0; panel < npanels; panel++) {
      M[panel].SetLength(n*MAT_BLK_SZ);
      unsigned long *panelp = &M[panel][0];

      for (long r = 0; r < n*MAT_BLK_SZ; r++) panelp[r] = 0;
   }

   if (trans) {
      // copy A transposed into panels
      for (long i = 0; i < n; i++) {
         const zz_p *row = &A[i][0];
         unsigned long *col = &M[i/MAT_BLK_SZ][i%MAT_BLK_SZ];
         for (long j = 0; j < n; j++) 
            col[j*MAT_BLK_SZ] = rep(row[j]);
      }
   }
   else {
      // copy A into panels
      for (long jj = 0, panel = 0; jj < n; jj += MAT_BLK_SZ, panel++) {
         long j_max = min(jj+MAT_BLK_SZ, n);
         unsigned long *panelp = &M[panel][0];

         for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
            const zz_p *ap = A[i].elts() + jj;

            for (long j = jj; j < j_max; j++)
               panelp[j-jj] = rep(ap[j-jj]);
         }
      }
   }

   Vec<long> bv;
   if (bp) conv(bv, *bp);
            
   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations


   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_reduce_struct red_struct = zz_p::red_struct();


   bool pivoting = false;

   unsigned long ured_trigger = 
      (~(0UL)-cast_unsigned(p-1))/(cast_unsigned(p-1)*cast_unsigned(p-1));
   // NOTE: corner case at p == 2: need unsigned long to prevent overflow

   long red_trigger = min(cast_unsigned(NTL_MAX_LONG), ured_trigger);

   long red_count = red_trigger;

   for (long kk = 0, kpanel = 0; kk < n; kk += MAT_BLK_SZ, kpanel++) {
      long k_max = min(kk+MAT_BLK_SZ, n);

      bool cleanup = false;

      if (red_count-MAT_BLK_SZ < 0) {
         red_count = red_trigger;
         cleanup = true;
      }

      red_count = red_count-MAT_BLK_SZ;
      unsigned long * NTL_RESTRICT kpanelp = &M[kpanel][0];

      if (cleanup) {
         for (long r = kk*MAT_BLK_SZ; r < n*MAT_BLK_SZ; r++) 
            kpanelp[r] = rem(kpanelp[r], p, red_struct);
      }

      for (long k = kk; k < k_max; k++) {

         long pos = -1;
         long pivot;
         long pivot_inv;

         for (long i = k; i < n; i++) {
            // NOTE: by using InvModStatus, this code will work
            // for prime-powers as well as primes
            pivot = rem(kpanelp[i*MAT_BLK_SZ+(k-kk)], p, red_struct);
            if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
               pos = i;
               break;
            }
         }

         if (pos == -1) {
            clear(d);
            return;
         }

         unsigned long * NTL_RESTRICT y = &kpanelp[k*MAT_BLK_SZ];
         if (k != pos) {
            // swap rows pos and k
            unsigned long * NTL_RESTRICT x = &kpanelp[pos*MAT_BLK_SZ];
            for (long j = 0; j < MAT_BLK_SZ; j++) _ntl_swap(x[j], y[j]);
            
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;

            if (bp) _ntl_swap(bv[pos], bv[k]);
         }

         det = MulMod(det, pivot, p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 
            for (long j = 0; j < MAT_BLK_SZ; j++) {
               long t2 = rem(y[j], p, red_struct);
               y[j] = MulModPrecon(t2, t1, p, t1pinv);
            }

            y[k-kk] = pivot_inv;

            if (bp) bv[k] = MulModPrecon(bv[k], t1, p, t1pinv);
         }

         for (long i = kk; i < n; i++) {
            if (i == k) continue; // skip row k

            unsigned long * NTL_RESTRICT x = &kpanelp[i*MAT_BLK_SZ];
            long t1 = rem(x[k-kk], p, red_struct);
            t1 = NegateMod(t1, p);
            x[k-kk] = 0;
            if (t1 == 0) continue;

            // add t1 * row k to row i
            unsigned long ut1 = t1;
            muladd_interval(x, y, ut1, MAT_BLK_SZ);
            if (bp)
            {
               long t2 = MulMod(bv[k], t1, p);
               bv[i] = AddMod(bv[i], t2, p);
            }
         }
      }


      // finished processing current kpanel
      // next, reduce and apply to all other kpanels

      for (long r = kk*MAT_BLK_SZ; r < n*MAT_BLK_SZ; r++) 
         kpanelp[r] = rem(kpanelp[r], p, red_struct);

      // special processing: subtract 1 off of diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = SubMod((long)kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);


      bool seq = double(npanels-(kpanel+1))*double(n)*double(MAT_BLK_SZ)*double(MAT_BLK_SZ) < PAR_THRESH;
      NTL_GEXEC_RANGE(seq, npanels-(kpanel+1), first, last)  
      NTL_IMPORT(p)
      NTL_IMPORT(n)
      NTL_IMPORT(red_struct)
      NTL_IMPORT(kpanel)
      NTL_IMPORT(kpanelp)
      NTL_IMPORT(kk)
      NTL_IMPORT(k_max)


      UniqueArray<unsigned long> buf_store;
      buf_store.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
      unsigned long *buf = &buf_store[0];

      for (long index = first; index < last; index++) {
         long jpanel = index + kpanel+1;

         unsigned long * NTL_RESTRICT jpanelp = &M[jpanel][0];

         if (cleanup) {
            for (long r = kk*MAT_BLK_SZ; r < n*MAT_BLK_SZ; r++) 
               jpanelp[r] = rem(jpanelp[r], p, red_struct);
         }

         // perform swaps
         for (long k = kk; k < k_max; k++) {
            long pos = P[k];
            if (pos != k) {
               // swap rows pos and k
               unsigned long * NTL_RESTRICT pos_p = &jpanelp[pos*MAT_BLK_SZ];
               unsigned long * NTL_RESTRICT k_p = &jpanelp[k*MAT_BLK_SZ];
               for (long j = 0; j < MAT_BLK_SZ; j++)
                  _ntl_swap(pos_p[j], k_p[j]);
            }
         }

         // copy block number kpanel (the one on the diagonal)  into buf
         // here, we transpose it

         for (long k = kk; k < k_max; k++) 
            for (long j = 0; j < MAT_BLK_SZ; j++)
               buf[j*MAT_BLK_SZ + (k-kk)] = 
                  rem(jpanelp[k*MAT_BLK_SZ+j], p, red_struct);

         // jpanel += kpanel*buf

         muladd_all_by_32(kk, n, jpanelp, kpanelp, buf, k_max-kk);
      }
                  
      NTL_GEXEC_RANGE_END

      // special processing: add 1 back to the diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = AddMod((long)kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);

   }

   if (bp) {
      xp->SetLength(n);
      zz_p *X = xp->elts();

      for (long i = n-1; i >= 0; i--) {
         long t1 = 0;
         long start_panel = ((i+1)+MAT_BLK_SZ-1)/MAT_BLK_SZ;
         for (long jj = MAT_BLK_SZ*start_panel, panel = start_panel; 
             jj < n; jj += MAT_BLK_SZ, panel++) {
            long j_max = min(jj+MAT_BLK_SZ, n);
            unsigned long *row = &M[panel][i*MAT_BLK_SZ];
            for (long j = jj; j < j_max; j++) {
               long t0 = rem(row[j-jj], p, red_struct);
               long t2 = MulMod(rep(X[j]), t0, p);
               t1 = AddMod(t1, t2, p);
            }
         }
         X[i].LoopHole() = SubMod(bv[i], t1, p);
      }
   }

   d.LoopHole() = det;

}


static
void blk_tri_LL(zz_p& d, const mat_zz_p& A, const vec_zz_p *bp, 
               vec_zz_p *xp, bool trans, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("tri: nonsquare matrix");

   if (bp && bp->length() != n)
      LogicError("tri: dimension mismatch");

   if (bp && !xp)
      LogicError("tri: bad args");

   if (n == 0) {
      set(d);
      if (xp) xp->SetLength(0);
      return;
   }

   if (NTL_OVERFLOW(n, MAT_BLK_SZ, 0)) ResourceError("dimension too large");

   long npanels = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;
   
   Vec< UniqueArray<long> > M;
   M.SetLength(npanels);
   for (long panel = 0; panel < npanels; panel++) {
      M[panel].SetLength(n*MAT_BLK_SZ);
      long *panelp = &M[panel][0];

      for (long r = 0; r < n*MAT_BLK_SZ; r++) panelp[r] = 0;
   }

   if (trans) {
      // copy A transposed into panels
      for (long i = 0; i < n; i++) {
         const zz_p *row = &A[i][0];
         long *col = &M[i/MAT_BLK_SZ][i%MAT_BLK_SZ];
         for (long j = 0; j < n; j++) 
            col[j*MAT_BLK_SZ] = rep(row[j]);
      }
   }
   else {
      // copy A into panels
      for (long jj = 0, panel = 0; jj < n; jj += MAT_BLK_SZ, panel++) {
         long j_max = min(jj+MAT_BLK_SZ, n);
         long *panelp = &M[panel][0];

         for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
            const zz_p *ap = A[i].elts() + jj;

            for (long j = jj; j < j_max; j++)
               panelp[j-jj] = rep(ap[j-jj]);
         }
      }
   }

   Vec<long> bv;
   if (bp) conv(bv, *bp);
            
   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations


   long det;
   det = 1;

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_ll_reduce_struct ll_red_struct = zz_p::ll_red_struct();


   bool pivoting = false;

   for (long kk = 0, kpanel = 0; kk < n; kk += MAT_BLK_SZ, kpanel++) {
      long k_max = min(kk+MAT_BLK_SZ, n);

      long * NTL_RESTRICT kpanelp = &M[kpanel][0];

      for (long k = kk; k < k_max; k++) {

         long pos = -1;
         long pivot;
         long pivot_inv;

         for (long i = k; i < n; i++) {
            // NOTE: by using InvModStatus, this code will work
            // for prime-powers as well as primes
            pivot = kpanelp[i*MAT_BLK_SZ+(k-kk)];
            if (pivot != 0 && !relaxed_InvModStatus(pivot_inv, pivot, p, relax)) {
               pos = i;
               break;
            }
         }

         if (pos == -1) {
            clear(d);
            return;
         }

         long * NTL_RESTRICT y = &kpanelp[k*MAT_BLK_SZ];
         if (k != pos) {
            // swap rows pos and k
            long * NTL_RESTRICT x = &kpanelp[pos*MAT_BLK_SZ];
            for (long j = 0; j < MAT_BLK_SZ; j++) _ntl_swap(x[j], y[j]);
            
            det = NegateMod(det, p);
            P[k] = pos;
            pivoting = true;

            if (bp) _ntl_swap(bv[pos], bv[k]);
         }

         det = MulMod(det, pivot, p);

         {
            // multiply row k by pivot_inv
            long t1 = pivot_inv;
            mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 
            for (long j = 0; j < MAT_BLK_SZ; j++) {
               y[j] = MulModPrecon(y[j], t1, p, t1pinv);
            }

            y[k-kk] = pivot_inv;

            if (bp) bv[k] = MulModPrecon(bv[k], t1, p, t1pinv);
         }

         for (long i = kk; i < n; i++) {
            if (i == k) continue; // skip row k

            long * NTL_RESTRICT x = &kpanelp[i*MAT_BLK_SZ];
            long t1 = x[k-kk];
            t1 = NegateMod(t1, p);
            x[k-kk] = 0;
            if (t1 == 0) continue;

            // add t1 * row k to row i
            long ut1 = t1;
            muladd_interval(x, y, ut1, MAT_BLK_SZ, p, pinv);
            if (bp)
            {
               long t2 = MulMod(bv[k], t1, p);
               bv[i] = AddMod(bv[i], t2, p);
            }
         }
      }


      // finished processing current kpanel
      // next, reduce and apply to all other kpanels

      // special processing: subtract 1 off of diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = SubMod((long)kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);


      bool seq = double(npanels-(kpanel+1))*double(n)*double(MAT_BLK_SZ)*double(MAT_BLK_SZ) < PAR_THRESH;
      NTL_GEXEC_RANGE(seq, npanels-(kpanel+1), first, last)  
      NTL_IMPORT(p)
      NTL_IMPORT(n)
      NTL_IMPORT(ll_red_struct)
      NTL_IMPORT(kpanel)
      NTL_IMPORT(kpanelp)
      NTL_IMPORT(kk)
      NTL_IMPORT(k_max)


      UniqueArray<long> buf_store;
      buf_store.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
      long *buf = &buf_store[0];

      for (long index = first; index < last; index++) {
         long jpanel = index + kpanel+1;

         long * NTL_RESTRICT jpanelp = &M[jpanel][0];

         // perform swaps
         for (long k = kk; k < k_max; k++) {
            long pos = P[k];
            if (pos != k) {
               // swap rows pos and k
               long * NTL_RESTRICT pos_p = &jpanelp[pos*MAT_BLK_SZ];
               long * NTL_RESTRICT k_p = &jpanelp[k*MAT_BLK_SZ];
               for (long j = 0; j < MAT_BLK_SZ; j++)
                  _ntl_swap(pos_p[j], k_p[j]);
            }
         }

         // copy block number kpanel (the one on the diagonal)  into buf
         // here, we transpose it

         for (long k = kk; k < k_max; k++) 
            for (long j = 0; j < MAT_BLK_SZ; j++)
               buf[j*MAT_BLK_SZ + (k-kk)] = jpanelp[k*MAT_BLK_SZ+j];

         // jpanel += kpanel*buf

         muladd_all_by_32(kk, n, jpanelp, kpanelp, buf, k_max-kk, p, ll_red_struct);
      }
                  
      NTL_GEXEC_RANGE_END

      // special processing: add 1 back to the diangonal

      for (long k = kk; k < k_max; k++)
         kpanelp[k*MAT_BLK_SZ+(k-kk)] = AddMod((long)kpanelp[k*MAT_BLK_SZ+(k-kk)], 1, p);

   }

   if (bp) {
      xp->SetLength(n);
      zz_p *X = xp->elts();

      for (long i = n-1; i >= 0; i--) {
         long t1 = 0;
         long start_panel = ((i+1)+MAT_BLK_SZ-1)/MAT_BLK_SZ;
         for (long jj = MAT_BLK_SZ*start_panel, panel = start_panel; 
             jj < n; jj += MAT_BLK_SZ, panel++) {
            long j_max = min(jj+MAT_BLK_SZ, n);
            long *row = &M[panel][i*MAT_BLK_SZ];
            for (long j = jj; j < j_max; j++) {
               long t0 = row[j-jj];
               long t2 = MulMod(rep(X[j]), t0, p);
               t1 = AddMod(t1, t2, p);
            }
         }
         X[i].LoopHole() = SubMod(bv[i], t1, p);
      }
   }

   d.LoopHole() = det;

}



#endif



static
void tri(zz_p& d, const mat_zz_p& A, const vec_zz_p *bp, 
               vec_zz_p *xp, bool trans, bool relax)
{
   long n = A.NumRows();

   if (A.NumCols() != n)
      LogicError("inv: nonsquare matrix");

   if (bp && bp->length() != n)
      LogicError("tri: dimension mismatch");

   if (bp && !xp)
      LogicError("tri: bad args");

#ifndef NTL_HAVE_LL_TYPE

   basic_tri(d, A, bp, xp, trans, relax);

#else

   long p = zz_p::modulus();

   if (n < 16) {
      //cerr << "basic_tri\n";
      basic_tri(d, A, bp, xp, trans, relax);
   }
   else if (n/MAT_BLK_SZ < 4) {
      long V = 64;

#ifdef NTL_HAVE_AVX
      if (p-1 <= MAX_DBL_INT &&
          V <= (MAX_DBL_INT-(p-1))/(p-1) &&
          V*(p-1) <= (MAX_DBL_INT-(p-1))/(p-1)) {

         //cerr << "alt_tri_DD\n";
         alt_tri_DD(d, A, bp, xp, trans, relax);
      }
      else 
#endif
           if (cast_unsigned(V) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1) &&
               cast_unsigned(V)*cast_unsigned(p-1) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1))  {

         //cerr << "alt_tri_L\n";
         alt_tri_L(d, A, bp, xp, trans, relax);

      }
      else {
  
         //cerr << "basic_tri\n";
         basic_tri(d, A, bp, xp, trans, relax);
      }
   }
   else {
      long V = 4*MAT_BLK_SZ;

#ifdef NTL_HAVE_AVX
      if (p-1 <= MAX_DBL_INT &&
          V <= (MAX_DBL_INT-(p-1))/(p-1) &&
          V*(p-1) <= (MAX_DBL_INT-(p-1))/(p-1)) {

         //cerr << "blk_tri_DD\n";
         blk_tri_DD(d, A, bp, xp, trans, relax);
      }
      else 
#endif
           if (cast_unsigned(V) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1) &&
               cast_unsigned(V)*cast_unsigned(p-1) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1))  {

         //cerr << "blk_tri_L\n";
         blk_tri_L(d, A, bp, xp, trans, relax);

      }
      else {
  
         //cerr << "blk_tri_LL\n";
         blk_tri_LL(d, A, bp, xp, trans, relax);
      }
   
   }

#endif



}



void relaxed_determinant(zz_p& d, const mat_zz_p& A, bool relax)
{
   tri(d, A, 0, 0, false, relax);
}


void relaxed_solve(zz_p& d, vec_zz_p& x, 
           const mat_zz_p& A, const vec_zz_p& b, bool relax)
{
   tri(d, A, &b, &x, true, relax);
}

void relaxed_solve(zz_p& d, const mat_zz_p& A, vec_zz_p& x, const vec_zz_p& b, bool relax)
{
   tri(d, A, &b, &x, false, relax);
}

// ******************************************************************
// 
// new image and kernel routines
//
// ******************************************************************


static
long elim_basic(const mat_zz_p& A, mat_zz_p *im, mat_zz_p *ker,
                long w, bool full)
{
   long n = A.NumRows();
   long m = A.NumCols();

   if (w < 0 || w > m) LogicError("elim: bad args");

   // take care of corner cases
   if (n == 0) {
      if (im) im->SetDims(0, m);
      if (ker) ker->SetDims(0, 0);
      return 0;
   }

   if (w == 0) {
      if (im) {
         if (full)
            (*im) = A;
         else
            im->SetDims(0, m);
      }
      if (ker) ident(*ker, n);
      return 0;
   }

   Mat<long> M;
   conv(M, A);

   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations

   Vec<long> pcol;
   pcol.SetLength(n);
   // pcol[i] records pivot columns for row i

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();

   bool pivoting = false;

   long r = 0;

   for (long k = 0; k < w; k++) {
      long pos = -1;
      long pivot_inv;
      for (long i = r; i < n; i++) {
         long pivot = M[i][k];
         if (pivot != 0) {
            pivot_inv = InvMod(pivot, p);
            pos = i;
            break;
         }
      }

      if (pos == -1) 
         continue;

      if (r != pos) {
         swap(M[pos], M[r]);
         P[r] = pos;
         pivoting = true;
      }

      bool seq = double(n-r)*double(m-k) < PAR_THRESH;

      NTL_GEXEC_RANGE(seq, n-(r+1), first, last)  
      NTL_IMPORT(p)
      NTL_IMPORT(n)
      NTL_IMPORT(k)
      NTL_IMPORT(r)
      long * NTL_RESTRICT y = &M[r][0]; 

      for (long ii = first; ii < last; ii++) {
         long i = ii + r+1;

         long * NTL_RESTRICT x = &M[i][0]; 
         long t1 = x[k];
         t1 = MulMod(t1, pivot_inv, p);
         t1 = NegateMod(t1, p);
         x[k] = t1;
         if (t1 == 0) continue;

         // add t1 * row r to row i
         mulmod_precon_t t1pinv = PrepMulModPrecon(t1, p, pinv); 

         for (long j = k+1; j < m; j++) {
            long t2 = MulModPrecon(y[j], t1, p, t1pinv);
            x[j] = AddMod(x[j], t2, p);
         }
      }
      NTL_GEXEC_RANGE_END

      pcol[r] = k;
      r++;
   }

   if (im) {
      mat_zz_p& Im = *im;;
      if (full)
         Im.SetDims(n, m);
      else
         Im.SetDims(r, m);

      for (long i = 0; i < r; i++) {
         long pc = pcol[i];
         for (long j = 0; j < pc; j++) Im[i][j].LoopHole() = 0;
         for (long j = pc; j < m; j++) Im[i][j].LoopHole() = M[i][j];
      }

      if (full) {
         for (long i = r; i < n; i++) {
            for (long j = 0; j < w; j++) Im[i][j].LoopHole() = 0;
            for (long j = w; j < m; j++) Im[i][j].LoopHole() = M[i][j];
         }
      }
   }

   if (ker) {

      if (n == r) {
         mat_zz_p& Ker = *ker;
         Ker.SetDims(n-r, n);
      }
      else {
	 Mat<long> colbuf;
	 colbuf.SetDims(r, n);

         for (long k = 0; k < r; k++) {
	    long pc = pcol[k];
	    for (long i = k+1; i < n; i++) colbuf[k][i] = M[i][pc];
         }

         M.kill();

	 Mat<long> X;
	 X.SetDims(n-r, r);

         bool seq = double(n-r)*double(r)*double(r)/2 < PAR_THRESH;
	 NTL_GEXEC_RANGE(seq, n-r, first, last)
	 NTL_IMPORT(p)
	 NTL_IMPORT(r)

	 for (long i = first; i < last; i++) {
	    long *Xi = &X[i][0];

	    for (long k = r-1; k >= 0; k--) {
	       long *cvecp = &colbuf[k][0];
         
	       long acc = cvecp[i+r];
	       for (long j = k+1; j < r; j++) { 
		  acc = AddMod( acc,  MulMod(Xi[j], cvecp[j], p), p );
	       }
	       Xi[k] = acc;
	    }

	 }

	 NTL_GEXEC_RANGE_END

	 mat_zz_p& Ker = *ker;
	 Ker.SetDims(n-r, n);
	 for (long i = 0; i < n-r; i++) {
	    for (long j = 0; j < r; j++) Ker[i][j].LoopHole() = X[i][j];
	    for (long j = r; j < n; j++) Ker[i][j].LoopHole() = 0;
	    Ker[i][r+i].LoopHole() = 1;
	 }

	 if (pivoting) {
	    for (long i = 0; i < n-r; i++) {
	       zz_p *x = Ker[i].elts();

	       for (long k = n-1; k >= 0; k--) {
		  long pos = P[k];
		  if (pos != k) swap(x[pos], x[k]);
	       }
	    }
	 }
      }
   }

   return r;
}

#ifdef NTL_HAVE_LL_TYPE


#ifdef NTL_HAVE_AVX


static inline
void CopyBlock(double *dst_ptr, long dst_blk, const double *src_ptr, long src_blk, long src_limit)
{
   long src_row = src_blk*MAT_BLK_SZ;
   long dst_row = dst_blk*MAT_BLK_SZ;

   long nrows = min(MAT_BLK_SZ, src_limit - src_row);

   for (long i = 0; i < nrows; i++) 
      for (long j = 0; j < MAT_BLK_SZ; j++)
         dst_ptr[(dst_row + i)*MAT_BLK_SZ + j] = src_ptr[(src_row + i)*MAT_BLK_SZ + j];

   for (long i = nrows; i < MAT_BLK_SZ; i++) 
      for (long j = 0; j < MAT_BLK_SZ; j++)
         dst_ptr[(dst_row + i)*MAT_BLK_SZ + j] = 0;

}

static inline
void CopyBlock(double *dst_ptr, long dst_blk, const double *src_ptr, long src_blk)
{
   long src_row = src_blk*MAT_BLK_SZ;
   long dst_row = dst_blk*MAT_BLK_SZ;

   long nrows = MAT_BLK_SZ;

   for (long i = 0; i < nrows; i++) 
      for (long j = 0; j < MAT_BLK_SZ; j++)
         dst_ptr[(dst_row + i)*MAT_BLK_SZ + j] = src_ptr[(src_row + i)*MAT_BLK_SZ + j];
}

static inline
void SwapOneRow(double *panelp, long i, long pos)
{
   double * NTL_RESTRICT pos_p = &panelp[pos*MAT_BLK_SZ];
   double * NTL_RESTRICT i_p = &panelp[i*MAT_BLK_SZ];
   for (long j = 0; j < MAT_BLK_SZ; j++)
      _ntl_swap(pos_p[j], i_p[j]);
}

static inline
void ApplySwaps(double *panelp, long start, long end, const Vec<long>& P)
{
   for (long i = start; i < end; i++) {
      long pos = P[i];
      if (pos != i) 
         SwapOneRow(panelp, i, pos);
   }
}


static inline
void MulAddBlock(double *x, const double *y, const double *z)
{
   // x += y*z 
   muladd_all_by_32(0, MAT_BLK_SZ, x, y, z, MAT_BLK_SZ);
}


static
long elim_blk_DD(const mat_zz_p& A, mat_zz_p *im, mat_zz_p *ker,
                 long w, bool full)
{
   long n = A.NumRows();
   long m = A.NumCols();

   if (w < 0 || w > m) LogicError("elim: bad args");

   // take care of corner cases
   if (n == 0) {
      if (im) im->SetDims(0, m);
      if (ker) ker->SetDims(0, 0);
      return 0;
   }

   if (w == 0) {
      if (im) {
         if (full)
            (*im) = A;
         else
            im->SetDims(0, m);
      }
      if (ker) ident(*ker, n);
      return 0;
   }

   if (NTL_OVERFLOW(n, MAT_BLK_SZ, 0)) ResourceError("dimension too large");
   if (NTL_OVERFLOW(m, MAT_BLK_SZ, 0)) ResourceError("dimension too large");

   long npanels = (m+MAT_BLK_SZ-1)/MAT_BLK_SZ;
   

   Vec< AlignedArray<double> > M;
   M.SetLength(npanels);
   for (long panel = 0; panel < npanels; panel++) {
      M[panel].SetLength(n*MAT_BLK_SZ);
      double *panelp = &M[panel][0];

      for (long h = 0; h < n*MAT_BLK_SZ; h++) panelp[h] = 0;
   }

   // copy A into panels
   for (long jj = 0, panel = 0; jj < m; jj += MAT_BLK_SZ, panel++) {
      long j_max = min(jj+MAT_BLK_SZ, m);
      double *panelp = &M[panel][0];

      for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
         const zz_p *ap = A[i].elts() + jj;

         for (long j = jj; j < j_max; j++)
            panelp[j-jj] = rep(ap[j-jj]);
      }
   }

   AlignedArray<double> aux_panel_store;
   aux_panel_store.SetLength(n*MAT_BLK_SZ);
   double * NTL_RESTRICT aux_panel = &aux_panel_store[0];


   AlignedArray<double> buf_store1;
   buf_store1.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
   double *buf1 = &buf_store1[0];

   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations

   Vec<long> pcol;
   pcol.SetLength(n);
   // pcol[i] records pivot columns for row i

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_reduce_struct red_struct = zz_p::red_struct();

   bool pivoting = false;

   long red_trigger = (MAX_DBL_INT-(p-1))/((p-1)*(p-1));
   long red_count = red_trigger;

   long r = 0, rr = 0, k = 0, kk = 0;
   long rpanel = 0, kpanel = 0;

   while (k < w) {

      if (r > rr && ker) { 
         // we have a panel from a previous iteration
         // we store enough of it to facilitate the kernel
         // computation later. At this point, we have 
         // r == rr+INV_BLK_SIZE, and it suffices to store
         // rows [r..n) into M[rpanel], and this will not
         // overwrite anything useful in M[rpanel]
         
         double *panelp = &M[rpanel][0];
         for (long h = r*MAT_BLK_SZ; h < n*MAT_BLK_SZ; h++) {
            panelp[h] = aux_panel[h];
         }

         rpanel++;
      }

      rr = r;

      for (long h = 0; h < n*MAT_BLK_SZ; h++) aux_panel[h] = 0;

      bool cleanup = false;

      if (red_count-MAT_BLK_SZ < 0) {
         red_count = red_trigger;
         cleanup = true;
      }

      red_count = red_count-MAT_BLK_SZ;

      for (; r < rr+MAT_BLK_SZ && k < w; k++) { // panel incomplete

         if (k == kk+MAT_BLK_SZ) { // start new kpanel
            kk = k;
            kpanel++;
         }

         double * NTL_RESTRICT kpanelp = &M[kpanel][0];

         if (k == kk) { // a fresh kpanel -- special processing

            if (cleanup) {
               for (long h = 0; h < n*MAT_BLK_SZ; h++)
                  kpanelp[h] = rem((unsigned long)(long)kpanelp[h], p, red_struct);
            }

            if (r > rr) {


               // apply current sequence of permutations

               ApplySwaps(kpanelp, rr, r, P);

	       // clean aux_panel
	       for (long h = 0; h < n*MAT_BLK_SZ; h++)
		  aux_panel[h] = rem((unsigned long)(long)aux_panel[h], p, red_struct);

               // copy rows [rr..r) of kpanel into buf1
               for (long i = 0; i < (r-rr)*MAT_BLK_SZ; i++)
                  buf1[i] = rem((unsigned long)(long)kpanelp[rr*MAT_BLK_SZ+i], p, red_struct);

               // kpanel[rr..n) += aux_panel[rr..n)*buf1

               muladd_all_by_32(rr, n, kpanelp, aux_panel, buf1, r-rr);
            }
         }

         long pos = -1;
         long pivot;
         long pivot_inv;
         for (long i = r; i < n; i++) {
            pivot = rem((unsigned long)(long)kpanelp[i*MAT_BLK_SZ+(k-kk)], p, red_struct);
            kpanelp[i*MAT_BLK_SZ+(k-kk)] = pivot;

            if (pivot != 0) {
               pivot_inv = InvMod(pivot, p);
               pos = i;
               break;
            }
         }

         if (pos == -1) {
            continue;
         }

         double * NTL_RESTRICT y = &kpanelp[r*MAT_BLK_SZ];
         double * NTL_RESTRICT y1 = &aux_panel[r*MAT_BLK_SZ];
         if (r != pos) {
            // swap rows pos and r
            double * NTL_RESTRICT x = &kpanelp[pos*MAT_BLK_SZ];
            double * NTL_RESTRICT x1 = &aux_panel[pos*MAT_BLK_SZ];

            for (long j = k-kk; j < MAT_BLK_SZ; j++) _ntl_swap(x[j], y[j]);
            for (long j = 0; j < r-rr; j++) _ntl_swap(x1[j], y1[j]);
            
            P[r] = pos;
            pivoting = true;
         }

         // clean up row r of kpanel and aux_panel
         for (long j = k-kk; j < MAT_BLK_SZ; j++) 
            y[j] = rem((unsigned long)(long)y[j], p, red_struct);
         for (long j = 0; j < r-rr; j++) 
            y1[j] = rem((unsigned long)(long)y1[j], p, red_struct);

         // clear column
         for (long i = r+1; i < n; i++) {
            double * NTL_RESTRICT x = &kpanelp[i*MAT_BLK_SZ];
            double * NTL_RESTRICT x1 = &aux_panel[i*MAT_BLK_SZ];
            long t1 = rem((unsigned long)(long)x[k-kk], p, red_struct);
            t1 = MulMod(t1, pivot_inv, p);
            t1 = NegateMod(t1, p);
            x[k-kk] = 0;
            x1[r-rr] = t1;
            if (t1 == 0) continue;

            // add t1 * row r to row i
            double ut1 = t1;

            for (long j = k-kk+1; j < MAT_BLK_SZ; j++) 
               x[j] += y[j]*ut1;
            for (long j = 0; j < r-rr; j++) 
               x1[j] += y1[j]*ut1;
         }

         pcol[r] = k;
         r++;
      }

      if (r > rr) {

         // we have a panel 

         // clean it up
         for (long h = 0; h < n*MAT_BLK_SZ; h++)
            aux_panel[h] = rem((unsigned long)(long)aux_panel[h], p, red_struct);

         bool seq = 
            double(npanels-(kpanel+1))*double(n-rr)*double(r-rr)*double(MAT_BLK_SZ) < PAR_THRESH;

         // apply aux_panel to remaining panels: [kpanel+1..npanels)
         NTL_GEXEC_RANGE(seq, npanels-(kpanel+1), first, last)  
         NTL_IMPORT(p)
         NTL_IMPORT(n)
         NTL_IMPORT(red_struct)
         NTL_IMPORT(aux_panel)
         NTL_IMPORT(rr)
         NTL_IMPORT(r)


         AlignedArray<double> buf_store;
         buf_store.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
         double *buf = &buf_store[0];


         for (long index = first; index < last; index++) {
            long jpanel = index + kpanel+1;

            double * NTL_RESTRICT jpanelp = &M[jpanel][0];

            if (cleanup) {
               for (long h = 0; h < n*MAT_BLK_SZ; h++) 
                  jpanelp[h] = rem((unsigned long)(long)jpanelp[h], p, red_struct);
            }

            // perform swaps
            ApplySwaps(jpanelp, rr, r, P);

            // copy rows [rr..r) of jpanel into buf
            for (long i = 0; i < (r-rr)*MAT_BLK_SZ; i++)
               buf[i] = rem((unsigned long)(long)jpanelp[rr*MAT_BLK_SZ+i], p, red_struct);

            // jpanel[rr..n) += aux_panel[rr..n)*buf

            muladd_all_by_32(rr, n, jpanelp, aux_panel, buf, r-rr);
         }
                     
         NTL_GEXEC_RANGE_END

      }

   }

   if (im) {
      mat_zz_p& Im = *im;;
      if (full)
         Im.SetDims(n, m);
      else
         Im.SetDims(r, m);

      for (long i = 0; i < r; i++) {
         long pc = pcol[i];
         for (long j = 0; j < pc; j++) Im[i][j].LoopHole() = 0;
         for (long j = pc; j < m; j++) {
            double t0 = M[j/MAT_BLK_SZ][i*MAT_BLK_SZ+(j%MAT_BLK_SZ)];
            Im[i][j].LoopHole() = rem((unsigned long)(long)t0, p, red_struct);
         }
      }

      if (full) {
	 for (long i = r; i < n; i++) {
	    for (long j = 0; j < w; j++) Im[i][j].LoopHole() = 0;
	    for (long j = w; j < m; j++) {
	       double t0 = M[j/MAT_BLK_SZ][i*MAT_BLK_SZ+(j%MAT_BLK_SZ)];
	       Im[i][j].LoopHole() = rem((unsigned long)(long)t0, p, red_struct);
	    }
	 }
      }
   }

   if (ker) {
      mat_zz_p& Ker = *ker;
      Ker.SetDims(n-r, n);
      if (r < n) {

	 long start_block = r/MAT_BLK_SZ;
	 long end_block = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;
	 long vblocks = end_block-start_block;
	 long hblocks = (r+MAT_BLK_SZ-1)/MAT_BLK_SZ;

	 Vec< AlignedArray<double> > kerbuf;
	 kerbuf.SetLength(vblocks);
	 for (long i = 0; i < vblocks; i++) 
	    kerbuf[i].SetLength(hblocks*MAT_BLK_SZ*MAT_BLK_SZ);

	 long colblocks = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;

	 // if r > rr, we have a panel sitting in 
	 // aux_panel, which may or may not be a full panel

         double *initial_panel = 0;
         if (r > rr) {
            initial_panel = aux_panel;
         }
         else {
            initial_panel = &M[hblocks-1][0];
         }

         for (long vb = start_block; vb < end_block; vb++) 
            CopyBlock(&kerbuf[vb-start_block][0], hblocks-1, initial_panel, vb, n);

         for (long hb = hblocks-2; hb >= 0; hb--) {

            ApplySwaps(&M[hb][0], (hb+1)*MAT_BLK_SZ, r, P);

            for (long b = hb+1; b < end_block; b++)
               CopyBlock(&M[hb][0], b-1, &M[hb][0], b, n);
         }

         bool seq = double(n-r)*double(r)*double(r)/2 < PAR_THRESH;


	 NTL_GEXEC_RANGE(seq, end_block-start_block, first, last)
	 NTL_IMPORT(p)
	 NTL_IMPORT(red_struct)
	 NTL_IMPORT(hblocks)

	 for (long index = first; index < last; index++) {
	    long vb = index + start_block;
	    double *kerbufp = &kerbuf[vb-start_block][0];

	    for (long hb = hblocks-2; hb >= 0; hb--) {
	       double *colbuf = &M[hb][0];
	       double *acc = &kerbufp[hb*MAT_BLK_SZ*MAT_BLK_SZ]; 

	       CopyBlock(acc, 0, colbuf, vb-1);

	       long red_trigger = (MAX_DBL_INT-(p-1))/((p-1)*(p-1));
	       long red_count = red_trigger;
     
	       for (long b = hb+1; b < hblocks; b++) { 

		  if (red_count-MAT_BLK_SZ < 0) {
		     red_count = red_trigger;
		     for (long h = 0; h < MAT_BLK_SZ*MAT_BLK_SZ; h++)
			acc[h] = rem((unsigned long)(long)acc[h], p, red_struct);

		  }
		  red_count = red_count-MAT_BLK_SZ;

		  MulAddBlock(acc, &kerbufp[b*MAT_BLK_SZ*MAT_BLK_SZ],
				   &colbuf[(b-1)*MAT_BLK_SZ*MAT_BLK_SZ]);
	       }

	       for (long h = 0; h < MAT_BLK_SZ*MAT_BLK_SZ; h++)
		  acc[h] = rem((unsigned long)(long)acc[h], p, red_struct);
	    }
         }

	 NTL_GEXEC_RANGE_END

         for (long i = r; i < n; i++) {

            double *kerbufp = &kerbuf[(i/MAT_BLK_SZ)-start_block][0];

            for (long j = 0; j < r; j++) {
               double t0 = 
                  kerbufp[(j/MAT_BLK_SZ)*MAT_BLK_SZ*MAT_BLK_SZ+
                          (i%MAT_BLK_SZ)*MAT_BLK_SZ+(j%MAT_BLK_SZ)];
          
               Ker[i-r][j].LoopHole() = long(t0);
            }
         }

         for (long i = 0; i < n-r; i++) {
            for (long j = 0; j < n-r; j++) {
               Ker[i][j+r].LoopHole() = 0;
            }
            Ker[i][i+r].LoopHole() = 1;
         }

	 if (pivoting) {
	    for (long i = 0; i < n-r; i++) {
	       zz_p *x = Ker[i].elts();

	       for (long k = n-1; k >= 0; k--) {
		  long pos = P[k];
		  if (pos != k) swap(x[pos], x[k]);
	       }
	    }
	 }
      }
   }

   return r;

}

#endif



static inline
void CopyBlock(unsigned long *dst_ptr, long dst_blk, const unsigned long *src_ptr, long src_blk, long src_limit)
{
   long src_row = src_blk*MAT_BLK_SZ;
   long dst_row = dst_blk*MAT_BLK_SZ;

   long nrows = min(MAT_BLK_SZ, src_limit - src_row);

   for (long i = 0; i < nrows; i++) 
      for (long j = 0; j < MAT_BLK_SZ; j++)
         dst_ptr[(dst_row + i)*MAT_BLK_SZ + j] = src_ptr[(src_row + i)*MAT_BLK_SZ + j];

   for (long i = nrows; i < MAT_BLK_SZ; i++) 
      for (long j = 0; j < MAT_BLK_SZ; j++)
         dst_ptr[(dst_row + i)*MAT_BLK_SZ + j] = 0;

}

static inline
void CopyBlock(unsigned long *dst_ptr, long dst_blk, const unsigned long *src_ptr, long src_blk)
{
   long src_row = src_blk*MAT_BLK_SZ;
   long dst_row = dst_blk*MAT_BLK_SZ;

   long nrows = MAT_BLK_SZ;

   for (long i = 0; i < nrows; i++) 
      for (long j = 0; j < MAT_BLK_SZ; j++)
         dst_ptr[(dst_row + i)*MAT_BLK_SZ + j] = src_ptr[(src_row + i)*MAT_BLK_SZ + j];
}

static inline
void TransposeBlock(unsigned long *dst_ptr, long dst_blk)
{
   dst_ptr += dst_blk*MAT_BLK_SZ*MAT_BLK_SZ;

   for (long i = 0; i < MAT_BLK_SZ; i++)
      for (long j = 0; j < i; j++)
         _ntl_swap(dst_ptr[i*MAT_BLK_SZ+j], dst_ptr[i+j*MAT_BLK_SZ]);
}

static inline
void SwapOneRow(unsigned long *panelp, long i, long pos)
{
   unsigned long * NTL_RESTRICT pos_p = &panelp[pos*MAT_BLK_SZ];
   unsigned long * NTL_RESTRICT i_p = &panelp[i*MAT_BLK_SZ];
   for (long j = 0; j < MAT_BLK_SZ; j++)
      _ntl_swap(pos_p[j], i_p[j]);
}

static inline
void ApplySwaps(unsigned long *panelp, long start, long end, const Vec<long>& P)
{
   for (long i = start; i < end; i++) {
      long pos = P[i];
      if (pos != i) 
         SwapOneRow(panelp, i, pos);
   }
}


static inline
void MulAddBlock(unsigned long *x, const unsigned long *y, const unsigned long *z)
{
   // x += y*z 

   muladd_all_by_32(0, MAT_BLK_SZ, x, y, z, MAT_BLK_SZ);
}


static
long elim_blk_L(const mat_zz_p& A, mat_zz_p *im, mat_zz_p *ker,
                 long w, bool full)
{
   long n = A.NumRows();
   long m = A.NumCols();

   if (w < 0 || w > m) LogicError("elim: bad args");

   // take care of corner cases
   if (n == 0) {
      if (im) im->SetDims(0, m);
      if (ker) ker->SetDims(0, 0);
      return 0;
   }

   if (w == 0) {
      if (im) {
         if (full)
            (*im) = A;
         else
            im->SetDims(0, m);
      }
      if (ker) ident(*ker, n);
      return 0;
   }

   if (NTL_OVERFLOW(n, MAT_BLK_SZ, 0)) ResourceError("dimension too large");
   if (NTL_OVERFLOW(m, MAT_BLK_SZ, 0)) ResourceError("dimension too large");

   long npanels = (m+MAT_BLK_SZ-1)/MAT_BLK_SZ;
   

   Vec< UniqueArray<unsigned long> > M;
   M.SetLength(npanels);
   for (long panel = 0; panel < npanels; panel++) {
      M[panel].SetLength(n*MAT_BLK_SZ);
      unsigned long *panelp = &M[panel][0];

      for (long h = 0; h < n*MAT_BLK_SZ; h++) panelp[h] = 0;
   }

   // copy A into panels
   for (long jj = 0, panel = 0; jj < m; jj += MAT_BLK_SZ, panel++) {
      long j_max = min(jj+MAT_BLK_SZ, m);
      unsigned long *panelp = &M[panel][0];

      for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
         const zz_p *ap = A[i].elts() + jj;

         for (long j = jj; j < j_max; j++)
            panelp[j-jj] = rep(ap[j-jj]);
      }
   }

   UniqueArray<unsigned long> aux_panel_store;
   aux_panel_store.SetLength(n*MAT_BLK_SZ);
   unsigned long * NTL_RESTRICT aux_panel = &aux_panel_store[0];


   UniqueArray<unsigned long> buf_store1;
   buf_store1.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
   unsigned long *buf1 = &buf_store1[0];

   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations

   Vec<long> pcol;
   pcol.SetLength(n);
   // pcol[i] records pivot columns for row i

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_reduce_struct red_struct = zz_p::red_struct();

   bool pivoting = false;

   unsigned long ured_trigger = 
      (~(0UL)-cast_unsigned(p-1))/(cast_unsigned(p-1)*cast_unsigned(p-1));
   // NOTE: corner case at p == 2: need unsigned long to prevent overflow

   long red_trigger = min(cast_unsigned(NTL_MAX_LONG), ured_trigger);

   long red_count = red_trigger;

   long r = 0, rr = 0, k = 0, kk = 0;
   long rpanel = 0, kpanel = 0;

   while (k < w) {

      if (r > rr && ker) { 
         // we have a panel from a previous iteration
         // we store enough of it to facilitate the kernel
         // computation later. At this point, we have 
         // r == rr+INV_BLK_SIZE, and it suffices to store
         // rows [r..n) into M[rpanel], and this will not
         // overwrite anything useful in M[rpanel]
         
         unsigned long *panelp = &M[rpanel][0];
         for (long h = r*MAT_BLK_SZ; h < n*MAT_BLK_SZ; h++) {
            panelp[h] = aux_panel[h];
         }

         rpanel++;
      }

      rr = r;

      for (long h = 0; h < n*MAT_BLK_SZ; h++) aux_panel[h] = 0;

      bool cleanup = false;

      if (red_count-MAT_BLK_SZ < 0) {
         red_count = red_trigger;
         cleanup = true;
      }

      red_count = red_count-MAT_BLK_SZ;

      for (; r < rr+MAT_BLK_SZ && k < w; k++) { // panel incomplete

         if (k == kk+MAT_BLK_SZ) { // start new kpanel
            kk = k;
            kpanel++;
         }

         unsigned long * NTL_RESTRICT kpanelp = &M[kpanel][0];

         if (k == kk) { // a fresh kpanel -- special processing

            if (cleanup) {
               for (long h = 0; h < n*MAT_BLK_SZ; h++)
                  kpanelp[h] = rem(kpanelp[h], p, red_struct);
            }

            if (r > rr) {


               // apply current sequence of permutations

               ApplySwaps(kpanelp, rr, r, P);

	       // clean aux_panel
	       for (long h = 0; h < n*MAT_BLK_SZ; h++)
		  aux_panel[h] = rem(aux_panel[h], p, red_struct);

               // copy rows [rr..r) of kpanel into buf1
               for (long i = 0; i < (r-rr)*MAT_BLK_SZ; i++)
                  buf1[i] = rem(kpanelp[rr*MAT_BLK_SZ+i], p, red_struct);

               TransposeBlock(buf1, 0);

               // kpanel[rr..n) += aux_panel[rr..n)*buf1

               muladd_all_by_32(rr, n, kpanelp, aux_panel, buf1, r-rr);
            }
         }

         long pos = -1;
         long pivot;
         long pivot_inv;
         for (long i = r; i < n; i++) {
            pivot = rem(kpanelp[i*MAT_BLK_SZ+(k-kk)], p, red_struct);
            kpanelp[i*MAT_BLK_SZ+(k-kk)] = pivot;

            if (pivot != 0) {
               pivot_inv = InvMod(pivot, p);
               pos = i;
               break;
            }
         }

         if (pos == -1) {
            continue;
         }

         unsigned long * NTL_RESTRICT y = &kpanelp[r*MAT_BLK_SZ];
         unsigned long * NTL_RESTRICT y1 = &aux_panel[r*MAT_BLK_SZ];
         if (r != pos) {
            // swap rows pos and r
            unsigned long * NTL_RESTRICT x = &kpanelp[pos*MAT_BLK_SZ];
            unsigned long * NTL_RESTRICT x1 = &aux_panel[pos*MAT_BLK_SZ];

            for (long j = k-kk; j < MAT_BLK_SZ; j++) _ntl_swap(x[j], y[j]);
            for (long j = 0; j < r-rr; j++) _ntl_swap(x1[j], y1[j]);
            
            P[r] = pos;
            pivoting = true;
         }

         // clean up row r of kpanel and aux_panel
         for (long j = k-kk; j < MAT_BLK_SZ; j++) 
            y[j] = rem(y[j], p, red_struct);
         for (long j = 0; j < r-rr; j++) 
            y1[j] = rem(y1[j], p, red_struct);

         // clear column
         for (long i = r+1; i < n; i++) {
            unsigned long * NTL_RESTRICT x = &kpanelp[i*MAT_BLK_SZ];
            unsigned long * NTL_RESTRICT x1 = &aux_panel[i*MAT_BLK_SZ];
            long t1 = rem(x[k-kk], p, red_struct);
            t1 = MulMod(t1, pivot_inv, p);
            t1 = NegateMod(t1, p);
            x[k-kk] = 0;
            x1[r-rr] = t1;
            if (t1 == 0) continue;

            // add t1 * row r to row i
            unsigned long ut1 = t1;

            for (long j = k-kk+1; j < MAT_BLK_SZ; j++) 
               x[j] += y[j]*ut1;
            for (long j = 0; j < r-rr; j++) 
               x1[j] += y1[j]*ut1;
         }

         pcol[r] = k;
         r++;
      }

      if (r > rr) {

         // we have a panel 

         // clean it up
         for (long h = 0; h < n*MAT_BLK_SZ; h++)
            aux_panel[h] = rem(aux_panel[h], p, red_struct);

         bool seq = 
            double(npanels-(kpanel+1))*double(n-rr)*double(r-rr)*double(MAT_BLK_SZ) < PAR_THRESH;

         // apply aux_panel to remaining panels: [kpanel+1..npanels)
         NTL_GEXEC_RANGE(seq, npanels-(kpanel+1), first, last)  
         NTL_IMPORT(p)
         NTL_IMPORT(n)
         NTL_IMPORT(red_struct)
         NTL_IMPORT(aux_panel)
         NTL_IMPORT(rr)
         NTL_IMPORT(r)


         UniqueArray<unsigned long> buf_store;
         buf_store.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
         unsigned long *buf = &buf_store[0];


         for (long index = first; index < last; index++) {
            long jpanel = index + kpanel+1;

            unsigned long * NTL_RESTRICT jpanelp = &M[jpanel][0];

            if (cleanup) {
               for (long h = 0; h < n*MAT_BLK_SZ; h++) 
                  jpanelp[h] = rem(jpanelp[h], p, red_struct);
            }

            // perform swaps
            ApplySwaps(jpanelp, rr, r, P);

            // copy rows [rr..r) of jpanel into buf
            for (long i = 0; i < (r-rr)*MAT_BLK_SZ; i++)
               buf[i] = rem(jpanelp[rr*MAT_BLK_SZ+i], p, red_struct);

            TransposeBlock(buf, 0);

            // jpanel[rr..n) += aux_panel[rr..n)*buf

            muladd_all_by_32(rr, n, jpanelp, aux_panel, buf, r-rr);
         }
                     
         NTL_GEXEC_RANGE_END

      }

   }

   if (im) {
      mat_zz_p& Im = *im;;
      if (full)
         Im.SetDims(n, m);
      else
         Im.SetDims(r, m);

      for (long i = 0; i < r; i++) {
         long pc = pcol[i];
         for (long j = 0; j < pc; j++) Im[i][j].LoopHole() = 0;
         for (long j = pc; j < m; j++) {
            unsigned long t0 = M[j/MAT_BLK_SZ][i*MAT_BLK_SZ+(j%MAT_BLK_SZ)];
            Im[i][j].LoopHole() = rem(t0, p, red_struct);
         }
      }

      if (full) {
	 for (long i = r; i < n; i++) {
	    for (long j = 0; j < w; j++) Im[i][j].LoopHole() = 0;
	    for (long j = w; j < m; j++) {
	       unsigned long t0 = M[j/MAT_BLK_SZ][i*MAT_BLK_SZ+(j%MAT_BLK_SZ)];
	       Im[i][j].LoopHole() = rem(t0, p, red_struct);
	    }
	 }
      }
   }

   if (ker) {
      mat_zz_p& Ker = *ker;
      Ker.SetDims(n-r, n);
      if (r < n) {

	 long start_block = r/MAT_BLK_SZ;
	 long end_block = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;
	 long vblocks = end_block-start_block;
	 long hblocks = (r+MAT_BLK_SZ-1)/MAT_BLK_SZ;

	 Vec< UniqueArray<unsigned long> > kerbuf;
	 kerbuf.SetLength(vblocks);
	 for (long i = 0; i < vblocks; i++) 
	    kerbuf[i].SetLength(hblocks*MAT_BLK_SZ*MAT_BLK_SZ);

	 long colblocks = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;

	 // if r > rr, we have a panel sitting in 
	 // aux_panel, which may or may not be a full panel

         unsigned long *initial_panel = 0;
         if (r > rr) {
            initial_panel = aux_panel;
         }
         else {
            initial_panel = &M[hblocks-1][0];
         }

         for (long vb = start_block; vb < end_block; vb++) 
            CopyBlock(&kerbuf[vb-start_block][0], hblocks-1, initial_panel, vb, n);

         for (long hb = hblocks-2; hb >= 0; hb--) {

            ApplySwaps(&M[hb][0], (hb+1)*MAT_BLK_SZ, r, P);

            for (long b = hb+1; b < end_block; b++) {
               CopyBlock(&M[hb][0], b-1, &M[hb][0], b, n);
               TransposeBlock(&M[hb][0], b-1);
            }
         }

         bool seq = double(n-r)*double(r)*double(r)/2 < PAR_THRESH;


	 NTL_GEXEC_RANGE(seq, end_block-start_block, first, last)
	 NTL_IMPORT(p)
	 NTL_IMPORT(red_struct)
	 NTL_IMPORT(hblocks)

	 for (long index = first; index < last; index++) {
	    long vb = index + start_block;
	    unsigned long *kerbufp = &kerbuf[vb-start_block][0];

	    for (long hb = hblocks-2; hb >= 0; hb--) {
	       unsigned long *colbuf = &M[hb][0];
	       unsigned long *acc = &kerbufp[hb*MAT_BLK_SZ*MAT_BLK_SZ]; 

	       CopyBlock(acc, 0, colbuf, vb-1);
               TransposeBlock(acc, 0);


               unsigned long ured_trigger = 
                  (~(0UL)-cast_unsigned(p-1))/(cast_unsigned(p-1)*cast_unsigned(p-1));
               // NOTE: corner case at p == 2: need unsigned long to prevent overflow

               long red_trigger = min(cast_unsigned(NTL_MAX_LONG), ured_trigger);
	       long red_count = red_trigger;
     
	       for (long b = hb+1; b < hblocks; b++) { 

		  if (red_count-MAT_BLK_SZ < 0) {
		     red_count = red_trigger;
		     for (long h = 0; h < MAT_BLK_SZ*MAT_BLK_SZ; h++)
			acc[h] = rem(acc[h], p, red_struct);

		  }
		  red_count = red_count-MAT_BLK_SZ;

		  MulAddBlock(acc, &kerbufp[b*MAT_BLK_SZ*MAT_BLK_SZ],
				   &colbuf[(b-1)*MAT_BLK_SZ*MAT_BLK_SZ]);
	       }

	       for (long h = 0; h < MAT_BLK_SZ*MAT_BLK_SZ; h++)
		  acc[h] = rem(acc[h], p, red_struct);
	    }
         }

	 NTL_GEXEC_RANGE_END

         for (long i = r; i < n; i++) {

            unsigned long *kerbufp = &kerbuf[(i/MAT_BLK_SZ)-start_block][0];

            for (long j = 0; j < r; j++) {
               unsigned long t0 = 
                  kerbufp[(j/MAT_BLK_SZ)*MAT_BLK_SZ*MAT_BLK_SZ+
                          (i%MAT_BLK_SZ)*MAT_BLK_SZ+(j%MAT_BLK_SZ)];
          
               Ker[i-r][j].LoopHole() = long(t0);
            }
         }

         for (long i = 0; i < n-r; i++) {
            for (long j = 0; j < n-r; j++) {
               Ker[i][j+r].LoopHole() = 0;
            }
            Ker[i][i+r].LoopHole() = 1;
         }

	 if (pivoting) {
	    for (long i = 0; i < n-r; i++) {
	       zz_p *x = Ker[i].elts();

	       for (long k = n-1; k >= 0; k--) {
		  long pos = P[k];
		  if (pos != k) swap(x[pos], x[k]);
	       }
	    }
	 }
      }
   }

   return r;

}


static inline
void CopyBlock(long *dst_ptr, long dst_blk, const long *src_ptr, long src_blk, long src_limit)
{
   long src_row = src_blk*MAT_BLK_SZ;
   long dst_row = dst_blk*MAT_BLK_SZ;

   long nrows = min(MAT_BLK_SZ, src_limit - src_row);

   for (long i = 0; i < nrows; i++) 
      for (long j = 0; j < MAT_BLK_SZ; j++)
         dst_ptr[(dst_row + i)*MAT_BLK_SZ + j] = src_ptr[(src_row + i)*MAT_BLK_SZ + j];

   for (long i = nrows; i < MAT_BLK_SZ; i++) 
      for (long j = 0; j < MAT_BLK_SZ; j++)
         dst_ptr[(dst_row + i)*MAT_BLK_SZ + j] = 0;

}

static inline
void CopyBlock(long *dst_ptr, long dst_blk, const long *src_ptr, long src_blk)
{
   long src_row = src_blk*MAT_BLK_SZ;
   long dst_row = dst_blk*MAT_BLK_SZ;

   long nrows = MAT_BLK_SZ;

   for (long i = 0; i < nrows; i++) 
      for (long j = 0; j < MAT_BLK_SZ; j++)
         dst_ptr[(dst_row + i)*MAT_BLK_SZ + j] = src_ptr[(src_row + i)*MAT_BLK_SZ + j];
}

static inline
void TransposeBlock(long *dst_ptr, long dst_blk)
{
   dst_ptr += dst_blk*MAT_BLK_SZ*MAT_BLK_SZ;

   for (long i = 0; i < MAT_BLK_SZ; i++)
      for (long j = 0; j < i; j++)
         _ntl_swap(dst_ptr[i*MAT_BLK_SZ+j], dst_ptr[i+j*MAT_BLK_SZ]);
}

static inline
void SwapOneRow(long *panelp, long i, long pos)
{
   long * NTL_RESTRICT pos_p = &panelp[pos*MAT_BLK_SZ];
   long * NTL_RESTRICT i_p = &panelp[i*MAT_BLK_SZ];
   for (long j = 0; j < MAT_BLK_SZ; j++)
      _ntl_swap(pos_p[j], i_p[j]);
}

static inline
void ApplySwaps(long *panelp, long start, long end, const Vec<long>& P)
{
   for (long i = start; i < end; i++) {
      long pos = P[i];
      if (pos != i) 
         SwapOneRow(panelp, i, pos);
   }
}


static inline
void MulAddBlock(long *x, const long *y, const long *z, 
                 long p, sp_ll_reduce_struct ll_red_struct)
{
   // x += y*z 

   muladd_all_by_32(0, MAT_BLK_SZ, x, y, z, MAT_BLK_SZ, p, ll_red_struct);
}



static
long elim_blk_LL(const mat_zz_p& A, mat_zz_p *im, mat_zz_p *ker,
                 long w, bool full)
{
   long n = A.NumRows();
   long m = A.NumCols();

   if (w < 0 || w > m) LogicError("elim: bad args");

   // take care of corner cases
   if (n == 0) {
      if (im) im->SetDims(0, m);
      if (ker) ker->SetDims(0, 0);
      return 0;
   }

   if (w == 0) {
      if (im) {
         if (full)
            (*im) = A;
         else
            im->SetDims(0, m);
      }
      if (ker) ident(*ker, n);
      return 0;
   }

   if (NTL_OVERFLOW(n, MAT_BLK_SZ, 0)) ResourceError("dimension too large");
   if (NTL_OVERFLOW(m, MAT_BLK_SZ, 0)) ResourceError("dimension too large");

   long npanels = (m+MAT_BLK_SZ-1)/MAT_BLK_SZ;
   

   Vec< UniqueArray<long> > M;
   M.SetLength(npanels);
   for (long panel = 0; panel < npanels; panel++) {
      M[panel].SetLength(n*MAT_BLK_SZ);
      long *panelp = &M[panel][0];

      for (long h = 0; h < n*MAT_BLK_SZ; h++) panelp[h] = 0;
   }

   // copy A into panels
   for (long jj = 0, panel = 0; jj < m; jj += MAT_BLK_SZ, panel++) {
      long j_max = min(jj+MAT_BLK_SZ, m);
      long *panelp = &M[panel][0];

      for (long i = 0; i < n; i++, panelp += MAT_BLK_SZ) {
         const zz_p *ap = A[i].elts() + jj;

         for (long j = jj; j < j_max; j++)
            panelp[j-jj] = rep(ap[j-jj]);
      }
   }

   UniqueArray<long> aux_panel_store;
   aux_panel_store.SetLength(n*MAT_BLK_SZ);
   long * NTL_RESTRICT aux_panel = &aux_panel_store[0];


   UniqueArray<long> buf_store1;
   buf_store1.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
   long *buf1 = &buf_store1[0];

   Vec<long> P;
   P.SetLength(n);
   for (long k = 0; k < n; k++) P[k] = k;
   // records swap operations

   Vec<long> pcol;
   pcol.SetLength(n);
   // pcol[i] records pivot columns for row i

   long p = zz_p::modulus();
   mulmod_t pinv = zz_p::ModulusInverse();
   sp_ll_reduce_struct ll_red_struct = zz_p::ll_red_struct();

   bool pivoting = false;

   long r = 0, rr = 0, k = 0, kk = 0;
   long rpanel = 0, kpanel = 0;

   while (k < w) {

      if (r > rr && ker) { 
         // we have a panel from a previous iteration
         // we store enough of it to facilitate the kernel
         // computation later. At this point, we have 
         // r == rr+INV_BLK_SIZE, and it suffices to store
         // rows [r..n) into M[rpanel], and this will not
         // overwrite anything useful in M[rpanel]
         
         long *panelp = &M[rpanel][0];
         for (long h = r*MAT_BLK_SZ; h < n*MAT_BLK_SZ; h++) {
            panelp[h] = aux_panel[h];
         }

         rpanel++;
      }

      rr = r;

      for (long h = 0; h < n*MAT_BLK_SZ; h++) aux_panel[h] = 0;

      for (; r < rr+MAT_BLK_SZ && k < w; k++) { // panel incomplete

         if (k == kk+MAT_BLK_SZ) { // start new kpanel
            kk = k;
            kpanel++;
         }

         long * NTL_RESTRICT kpanelp = &M[kpanel][0];

         if (k == kk) { // a fresh kpanel -- special processing


            if (r > rr) {


               // apply current sequence of permutations

               ApplySwaps(kpanelp, rr, r, P);

               // copy rows [rr..r) of kpanel into buf1
               for (long i = 0; i < (r-rr)*MAT_BLK_SZ; i++)
                  buf1[i] = kpanelp[rr*MAT_BLK_SZ+i];

               TransposeBlock(buf1, 0);

               // kpanel[rr..n) += aux_panel[rr..n)*buf1

               muladd_all_by_32(rr, n, kpanelp, aux_panel, buf1, r-rr, p, ll_red_struct);
            }
         }

         long pos = -1;
         long pivot;
         long pivot_inv;
         for (long i = r; i < n; i++) {
            pivot = kpanelp[i*MAT_BLK_SZ+(k-kk)];
            kpanelp[i*MAT_BLK_SZ+(k-kk)] = pivot;

            if (pivot != 0) {
               pivot_inv = InvMod(pivot, p);
               pos = i;
               break;
            }
         }

         if (pos == -1) {
            continue;
         }

         long * NTL_RESTRICT y = &kpanelp[r*MAT_BLK_SZ];
         long * NTL_RESTRICT y1 = &aux_panel[r*MAT_BLK_SZ];
         if (r != pos) {
            // swap rows pos and r
            long * NTL_RESTRICT x = &kpanelp[pos*MAT_BLK_SZ];
            long * NTL_RESTRICT x1 = &aux_panel[pos*MAT_BLK_SZ];

            for (long j = k-kk; j < MAT_BLK_SZ; j++) _ntl_swap(x[j], y[j]);
            for (long j = 0; j < r-rr; j++) _ntl_swap(x1[j], y1[j]);
            
            P[r] = pos;
            pivoting = true;
         }

         // clear column
         for (long i = r+1; i < n; i++) {
            long * NTL_RESTRICT x = &kpanelp[i*MAT_BLK_SZ];
            long * NTL_RESTRICT x1 = &aux_panel[i*MAT_BLK_SZ];
            long t1 = x[k-kk];
            t1 = MulMod(t1, pivot_inv, p);
            t1 = NegateMod(t1, p);
            x[k-kk] = 0;
            x1[r-rr] = t1;
            if (t1 == 0) continue;

            // add t1 * row r to row i
            long ut1 = t1;
            mulmod_precon_t ut1_pinv = PrepMulModPrecon(ut1, p, pinv);

            for (long j = k-kk+1; j < MAT_BLK_SZ; j++) 
               x[j] = AddMod(x[j], MulModPrecon(y[j], ut1, p, ut1_pinv), p);
            for (long j = 0; j < r-rr; j++) 
               x1[j] = AddMod(x1[j], MulModPrecon(y1[j], ut1, p, ut1_pinv), p);
         }

         pcol[r] = k;
         r++;
      }

      if (r > rr) {

         // we have a panel 

         bool seq = 
            double(npanels-(kpanel+1))*double(n-rr)*double(r-rr)*double(MAT_BLK_SZ) < PAR_THRESH;

         // apply aux_panel to remaining panels: [kpanel+1..npanels)
         NTL_GEXEC_RANGE(seq, npanels-(kpanel+1), first, last)  
         NTL_IMPORT(p)
         NTL_IMPORT(n)
         NTL_IMPORT(ll_red_struct)
         NTL_IMPORT(aux_panel)
         NTL_IMPORT(rr)
         NTL_IMPORT(r)


         UniqueArray<long> buf_store;
         buf_store.SetLength(MAT_BLK_SZ*MAT_BLK_SZ);
         long *buf = &buf_store[0];


         for (long index = first; index < last; index++) {
            long jpanel = index + kpanel+1;

            long * NTL_RESTRICT jpanelp = &M[jpanel][0];

            // perform swaps
            ApplySwaps(jpanelp, rr, r, P);

            // copy rows [rr..r) of jpanel into buf
            for (long i = 0; i < (r-rr)*MAT_BLK_SZ; i++)
               buf[i] = jpanelp[rr*MAT_BLK_SZ+i];

            TransposeBlock(buf, 0);

            // jpanel[rr..n) += aux_panel[rr..n)*buf

            muladd_all_by_32(rr, n, jpanelp, aux_panel, buf, r-rr, p, ll_red_struct);
         }
                     
         NTL_GEXEC_RANGE_END

      }

   }

   if (im) {
      mat_zz_p& Im = *im;;
      if (full)
         Im.SetDims(n, m);
      else
         Im.SetDims(r, m);

      for (long i = 0; i < r; i++) {
         long pc = pcol[i];
         for (long j = 0; j < pc; j++) Im[i][j].LoopHole() = 0;
         for (long j = pc; j < m; j++) {
            long t0 = M[j/MAT_BLK_SZ][i*MAT_BLK_SZ+(j%MAT_BLK_SZ)];
            Im[i][j].LoopHole() = t0;
         }
      }

      if (full) {
	 for (long i = r; i < n; i++) {
	    for (long j = 0; j < w; j++) Im[i][j].LoopHole() = 0;
	    for (long j = w; j < m; j++) {
	       long t0 = M[j/MAT_BLK_SZ][i*MAT_BLK_SZ+(j%MAT_BLK_SZ)];
	       Im[i][j].LoopHole() = t0;
	    }
	 }
      }
   }

   if (ker) {
      mat_zz_p& Ker = *ker;
      Ker.SetDims(n-r, n);
      if (r < n) {

	 long start_block = r/MAT_BLK_SZ;
	 long end_block = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;
	 long vblocks = end_block-start_block;
	 long hblocks = (r+MAT_BLK_SZ-1)/MAT_BLK_SZ;

	 Vec< UniqueArray<long> > kerbuf;
	 kerbuf.SetLength(vblocks);
	 for (long i = 0; i < vblocks; i++) 
	    kerbuf[i].SetLength(hblocks*MAT_BLK_SZ*MAT_BLK_SZ);

	 long colblocks = (n+MAT_BLK_SZ-1)/MAT_BLK_SZ;

	 // if r > rr, we have a panel sitting in 
	 // aux_panel, which may or may not be a full panel

         long *initial_panel = 0;
         if (r > rr) {
            initial_panel = aux_panel;
         }
         else {
            initial_panel = &M[hblocks-1][0];
         }

         for (long vb = start_block; vb < end_block; vb++) 
            CopyBlock(&kerbuf[vb-start_block][0], hblocks-1, initial_panel, vb, n);

         for (long hb = hblocks-2; hb >= 0; hb--) {

            ApplySwaps(&M[hb][0], (hb+1)*MAT_BLK_SZ, r, P);

            for (long b = hb+1; b < end_block; b++) {
               CopyBlock(&M[hb][0], b-1, &M[hb][0], b, n);
               TransposeBlock(&M[hb][0], b-1);
            }
         }

         bool seq = double(n-r)*double(r)*double(r)/2 < PAR_THRESH;


	 NTL_GEXEC_RANGE(seq, end_block-start_block, first, last)
	 NTL_IMPORT(p)
	 NTL_IMPORT(ll_red_struct)
	 NTL_IMPORT(hblocks)

	 for (long index = first; index < last; index++) {
	    long vb = index + start_block;
	    long *kerbufp = &kerbuf[vb-start_block][0];

	    for (long hb = hblocks-2; hb >= 0; hb--) {
	       long *colbuf = &M[hb][0];
	       long *acc = &kerbufp[hb*MAT_BLK_SZ*MAT_BLK_SZ]; 

	       CopyBlock(acc, 0, colbuf, vb-1);
               TransposeBlock(acc, 0);

	       for (long b = hb+1; b < hblocks; b++) { 
		  MulAddBlock(acc, &kerbufp[b*MAT_BLK_SZ*MAT_BLK_SZ],
				   &colbuf[(b-1)*MAT_BLK_SZ*MAT_BLK_SZ], p, ll_red_struct);
	       }
	    }
         }

	 NTL_GEXEC_RANGE_END

         for (long i = r; i < n; i++) {

            long *kerbufp = &kerbuf[(i/MAT_BLK_SZ)-start_block][0];

            for (long j = 0; j < r; j++) {
               long t0 = 
                  kerbufp[(j/MAT_BLK_SZ)*MAT_BLK_SZ*MAT_BLK_SZ+
                          (i%MAT_BLK_SZ)*MAT_BLK_SZ+(j%MAT_BLK_SZ)];
          
               Ker[i-r][j].LoopHole() = long(t0);
            }
         }

         for (long i = 0; i < n-r; i++) {
            for (long j = 0; j < n-r; j++) {
               Ker[i][j+r].LoopHole() = 0;
            }
            Ker[i][i+r].LoopHole() = 1;
         }

	 if (pivoting) {
	    for (long i = 0; i < n-r; i++) {
	       zz_p *x = Ker[i].elts();

	       for (long k = n-1; k >= 0; k--) {
		  long pos = P[k];
		  if (pos != k) swap(x[pos], x[k]);
	       }
	    }
	 }
      }
   }

   return r;

}


#endif



static
long elim(const mat_zz_p& A, mat_zz_p *im, mat_zz_p *ker, long w, bool full)
{
   long n = A.NumRows();
   long m = A.NumCols();

   if (w < 0 || w > m) LogicError("elim: bad args");

#ifndef NTL_HAVE_LL_TYPE

   return elim_basic(A, im, ker, w, full);

#else

   long p = zz_p::modulus();

   if (n/MAT_BLK_SZ < 4 || w/MAT_BLK_SZ < 4) {
      return elim_basic(A, im, ker, w, full);
   }
   else {
      long V = 4*MAT_BLK_SZ;

#ifdef NTL_HAVE_AVX
      if (p-1 <= MAX_DBL_INT &&
          V <= (MAX_DBL_INT-(p-1))/(p-1) &&
          V*(p-1) <= (MAX_DBL_INT-(p-1))/(p-1)) {

         return elim_blk_DD(A, im, ker, w, full);
      }
      else 
#endif
           if (cast_unsigned(V) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1) &&
               cast_unsigned(V)*cast_unsigned(p-1) <= (~(0UL)-cast_unsigned(p-1))/cast_unsigned(p-1))  {

         return elim_blk_L(A, im, ker, w, full);

      }
      else {
  
         return elim_blk_LL(A, im, ker, w, full);
      }
   
   }

#endif



}


// ******************************************************************
//
// High level interfaces
//
// ******************************************************************



long gauss(mat_zz_p& M, long w)
{
   return elim(M, &M, 0, w, true);
}
   

long gauss(mat_zz_p& M)
{
   return gauss(M, M.NumCols());
}

void image(mat_zz_p& X, const mat_zz_p& A)
{
   elim(A, &X, 0, A.NumCols(), false);
}
   
void kernel(mat_zz_p& X, const mat_zz_p& A)
{
   elim(A, 0, &X, A.NumCols(), false);
}


// ******************************************************************
//
// Operator/functional notation
//
// ******************************************************************




mat_zz_p operator+(const mat_zz_p& a, const mat_zz_p& b)
{
   mat_zz_p res;
   add(res, a, b);
   NTL_OPT_RETURN(mat_zz_p, res);
}

mat_zz_p operator*(const mat_zz_p& a, const mat_zz_p& b)
{
   mat_zz_p res;
   mul_aux(res, a, b);
   NTL_OPT_RETURN(mat_zz_p, res);
}

mat_zz_p operator-(const mat_zz_p& a, const mat_zz_p& b)
{
   mat_zz_p res;
   sub(res, a, b);
   NTL_OPT_RETURN(mat_zz_p, res);
}


mat_zz_p operator-(const mat_zz_p& a)
{
   mat_zz_p res;
   negate(res, a);
   NTL_OPT_RETURN(mat_zz_p, res);
}


vec_zz_p operator*(const mat_zz_p& a, const vec_zz_p& b)
{
   vec_zz_p res;
   mul_aux(res, a, b);
   NTL_OPT_RETURN(vec_zz_p, res);
}

vec_zz_p operator*(const vec_zz_p& a, const mat_zz_p& b)
{
   vec_zz_p res;
   mul(res, a, b);
   NTL_OPT_RETURN(vec_zz_p, res);
}


NTL_END_IMPL
