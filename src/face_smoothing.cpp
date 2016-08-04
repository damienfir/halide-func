// 2016 Damien Firmenich

#include "Halide.h"
#include <stdio.h>
#include <chrono>
#include <iostream>

#include "halide_image_io.h"
#include "color_conv.h"
#include "filters.h"

using namespace Halide;
using namespace Halide::Tools;



int main(int argc, char **argv) {
  printf("Running...\n");
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
  // vis_base = bilateralFilter(vis_gray, 0.05, 2);
  // nir_base = bilateralFilter(nir_scaled, 0.05, 2);
  vis_base = bilateralGridFilter(vis_gray, 0.1f, 8);
  nir_base = bilateralGridFilter(nir_scaled, 0.1f, 8);
  nir_details(x, y) = nir_scaled(x,y) / nir_base(x,y);

  // Merge and convert back
  Func smoothed_gray, smoothed, smoothed_rgb;
  // TODO change 90 to 100 and fix values over 100
  smoothed_gray(x, y) = 90 * (vis_base(x, y) * nir_details(x,y));
  smoothed(x, y, c) = select(c == 0, smoothed_gray(x,y), vis_lab(x,y,c));
  smoothed_rgb = labToRgb(smoothed);
  // smoothed_rgb(x, y, c) = 255.f * vis_base(x, y);

  Func result;
  result(x, y, c) = cast<uint8_t>(smoothed_rgb(x,y,c));

  // Schedule
  vis_lab.compute_root();
  vis_base.compute_root();
  nir_base.compute_root();
  smoothed_rgb.compute_root();

  auto t1 = std::chrono::high_resolution_clock::now();

  Image<uint8_t> output = result.realize(vis.width(), vis.height(), vis.channels());

  auto t2 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
  std::cout << "Processing time: " << fp_ms.count() << " ms" << std::endl;

  save_image(output, argv[3]);

  return 0;
}
