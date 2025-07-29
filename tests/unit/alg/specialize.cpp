#include "Halide.h"
#include <stdio.h>

using namespace Halide;

int main(int argc, char *argv[]) {

  Func f("f"), out("out");
  Var x("x"), y("y");
  Param<bool> choose;

  out(x, y) = x + y;
  out.ensures(out(x, y) == x + y);
  out(x, y) = 
    select(choose, 
      2*y + out(x, y),
      3*y + out(x, y));  
  out.ensures(out(x,y) == select(choose, 2*y + x + y, 3*y + x + y)); 
  out.update().specialize(choose);

  int nx = 100, ny = 42;
  out.output_buffer().dim(0).set_bounds(0,nx);
  out.output_buffer().dim(1).set_bounds(0,ny);
  out.output_buffer().dim(1).set_stride(nx);
  
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
  out.translate_to_pvl(name +"_front.pvl", {choose}, pipeline_anns); 
  out.compile_to_c(name + "_non_unique"+ ".c" , {choose}, pipeline_anns, name, new_target, mem_only, false);
  out.compile_to_c(name + ".c" , {choose}, pipeline_anns, name, new_target, mem_only, true);
}