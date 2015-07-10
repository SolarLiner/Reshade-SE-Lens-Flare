// Implementation based on the article "Efficient Gaussian blur with linear sampling"
// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
// Modified into a Lens Flare shader by SolarLiner - 02/2015
// Ported from John Chapman's Lens Flare, 2 years after :) : http://john-chapman-graphics.blogspot.co.uk/2013/02/pseudo-lens-flare.html

//GaussEffect
//[0 or 1] For debug. 1 shows the flare only. 0 shows the original image behind.
#define GaussEffect 1

//Ghots number
//[1 to 16] Number of ghosts to be added to the final image. Please keep odd. / ! \ DON'T PUT ZERO
#define nGhosts 	8

//Blur Strength
//[0.00 to 1.00]   Amount of effect blended into the final image.
#define BlurStrength	0.15

//Halo Strength
//[0.00 to 1.00] Amount of halo effect blended into the final image.
#define HaloStrength 	0.45

//Dirt Strength
//[0.00 to 1.00] Amount of lens dirt applied.
#define DirtStrength	0.6

// Chromatic Aberration strength
//[0.00 to 0.08] Defines the amount of chromatic aberration to apply to the ghosts. /!\ KEEP IT LOW /!\ 
#define ChromaticAberrationStrength	0.01

// Threshold
//[0 to 255] Luminance color at which the stars are enabled 
#define Threshold	250

// Threshold knee
//[0 to 255] Size of the clamping softness, in luminance values
#define ThresKnee	50

//Input Saturation
// Defines the amount of color from the original frame used in the lens flare.
#define Vibrance     			10.00 						//[-50.00 to 50.00] Intelligently saturates (or desaturates if you use negative values) the pixels depending on their original saturation.
#define Vibrance_RGB_balance 	float3(1.00, 1.00, 1.50) 	//[-10.00 to 10.00] A per channel multiplier to the Vibrance strength so you can give more boost to certain colors over others

// Lens Color Amount
//[0.00 to 1.00] Amount of coloring the ghosts get.
#define LensColorLevel	0.1

//Bloom Width
//HW is Horizontal, VW is Vertical, SW is Slant. Higher numbers = wider bloom.
#define HW 10.00
#define VW 10.00
#define SW 15.00

//GaussQuality
//0 = original, 1 = new. New is the same as original but has additional sample points in-between.
//When using 1, setting N_PASSES to 9 can help smooth wider bloom settings.
#define GaussQuality 1

//N_PASSES
//Number of gaussian passes. When GaussQuality = 0, N_PASSES must be set to 3, 4, or 5.
//When using GaussQuality = 1, N_PASSES must be set to 3,4,5,6,7,8, or 9.
//Still fine tuning this. Changing the number of passes can affect brightness. 
#define N_PASSES 9