// Implementation based on the article "Efficient Gaussian blur with linear sampling"
// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
// Modified into a Lens Flare shader by SolarLiner - 02/2015
// Ported from John Chapman's Lens Flare, 2 years after :) : http://john-chapman-graphics.blogspot.co.uk/2013/02/pseudo-lens-flare.html

#include "ReShade/LensFlare/lensflare.h"
#include "ReShade/LensFlare/lensflare_settings.h"