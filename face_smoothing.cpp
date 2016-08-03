// 2016 Damien Firmenich

#include "Halide.h"
#include <stdio.h>

#include "halide_image_io.h"
#include "color_conv.h"

using namespace Halide;
using namespace Halide::Tools;

#define PI 3.14159


Func bilateralFilter(Func im, float sigmaDomain, float sigmaRange) {
  Var x, y, i, j, v;
  Func w("weight"), diff("diff"), out("out"), outc("outc");
  RDom d(-2*sigmaRange, 4*sigmaRange+1, -2*sigmaRange, 4*sigmaRange+1);

  diff(x, y, i, j) = im(x,y) - im(x+i, y+j);
  w(x, y, i, j) = exp(-(diff(x,y,i,j)*diff(x,y,i,j))/(2.f*sigmaDomain*sigmaDomain)
                      -(i*i+j*j)/(2.f*sigmaRange*sigmaRange));

  out(x, y) = sum(im(x+d.x, y+d.y) * w(x, y, d.x, d.y)) / sum(w(x, y, d.x, d.y));
  outc(x ,y) = clamp(out(x,y), 0.f, 1.f);

  return outc;
}


int main(int argc, char **argv) {
  if (argc < 4) {
    printf("Usage: %s visible_image nir_image output\n", argv[0]);
    return 1;
  }

  Var x, y, c;

  Image<uint8_t> vis = load_image(argv[1]);
  Image<uint8_t> nir = load_image(argv[2]);

  Func vis_clamped, nir_clamped;
  vis_clamped = BoundaryConditions::repeat_edge(vis);
  nir_clamped = BoundaryConditions::repeat_edge(nir);

  // Converting into luminance, and normalizing
  Func vis_lab, vis_gray, nir_scaled;
  vis_lab = rgbToLab(vis_clamped);
  vis_gray(x, y) = vis_lab(x, y, 0) / 100.f;
  nir_scaled(x, y) = nir_clamped(x, y) / 255.f;

  // Decomposition into base and detail layers
  Func vis_base, nir_base, nir_details;
  vis_base = bilateralFilter(vis_gray, 0.05, 2);
  nir_base = bilateralFilter(nir_scaled, 0.05, 2);
  nir_details(x, y) = nir_scaled(x,y) / nir_base(x,y);

  // Merge and convert back
  Func smoothed_gray, smoothed, smoothed_rgb;
  // TODO change 90 to 100 and fix values over 100
  smoothed_gray(x, y) = 90 * (vis_base(x, y) * nir_details(x,y));
  smoothed(x, y, c) = select(c == 0, smoothed_gray(x,y), vis_lab(x,y,c));
  smoothed_rgb = labToRgb(smoothed);

  Func result;
  result(x, y, c) = cast<uint8_t>(smoothed_rgb(x,y,c));

  // Schedule
  vis_lab.compute_root();
  vis_base.compute_root();
  nir_base.compute_root();
  smoothed_rgb.compute_root();

  Image<uint8_t> output = result.realize(vis.width(), vis.height(), vis.channels());
  save_image(output, argv[3]);

  return 0;
}
