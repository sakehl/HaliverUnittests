#include "Halide.h"
#include <stdio.h>

using namespace Halide;

int main(int argc, char *argv[]) {

  Func f("f"), out("out");
  Var x("x"), y("y");
  ImageParam max_x(type_of<int>(), 1, "max_x");

  f(x) = 1;
  f.ensures(f(x) == 1);
  f.compute_root();

  RDom r(0,42, "r");
  r.where(r < max_x(y));
  out(x, y) = x + y;
  out.ensures(out(x,y) == x+y);
  out(x,y) += f(r);
  out.invariant(out(x,y) == x + y + min(max(max_x(y), 0), r));
  out.ensures(out(x,y) == x + y + min(max(max_x(y), 0), 42));

  int nx = 100, ny = 42;
  out.output_buffer().dim(0).set_bounds(0,nx);
  out.output_buffer().dim(1).set_bounds(0,ny);
  out.output_buffer().dim(1).set_stride(nx);
  max_x.dim(0).set_bounds(0, ny);
  Var yi("yi"), yo("yo");
  out.update().split(y, yo, yi, 4, TailStrategy::GuardWithIf);
  
  Target target = Target();
  Target new_target = target
    .with_feature(Target::NoAsserts)
    .with_feature(Target::NoBoundsQuery)
    ;
  
  std::vector<Annotation> pipeline_anns;

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
  out.compile_to_c(name + "_non_unique"+ ".c" , {max_x}, pipeline_anns, name, new_target, mem_only, false);
  out.compile_to_c(name + ".c" , {max_x}, pipeline_anns, name, new_target, mem_only, true);
}