// 2016 Damien Firmenich

#include "Halide.h"


#define PI 3.14159


Func bilateralFilter(Func im, float r_sigma, float s_sigma) {
  Var x, y, i, j, v;
  Func w("weight"), diff("diff"), out("out"), outc("outc");
  RDom d(-2*s_sigma, 4*s_sigma+1, -2*s_sigma, 4*s_sigma+1);

  diff(x, y, i, j) = im(x,y) - im(x+i, y+j);
  w(x, y, i, j) = exp(-(diff(x,y,i,j)*diff(x,y,i,j))/(2.f*r_sigma*r_sigma)
                      -(i*i+j*j)/(2.f*s_sigma*s_sigma));

  out(x, y) = sum(im(x+d.x, y+d.y) * w(x, y, d.x, d.y)) / sum(w(x, y, d.x, d.y));
  outc(x ,y) = clamp(out(x,y), 0.f, 1.f);

  Var xi("xi"), yi("yi"), tidx("tidx");

  outc.compute_root()
    .tile(x, y, xi, yi, 64, 64)
    .fuse(x, y, tidx)
    .parallel(tidx);

  return outc;
}


Func bilateralGridFilter(Func im, float r_sigma, int s_sigma) {

  Var x, y, z, c, dx;

  RDom r(-s_sigma/2.f, s_sigma+1, -s_sigma/2.f, s_sigma+1);
  Expr val = im(x * s_sigma + r.x, y * s_sigma + r.y);
  val = clamp(val, 0.f, 1.f);
  Expr zi = cast<int>(val / r_sigma + 0.5f);
  Func grid("grid");
  grid(x, y, z, c) = 0.f;
  grid(x, y, zi, c) += select(c == 0, val, 1.f);

  Func blurx("blurx"), blury("blury"), blurz("blurz"), filter("filter");
  /* RDom spacial(-2*s_sigma, 4*s_sigma+1); */
  /* RDom domain(-2*r_sigma, 4*r_sigma+1); */
  /* filter(dx) = exp(-(dx * dx) / (2.f * s_sigma * s_sigma)); */
  /* blurz(x, y, z, c) = sum(grid(x, y, z + domain, c) * filter(domain)); */
  /* blury(x, y, z, c) = sum(blurz(x, y+spacial, z, c) * filter(spacial)); */
  /* blurx(x, y, z, c) = sum(blury(x+spacial, y, z, c) * filter(spacial)); */
  blurz(x, y, z, c) = (grid(x, y, z-2, c) +
                       grid(x, y, z-1, c)*4 +
                       grid(x, y, z  , c)*6 +
                       grid(x, y, z+1, c)*4 +
                       grid(x, y, z+2, c));
  blurx(x, y, z, c) = (blurz(x-2, y, z, c) +
                       blurz(x-1, y, z, c)*4 +
                       blurz(x  , y, z, c)*6 +
                       blurz(x+1, y, z, c)*4 +
                       blurz(x+2, y, z, c));
  blury(x, y, z, c) = (blurx(x, y-2, z, c) +
                       blurx(x, y-1, z, c)*4 +
                       blurx(x, y  , z, c)*6 +
                       blurx(x, y+1, z, c)*4 +
                       blurx(x, y+2, z, c));


  val = clamp(im(x, y), 0.f, 1.f);
  Expr zv = val / r_sigma;
  zi = cast<int>(zv);
  Expr zf = zv - zi;
  Expr xf = cast<float>(x % s_sigma) / s_sigma;
  Expr yf = cast<float>(y % s_sigma) / s_sigma;
  Expr xi = x / s_sigma;
  Expr yi = y / s_sigma;
  Func filtered("filtered");
  filtered(x, y, c) = lerp(
      lerp(lerp(blurx(xi, yi, zi, c), blurx(xi + 1, yi, zi, c), xf),
           lerp(blurx(xi, yi + 1, zi, c), blurx(xi + 1, yi + 1, zi, c), xf),
           yf),
      lerp(lerp(blurx(xi, yi, zi + 1, c), blurx(xi + 1, yi, zi + 1, c), xf),
           lerp(blurx(xi, yi + 1, zi + 1, c), blurx(xi + 1, yi + 1, zi + 1, c),
                xf),
           yf),
      zf);

  Func normalized("normalized");
  normalized(x, y) = filtered(x, y, 0) / filtered(x, y, 1);

  grid.compute_root();
  blurz.compute_root();
  blury.compute_root();
  blurx.compute_root();
  filtered.compute_root();
  normalized.compute_root();

  return normalized;
}
