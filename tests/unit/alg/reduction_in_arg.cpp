#include "Halide.h"
#include <stdio.h>

using namespace Halide;

int main(int argc, char *argv[]) {

  Func f("f"), out("out");
  Var x("x"), y("y");

  f(x) = 1;
  f.ensures(f(x) == 1);
  f.compute_root();

  RDom r(0,42, "r");
  out(x) = x;
  out.ensures(out(x) == x);
  out(r) += f(r);
  // For values smaller than 0 or >=42, this never gets executed
  // so x<0 || x>=42 ==> out(x) == x;
  // For values for which r is smaller or equal than x, the reduction domain is not yet executed
  // So r<=x ==> out(x) == x;
  // For the remaining values, we have added one to the value of x, so
  // x>=0 && x<r ==> out(x) == x + 1;
  // This can be summarised as follows
  out.invariant(out(x) == x + select(x >= 0 && x < r, 1, 0));
  out.ensures(out(x) == x + select(x >= 0 && x < 42, 1, 0));

  int nx = 100, ny = 42;
  out.output_buffer().dim(0).set_bounds(0,nx);
  // out.output_buffer().dim(1).set_bounds(0,ny);
  // out.output_buffer().dim(1).set_stride(nx);
  
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
  out.compile_to_c(name + "-non-unique"+ ".c" , {}, pipeline_anns, name, new_target, mem_only, false);
  out.compile_to_c(name + ".c" , {}, pipeline_anns, name, new_target, mem_only, true);
}