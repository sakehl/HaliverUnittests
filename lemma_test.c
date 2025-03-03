/*@
 requires a >= 0;
 requires b > 0;
 requires a < max_a;
 ensures a*b <= (max_a-1)*b;
 ensures \result;
 decreases b;
pure bool lemma_nonlinear(int a, int b, int max_a) = b>1 ? lemma_nonlinear(a, b-1, max_a) : true;
 
 requires a-min_a >= 0 && a-min_a<extent_a;
 requires stride_a > 0;

 ensures (a-min_a)*stride_a <= (extent_a-1)*stride_a;
 ensures \result;
 decreases;
pure bool lemma_1d_access(
 int a, int min_a, int stride_a, int extent_a) = lemma_nonlinear(a-min_a, stride_a, extent_a);


 requires a-min_a >= 0 && a-min_a<extent_a;
 requires b-min_b >= 0 && b-min_b<extent_b;
 requires stride_a > 0;
 //requires stride_b >= extent_a*stride_a;
 requires stride_b > (a-min_a) * stride_a;

 ensures 0 <= (b-min_b)*stride_b;
 ensures 0 <= (a-min_a)*stride_a + (b-min_b)*stride_b;
 ensures (a-min_a)*stride_a + (b-min_b)*stride_b < stride_b*extent_b;
 ensures \result;
 decreases;
pure bool lemma_2d_access(
 int a, int min_a, int stride_a, int extent_a,
 int b, int min_b, int stride_b, int extent_b) = lemma_1d_access(a, min_a, stride_a, extent_a) && lemma_1d_access(b,min_b, stride_b, extent_b);

 requires a-min_a >= 0 && a-min_a < extent_a;
 requires b-min_b >= 0 && b-min_b < extent_b;
 requires c-min_c >= 0 && c-min_c < extent_c;
 requires stride_a > 0;
 requires stride_b > (a-min_a) * stride_a;
 requires stride_c > (b-min_b) * stride_b + (a-min_a) * stride_a;
 //requires stride_b >= extent_a * stride_a;
 //requires stride_c >= extent_b * stride_b;
 
 ensures 0 <= (a-min_a) * stride_a + (b-min_b) * stride_b + (c-min_c) * stride_c;
 ensures (a-min_a) * stride_a + (b-min_b) * stride_b + (c-min_c) * stride_c < stride_c * extent_c;
 ensures \result;
 decreases;
pure bool lemma_3d_access(
  int a, int min_a, int stride_a, int extent_a,
  int b, int min_b, int stride_b, int extent_b,
  int c, int min_c, int stride_c, int extent_c
) = lemma_2d_access(a, min_a, stride_a, extent_a, b, min_b, stride_b, extent_b) && lemma_nonlinear(c-min_c, stride_c, extent_c);

  ghost
void f(){
  int x, y, z;
  int nx, ny, nz;
  assume nx>0 && ny>0 && nz>0;
  assume 0 <= x && x< 5;
  assume 0 <= y && y< 3;
  assume x+5*y < 12;
  assume 0 <= z && z < 2;
  lemma_3d_access(x, 0, 1, 5
                , y, 0, 5, 3
                , z, 0, 12, 2);

  assert x+5*y + 12*z < 12*30;
}

 requires a-min_a >= 0 && a-min_a < extent_a;
 requires b-min_b >= 0 && b-min_b < extent_b;
 requires c-min_c >= 0 && c-min_c < extent_c;
 requires d-min_d >= 0 && d-min_d < extent_d;
 requires stride_a > 0;
 requires stride_b >= extent_a * stride_a;
 requires stride_c >= extent_b * stride_b;
 requires stride_d >= extent_c * stride_c;

 ensures 0 <= (a-min_a) * stride_a + (b-min_b) * stride_b + (c-min_c) * stride_c + (d-min_d) * stride_d;
 ensures (a-min_a) * stride_a + (b-min_b) * stride_b + (c-min_c) * stride_c + (d-min_d) * stride_d < stride_d * extent_d;
 ensures \result;
 decreases;
pure bool lemma_4d_access(
  int a, int min_a, int stride_a, int extent_a,
  int b, int min_b, int stride_b, int extent_b,
  int c, int min_c, int stride_c, int extent_c,
  int d, int min_d, int stride_d, int extent_d
);// = lemma_3d_access(a, min_a, stride_a, extent_a, b, min_b, stride_b, extent_b, c, min_c, stride_c, extent_c) && lemma_nonlinear(d-min_d, stride_d, extent_d);

 requires a-min_a >= 0 && a-min_a < extent_a;
 requires b-min_b >= 0 && b-min_b < extent_b;
 requires c-min_c >= 0 && c-min_c < extent_c;
 requires d-min_d >= 0 && d-min_d < extent_d;
 requires e-min_e >= 0 && e-min_e < extent_e;
 requires stride_a > 0;
 requires stride_b >= extent_a * stride_a;
 requires stride_c >= extent_b * stride_b;
 requires stride_d >= extent_c * stride_c;
 requires stride_e >= extent_d * stride_d;

 ensures 0 <= (a-min_a) * stride_a + (b-min_b) * stride_b + (c-min_c) * stride_c + (d-min_d) * stride_d + (e-min_e) * stride_e;
 ensures (a-min_a) * stride_a + (b-min_b) * stride_b + (c-min_c) * stride_c + (d-min_d) * stride_d + (e-min_e) * stride_e < stride_e * extent_e;
 ensures \result;
 decreases;
pure bool lemma_5d_access(
  int a, int min_a, int stride_a, int extent_a,
  int b, int min_b, int stride_b, int extent_b,
  int c, int min_c, int stride_c, int extent_c,
  int d, int min_d, int stride_d, int extent_d,
  int e, int min_e, int stride_e, int extent_e
);// = lemma_4d_access(a, min_a, stride_a, extent_a, b, min_b, stride_b, extent_b, c, min_c, stride_c, extent_c, d, min_d, stride_d, extent_d) && lemma_nonlinear(e-min_e, stride_e, extent_e);

@*/