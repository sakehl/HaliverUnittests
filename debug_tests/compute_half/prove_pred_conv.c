/*@
 ghost
 context_everywhere extentx>0 ** extenty>0;
 context_everywhere _f != NULL ** \pointer_length(_f) == extentx*extenty;
 requires (\forall* int i; 0<=i && i < \pointer_length(_f); Perm(&_f[i], write));
 ensures (\forall* int x, int y; minx <= x && x < minx+extentx && miny <= y && y < miny+extenty; {:f_pred(_f, x, minx, extentx, y, miny, extenty):});
void to_pred(int* _f, int minx, int extentx, int miny, int extenty){
 
    loop_invariant miny <= j && j <= miny+extenty;
    loop_invariant (\forall* int x, int y; minx<=x && x<minx+extentx && j<=y && y<miny+extenty;
      Perm(&_f[(y-miny)*extentx + x-minx], write));
    loop_invariant (\forall* int x, int y; minx<=x && x<minx+extentx && miny<=y && y<j;
      {:f_pred(_f, x, minx, extentx, y, miny, extenty):});
  for(int j=miny; j<miny+extenty; j++){
      loop_invariant minx <= i && i <= minx+extentx;
      loop_invariant (\forall* int x; i<=x && x<minx+extentx
      && lemma_2d_access(x, minx, 1, extentx, j, miny, extentx, extenty);
        Perm(&_f[(j-miny)*extentx + x-minx], write));
      loop_invariant (\forall* int x; minx<=x && x<i;
        {:f_pred(_f, x, minx, extentx, j, miny, extenty):});
    for(int i=minx; i<minx+extentx; i++){
      fold f_pred(_f, i, minx, extentx, j, miny, extenty);
    }
  }
}
  
  ghost
  context_everywhere extentx>0 ** extenty>0;
  context_everywhere _f != NULL ** \pointer_length(_f) == extentx*extenty;
  requires (\forall* int x, int y; minx <= x && x < minx+extentx && miny <= y && y < miny+extenty; {:f_pred(_f, x, minx, extentx, y, miny, extenty):});
  ensures _int32_pred(_f);
void converse_pred(int* _f, int minx, int extentx, int miny, int extenty){
    loop_invariant miny <= j && j <= miny+extenty;
    loop_invariant (\forall* int x, int y; minx<=x && x<minx+extentx && miny<=y && y<j;
      Perm(&_f[(y-miny)*extentx + x-minx], write));
    loop_invariant (\forall* int x, int y; minx<=x && x<minx+extentx && j<=y && y<miny+extenty;
      {:f_pred(_f, x, minx, extentx, y, miny, extenty):});
  for(int j=miny; j<miny+extenty; j++){
      loop_invariant minx <= i && i <= minx+extentx;
      loop_invariant (\forall* int x; minx<=x && x<i
      && lemma_2d_access(x, minx, 1, extentx, j, miny, extentx, extenty);
        Perm(&_f[(j-miny)*extentx + x-minx], write));
      loop_invariant (\forall* int x; i<=x && x<minx+extentx;
        {:f_pred(_f, x, minx, extentx, j, miny, extenty):});
    for(int i=minx; i<minx+extentx; i++){
      unfold f_pred(_f, i, minx, extentx, j, miny, extenty);
    }
  }
  fold _int32_pred(_f);
}



resource f_pred(
 int* data0
 , int x, int x_min, int x_extent, int y, int y_min, int y_extent) = 
 ( x >= x_min && x < x_min + x_extent && 
  y >= y_min && y < y_min + y_extent && 
  data0 != NULL && \pointer_length(data0) == x_extent * y_extent && 
  lemma_2d_access(x, x_min, 1, x_extent, y, y_min, x_extent, y_extent)) ** 
  (\let int idx = (x-x_min)*1 + (y-y_min)*x_extent;
   Perm(&data0[idx], write));

resource _int32_pred(int* data) = data != NULL ** (\forall* int i; 0<=i && i< \pointer_length(data); Perm(&data[i], write) );

 requires a-min_a >= 0 && a-min_a<extent_a;
 requires b-min_b >= 0 && b-min_b<extent_b;
 requires stride_a > 0;
 requires stride_b >= extent_a*stride_a;

 ensures 0 <= (b-min_b)*stride_b;
 ensures 0 <= (a-min_a)*stride_a + (b-min_b)*stride_b;
 ensures (a-min_a)*stride_a + (b-min_b)*stride_b < stride_b*extent_b;
 ensures \result;
 decreases;
pure bool lemma_2d_access(
 int a, int min_a, int stride_a, int extent_a,
 int b, int min_b, int stride_b, int extent_b);
@*/