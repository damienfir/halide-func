#include "Halide.h"
#include <stdio.h>

#include "halide_image_io.h"
#include "colorconversions.h"

using namespace Halide;
using namespace Halide::Tools;

#define PI 3.14159


Func bilateralFilter(Func im, const float sigmaDomain, const float sigmaRange) {
  Var x, y, i, j, v;
  RDom d(-3*sigmaRange, 3*sigmaRange, -3*sigmaRange, 3*sigmaRange);
  Func fdomain, frange, px, w, out;

  fdomain(v) = exp(-(v*v) / 2.f*sigmaDomain*sigmaDomain);
  frange(x, y) = exp(-(x*x+y*y)/2.f*sigmaRange*sigmaRange);

  // w(x, y, i, j) = fdomain(im(x,y) + im(x+i, y+j)) * frange(i, j);
  w(x, y, i, j) = frange(i, j);
  px(x, y, i, j) = im(x+i, y+j) * w(x, y, i, j);

  out(x, y) = sum(px(x,y, d.x, d.y)) / sum(w(x, y, d.x, d.y));

  return out;
}


int main(int argc, char **argv) {
  Var x, y, c;
  Func rgb, Lab, gray, filtered, color, converted, result;

  Image<uint8_t> input = load_image(argv[1]);

  rgb = BoundaryConditions::repeat_edge(input);
  Lab = xyzToLab(rgbToXyz(rgb));
  gray(x, y) = Lab(x, y, 0);

  filtered = bilateralFilter(gray, 1, 1);
  color(x, y, c) = select(c == 0, filtered(x,y), Lab(x,y,c));

  converted = xyzToRgb(labToXyz(Lab));
  result(x, y, c) = cast<uint8_t>(converted(x,y,c));

  Image<uint8_t> output = result.realize(input.width(), input.height(), input.channels());
  save_image(output, "output.png");

  return 0;
}
