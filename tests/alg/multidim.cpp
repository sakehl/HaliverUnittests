#include "Halide.h"
#include <stdio.h>
#include <vector>

using namespace Halide;

int main(int argc, char *argv[]) {

  Func f("f"), h("h"), out("out"), input("input");
  Var i("i"), j("j"), x("x"), y("y");
  Param<int> nx("nx"), ny("ny"), minx("minx"), miny("miny");
  input(i, j, x, y) = i+j+x+y;

  f(x, y) = Tuple(input(0,0, x,y), input(1,1, x,y));
  out(i, j, x, y) = select(i == 0, j+f(x, y)[0], j+f(x, y)[1]);
  out.bound(i, 0, 2).bound(j, 0, 2);
  out.unroll(i).unroll(j);
  input.compute_at(out, x);

  out.output_buffer().dim(0).set_bounds(0, 2);
  out.output_buffer().dim(1).set_bounds(0, 2);
  out.output_buffer().dim(2).set_bounds(0, nx);
  out.output_buffer().dim(3).set_bounds(0, ny);

  out.output_buffer().dim(1).set_stride(2);
  out.output_buffer().dim(2).set_stride(4);
  out.output_buffer().dim(3).set_stride(4*nx);

  
  // out.ensures(
  //   implies(i==0, out(i,j,x,y) == x+y +j)
  // );

  // int nx = 100, ny = 42;
  // out.output_buffer().dim(0).set_bounds(0,2);
  // out.output_buffer().dim(1).set_bounds(0,2);
  // out.output_buffer().dim(2).set_bounds(0,nx);
  // out.output_buffer().dim(3).set_bounds(0,ny);
  // out.output_buffer().dim(1).set_stride(2);
  // out.output_buffer().dim(2).set_stride(2*2);
  // out.output_buffer().dim(3).set_stride(2*2*nx);
  
  Target target = Target();
  Target new_target = target
    .with_feature(Target::NoAsserts)
    .with_feature(Target::NoBoundsQuery)
    ;
  
  std::vector<Annotation> pipeline_anns = {context_everywhere(nx > 0),  context_everywhere(ny > 0)};

  std::string name = argv[1];
  std::string mem_only_s = "";
  if(argc >= 3){
    mem_only_s = argv[2];
  }
  bool mem_only = mem_only_s == "-mem_only";
  if(mem_only){
    name += "_mem";
  }
  out.translate_to_pvl(name +"_front.pvl", {}, pipeline_anns); 
  out.compile_to_c(name + ".c" , {nx, ny, minx, miny}, pipeline_anns, name, new_target);
}