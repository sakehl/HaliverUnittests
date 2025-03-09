#include "Halide.h"
#include <stdio.h>
#include <vector>

using namespace Halide;

int main(int argc, char *argv[]) {
  Func denominator_inter("denominator_inter");
  Func numerator("numerator"), denominator("denominator");
  Var i("i"), si("si"), vis("vis");

  numerator(si) = 0;
  denominator(si) = 0;
  RDom rv(0, 10, "rv");

  denominator_inter(i, vis) = 0;
  denominator_inter(0, vis) = 1 + vis;
  denominator(rv) += denominator_inter(0, rv);

  numerator(rv) += rv;
  numerator.compute_root();
  denominator.compute_root();
  denominator.update().compute_with(numerator.update(), rv);

  Func out("out");
  out(si) = numerator(si) / denominator(si);

  out.output_buffer().dim(0).set_bounds(0,10);
  
  Target target = Target();
  Target new_target = target
    // .with_feature(Target::NoAsserts)
    // .with_feature(Target::NoBoundsQuery)
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
  out.compile_to_c(name + ".c" , {}, pipeline_anns, name, new_target, mem_only);
}