// 2016 Damien Firmenich

#include "Halide.h"
#include <stdio.h>

#include "halide_image_io.h"
#include "colorconversions.h"

using namespace Halide;
using namespace Halide::Tools;

#define PI 3.14159


Func bilateralFilter(Func im, const float sigmaDomain, const float sigmaRange) {
  Var x, y, i, j, v;
  RDom d(cast<int>(-3*sigmaRange), cast<int>(6*sigmaRange+1),
         cast<int>(-3*sigmaRange), cast<int>(6*sigmaRange+1));
  Func fdomain, frange, w, out;

  fdomain(v) = exp(-(v*v)/2.f*sigmaDomain*sigmaDomain);
  frange(x, y) = exp(-(x*x+y*y)/2.f*sigmaRange*sigmaRange);

  // w(x, y, i, j) = fdomain(im(x,y) - im(x+i, y+j)) * frange(i, j);
  w(x, y, i, j) = 1.f;

  out(x, y) = sum(im(x+d.x, y+d.y) * w(x, y, d.x, d.y)) / sum(w(x, y, d.x, d.y));

  return out;
}


int main(int argc, char **argv) {
  Var x, y, c;

  Image<uint8_t> vis = load_image(argv[1]);
  Image<uint8_t> nir = load_image(argv[2]);

  Func vis_clamped, nir_clamped;
  vis_clamped = BoundaryConditions::repeat_edge(vis);
  nir_clamped = BoundaryConditions::repeat_edge(nir);

  Func vis_lab, vis_gray, nir_scaled;
  vis_lab = rgbToLab(vis_clamped);
  vis_gray(x, y) = vis_lab(x, y, 0) / 100.f;
  nir_scaled(x, y) = nir_clamped(x, y) / 255.f;

  Func vis_base, nir_base, nir_details;
  vis_base = bilateralFilter(vis_gray, 10, 1);
  nir_base = bilateralFilter(nir_scaled, 1, 1);
  nir_details(x, y) = nir_scaled(x,y) - nir_base(x,y);

  Func smoothed_gray, smoothed, smoothed_rgb;
  smoothed_gray(x, y) = 100.f * (vis_base(x, y) + nir_details(x, y));
  smoothed(x, y, c) = select(c == 0, 100.f * vis_base(x,y), vis_lab(x,y,c));
  smoothed_rgb = labToRgb(smoothed);

  Func result;
  result(x, y, c) = cast<uint8_t>(smoothed_rgb(x,y,c));

  Image<uint8_t> output = result.realize(vis.width(), vis.height(), vis.channels());
  save_image(output, "output.png");

  return 0;
}
