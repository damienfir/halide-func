// 2016 Damien Firmenich


#include "Halide.h"

#include "halide_image_io.h"

using namespace Halide;


// conversions from http://www.easyrgb.com/index.php

Func rgbToXyz(Func f) {
  Var x, y, c;
  Func out, rgb, R, G, B, conv;

  rgb(x, y, c) = f(x, y, c) / 255.f;
  conv(x, y, c) = select(rgb(x, y, c) > 0.04045f,
                         pow((rgb(x, y, c) + 0.055f) / 1.055f, 2.4f),
                         rgb(x, y, c) / 12.92f) * 100.f;
  R(x, y) = conv(x, y, 0);
  G(x, y) = conv(x, y, 1);
  B(x, y) = conv(x, y, 2);

  // Observer: 2°, Illuminant: D65
  out(x, y, c) = select(c == 0, R(x,y) * 0.4124f + G(x,y) * 0.3576f + B(x,y) * 0.1805f,
                        c == 1, R(x,y) * 0.2126f + G(x,y) * 0.7152f + B(x,y) * 0.0722f,
                        R(x,y) * 0.0193f + G(x,y) * 0.1192f + B(x,y) * 0.9505f);

  return out;
}


Func xyzToRgb(Func f) {
  Var x, y, c;
  Func out, rgb, R, G, B, conv, g;

  g(x, y, c) = f(x, y, c) / 100.f;

  rgb(x, y, c) = select(c == 0, g(x,y,0) * 3.2406f + g(x,y,1) * -1.5372f + g(x,y,2) * -0.4986f,
                        c == 1, g(x,y,0) * -0.9689f + g(x,y,1) * 1.8758f + g(x,y,2) * 0.0415f,
                        g(x,y,0) * 0.0557f + g(x,y,1) * -0.2040f + g(x,y,2) * 1.0570f);

  conv(x, y, c) = select(rgb(x, y, c) > 0.0031308f,
                         1.055f * pow(rgb(x,y,c), 1/2.4f) - 0.055f,
                         12.92f * rgb(x,y,c));

  out(x, y, c) = conv(x, y, c) * 255.f;

  return out;
}


Func xyzToLab(Func f) {
  Var x, y, c;
  Func conv, X, Y, Z, out, g;

  g(x, y, c) = select(c == 0, f(x, y, 0) / 95.047f,
                      c == 1, f(x, y, 1) / 100.f,
                      f(x, y, 2) / 108.883f);

  conv(x, y, c) = select(g(x, y, c) > 0.008856f,
                         pow(g(x, y, c), 1.f/3.f),
                         (7.787f * g(x, y, c)) + (16.f/116.f));
  X(x, y) = conv(x, y, 0);
  Y(x, y) = conv(x, y, 1);
  Z(x, y) = conv(x, y, 2);

  out(x, y, c) = select(c == 0, 116.f * Y(x,y) - 16.f,
                        c == 1, 500.f * (X(x,y) - Y(x,y)),
                        200.f * (Y(x,y) - Z(x,y)));

  return out;
}

Func labToXyz(Func f) {
  Var x, y, c;
  Func conv, X, Y, Z, out, g, g0;

  g0(x, y) = (f(x,y,0) + 16.f) / 116.f;
  g(x, y, c) = select(c == 1, g0(x, y),
                      c == 0, f(x,y,1) / 500.f + g0(x,y),
                      g0(x,y) - f(x,y,2) / 200.f);

  conv(x, y, c) = select(g(x, y, c) > 0.008856f,
                         pow(g(x, y, c), 3.f),
                         (g(x, y, c) - 16.f / 116.f) / 7.787f);

  X(x, y) = conv(x, y, 0);
  Y(x, y) = conv(x, y, 1);
  Z(x, y) = conv(x, y, 2);

  // Observer: 2°, Illuminant: D65
  out(x, y, c) = select(c == 0, X(x,y) * 95.047f,
                        c == 1, Y(x,y) * 100.f,
                        Z(x,y) * 108.883f);

  return out;
}


Func rgbToLab(Func f) {
  return xyzToLab(rgbToXyz(f));
}


Func labToRgb(Func f) {
  return xyzToRgb(labToXyz(f));
}


Func rgbToYuv(Func f){
  Var x,y,c;
  Func out;
  Expr Y = .299f*f(x,y,0) +  0.587f*f(x,y,1) + 0.114f*f(x,y,2);
  Expr U = -0.14713f*f(x,y,0) -0.28886f*f(x,y,1) + 0.436f*f(x,y,2);
  Expr V = 0.615f*f(x,y,0) -0.51499f*f(x,y,1) -0.10001f*f(x,y,2);
  out(x,y,c) = select(c == 0, Y,
                      c == 1, U,
                      V);
  return out;
}


Func yuvToRgb(Func f){
  Var x,y,c;
  Func out;
  Expr red = 1.0f*f(x,y,0) + 1.13983f*f(x,y,2);
  Expr green = 1.0f*f(x,y,0) - 0.39465f*f(x,y,1) - 0.58060f*f(x,y,2);
  Expr blue = 1.0f*f(x,y,0) + 2.03211f*f(x,y,1);
  out(x,y,c) = select(c == 0, red,
                      c == 1, green,
                      blue);
  return out;
}
