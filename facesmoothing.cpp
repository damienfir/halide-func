// 2016 Damien Firmenich

#include "Halide.h"
#include <stdio.h>

#include "halide_image_io.h"
#include "colorconversions.h"

using namespace Halide;
using namespace Halide::Tools;

#define PI 3.14159


Func bilateralFilter(Func im, float sigmaDomain, float sigmaRange) {
  Var x, y, i, j, v;
  RDom d(-2*sigmaRange, 4*sigmaRange+1,
         -2*sigmaRange, 4*sigmaRange+1);
  Func w, out("out"), diff;

  diff(x, y, i, j) = im(x,y) - im(x+i, y+j);

  w(x, y, i, j) = exp(-(diff(x,y,i,j)*diff(x,y,i,j))/(2.f*sigmaDomain*sigmaDomain)
                      -(i*i+j*j)/(2.f*sigmaRange*sigmaRange));

  out(x, y) = sum(im(x+d.x, y+d.y) * w(x, y, d.x, d.y)) / sum(w(x, y, d.x, d.y));

  w.compute_root();

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
  vis_base = bilateralFilter(vis_gray, 0.1f, 1);
  nir_base = bilateralFilter(nir_scaled, 0.1f, 1);
  nir_details(x, y) = nir_scaled(x,y) - nir_base(x,y);

  Func smoothed_gray, smoothed, smoothed_rgb;
  smoothed_gray(x, y) = 100 * (vis_base(x, y) + nir_details(x, y));
  smoothed(x, y, c) = select(c == 0, smoothed_gray(x,y), vis_lab(x,y,c));
  smoothed_rgb = labToRgb(smoothed);
  // smoothed_rgb(x, y, c) = 255 * vis_base(x,y);

  Func result;
  result(x, y, c) = cast<uint8_t>(smoothed_rgb(x,y,c));

  // Schedule
  vis_gray.compute_root();
  vis_base.compute_root();

  Image<uint8_t> output = result.realize(vis.width(), vis.height(), vis.channels());
  save_image(output, "output.png");

  return 0;
}
