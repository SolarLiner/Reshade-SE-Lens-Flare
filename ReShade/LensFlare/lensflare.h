// Implementation based on the article "Efficient Gaussian blur with linear sampling"
// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
// Modified into a Lens Flare shader by SolarLiner - 02/2015
// Ported from John Chapman's Lens Flare, 2 years after :) : http://john-chapman-graphics.blogspot.co.uk/2013/02/pseudo-lens-flare.html

// Include settings file
#include "Reshade/LensFlare/lensflare_settings.h"

#define PIXEL_SIZE float2(BUFFER_RCP_WIDTH,BUFFER_RCP_HEIGHT)
#define CoefLuma_G            float3(0.2126, 0.7152, 0.0722)      // BT.709 & sRBG luma coefficient (Monitors and HD Television)
#define sharp_strength_luma_G (CoefLuma_G * GaussStrength + 0.2)
#define sharp_clampG        0.035

texture texDirt <source = "Reshade/LensFlare/texture/mcdirt.png"; >
{
	Width = 1920;
	Height = 1080;
};
sampler DirtSampler { Texture = texDirt; };

texture texStar <source = "Reshade/LensFlare/texture/lensstar.png"; >
{
	Width = 1024;
	Height = 1024;
};
sampler StarSampler { Texture = texStar; };

texture texColor <source = "Reshade/LensFlare/texture/lenscolor.png"; >
{
	Width = 256;
	Height = 1;
};
sampler ColorSampler { Texture = texColor; };


texture frameTex2D : SV_TARGET;
texture blurframeTex2D : SV_TARGET;
texture origframeTex2D
{
	Width = BUFFER_WIDTH;
	Height = BUFFER_HEIGHT;
};
texture thresTex
{
	Width = BUFFER_WIDTH;
	Height = BUFFER_HEIGHT;
};

sampler frameSampler
{
    Texture = frameTex2D;
    //AddressU  = Clamp; AddressV = Clamp;
    //MipFilter = None; MinFilter = Linear; MagFilter = Linear;
    //SRGBTexture = false;
};
sampler blurframeSampler
{
	Texture = blurframeTex2D;
};
sampler origframeSampler
{
    Texture = origframeTex2D;
    //AddressU  = Clamp; AddressV = Clamp;
    //MipFilter = None; MinFilter = Linear; MagFilter = Linear;
    //SRGBTexture = false;
};
sampler thresSampler { Texture = thresTex; };

void FrameVS(in uint id : SV_VertexID, out float4 pos : SV_Position, out float2 tex : TEXCOORD)
{
	tex.x = (id == 2) ? 2.0 : 0.0;
	tex.y = (id == 1) ? 2.0 : 0.0;
	pos = float4(tex * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
	
	float gRotationFlare = 3.14159268;
	
	tex -= 0.5 ;
	
	float cFlare = cos(gRotationFlare);
	float sFlare = sin(gRotationFlare);

	tex = mul(tex, float2x2(cFlare,-sFlare,sFlare, cFlare));
	
	tex += 0.5f ; 
}

float Desaturate (in float3 color)
{
	return dot( color, float3(0.22, 0.707, 0.071) );
}

float3 VibrancePass( float3 colorInput )
{
   	#define Vibrance_coeff float3(Vibrance_RGB_balance * Vibrance)

	float3 color = colorInput; //original input color
  	float3 lumCoeff = float3(0.212656, 0.715158, 0.072186);  //Values to calculate luma with

	float luma = Desaturate(color.rgb); //calculate luma (grey)

	float max_color = max(colorInput.r, max(colorInput.g,colorInput.b)); //Find the strongest color
	float min_color = min(colorInput.r, min(colorInput.g,colorInput.b)); //Find the weakest color

  	float color_saturation = max_color - min_color; //The difference between the two is the saturation

   	color.rgb = lerp(luma, color.rgb, (1.0 + (Vibrance_coeff * (1.0 - (sign(Vibrance_coeff) * color_saturation))))); //extrapolate between luma and original by 1 + (1-saturation) - current

 	return color; //return the result
}

float HistogramScale(in float value, in float min, in float max)
{
	float clamped = clamp(value, min, max);
	float mined = clamped - min;
	float maxed = mined*(1/(max-min));
	
	return maxed;
}

float4 ThresholdPS(in float4 position : SV_Position, in float2 coord : TEXCOORD0) : SV_Target
{
	float4 color = tex2D(frameSampler, coord);
	float4 thres = float4(0, 0, 0, 1);
	
	float fThres = Threshold/255.0f;
	float fThresSizeM = fThres - (ThresKnee/2/255.0f);
	float fThresSizeP = fThres + (ThresKnee/2/255.0f);
	fThresSizeP = fThresSizeP>1? 1 : fThresSizeP;
	
	float val = saturate(HistogramScale(Desaturate(color), fThresSizeM, fThresSizeP));
	color = float4(VibrancePass(color.rgb), 1);
	return float4(val, val, val, 1)*color;
}

float4 HGaussianBlurPS(in float4 pos : SV_Position, in float2 coord : TEXCOORD) : SV_TARGET
{
	#if (GaussQuality == 0)
	float sampleOffsets[5] = { 0.0, 1.4347826, 3.3478260, 5.2608695, 7.1739130 };
	float sampleWeights[5] = { 0.16818994, 0.27276957, 0.11690125, 0.024067905, 0.0021112196 };
	#else
	float sampleOffsets[9] = { 0.0, 1.43*.50, 1.43, 2, 3.35, 4, 5.26, 6, 7.17 };
	float sampleWeights[9] = { 0.168, 0.273, 0.273, 0.117, 0.117, 0.024, 0.024, 0.002, 0.002};
	#endif
	
	float4 color = tex2D(frameSampler, coord) * sampleWeights[0];
	for(int i = 1; i < N_PASSES; ++i) {
		color += tex2D(frameSampler, coord + float2(sampleOffsets[i]*HW * PIXEL_SIZE.x, 0.0)) * sampleWeights[i];
		color += tex2D(frameSampler, coord - float2(sampleOffsets[i]*HW * PIXEL_SIZE.x, 0.0)) * sampleWeights[i];
	}
	return color;
}

float4 VGaussianBlurPS(in float4 pos : SV_Position, in float2 coord : TEXCOORD) : SV_TARGET
{
	#if (GaussQuality == 0)
	float sampleOffsets[5] = { 0.0, 1.4347826, 3.3478260, 5.2608695, 7.1739130 };
	float sampleWeights[5] = { 0.16818994, 0.27276957, 0.11690125, 0.024067905, 0.0021112196 };
	#else 
	float sampleOffsets[9] = { 0.0, 1.4347826*.50, 1.4347826, 2, 3.3478260, 4, 5.2608695, 6, 7.1739130 };
	float sampleWeights[9] = { 0.16818994, 0.27276957, 0.27276957, 0.11690125, 0.11690125, 0.024067905, 0.024067905, 0.0021112196 , 0.0021112196};
	#endif

	float4 color = tex2D(frameSampler, coord) * sampleWeights[0];
	for(int i = 1; i < N_PASSES; ++i) {
		color += tex2D(frameSampler, coord + float2(0.0, sampleOffsets[i]*VW * PIXEL_SIZE.y)) * sampleWeights[i];
		color += tex2D(frameSampler, coord - float2(0.0, sampleOffsets[i]*VW * PIXEL_SIZE.y)) * sampleWeights[i];
	}
	return color;
}

float4 SGaussianBlurPS(in float4 pos : SV_Position, in float2 coord : TEXCOORD) : SV_TARGET
{
	#if (GaussQuality == 0)
	float sampleOffsets[5] = { 0.0, 1.4347826, 3.3478260, 5.2608695, 7.1739130 };
	float sampleWeights[5] = { 0.16818994, 0.27276957, 0.11690125, 0.024067905, 0.0021112196 };
	#else 
	float sampleOffsets[9] = { 0.0, 1.4347826*.50, 1.4347826, 2, 3.3478260, 4, 5.2608695, 6, 7.1739130 };
	float sampleWeights[9] = { 0.16818994, 0.27276957, 0.27276957, 0.11690125, 0.11690125, 0.024067905, 0.024067905, 0.0021112196 , 0.0021112196};
	#endif

	float4 color = tex2D(frameSampler, coord) * sampleWeights[0];
	for(int i = 1; i < N_PASSES; ++i) {
		color += tex2D(frameSampler, coord + float2(sampleOffsets[i]*SW * PIXEL_SIZE.x, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		color += tex2D(frameSampler, coord - float2(sampleOffsets[i]*SW * PIXEL_SIZE.x, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		color += tex2D(frameSampler, coord + float2(-sampleOffsets[i]*SW * PIXEL_SIZE.x, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		color += tex2D(frameSampler, coord + float2(sampleOffsets[i]*SW * PIXEL_SIZE.x, -sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
	}
	return color * 0.50;
}	

float4 BGaussianBlurPS(in float4 pos : SV_Position, in float2 coord : TEXCOORD) : SV_TARGET
{
	float sampleOffsets[5] = { 0.0, 1.4347826, 3.3478260, 5.2608695, 7.1739130 };
	float sampleWeights[5] = { 0.16818994, 0.27276957, 0.11690125, 0.024067905, 0.0021112196 };

	float4 color = tex2D(blurframeSampler, coord) * sampleWeights[0];
	for(int i = 1; i < 5; ++i) {
		//color += tex2D(blurframeSampler, coord + float2(sampleOffsets[i] * PIXEL_SIZE.x, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		//color += tex2D(blurframeSampler, coord - float2(sampleOffsets[i] * PIXEL_SIZE.x, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		//color += tex2D(blurframeSampler, coord + float2(-sampleOffsets[i] * PIXEL_SIZE.x, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		//color += tex2D(blurframeSampler, coord + float2(sampleOffsets[i] * PIXEL_SIZE.x, -sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		color += tex2D(blurframeSampler, coord + float2(0.0, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		color += tex2D(blurframeSampler, coord - float2(0.0, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		color += tex2D(blurframeSampler, coord + float2(sampleOffsets[i] * PIXEL_SIZE.x, 0.0)) * sampleWeights[i];
		color += tex2D(blurframeSampler, coord - float2(sampleOffsets[i] * PIXEL_SIZE.x, 0.0)) * sampleWeights[i];
	}
	return color * 0.5;
}

float4 LensPS(in float4 pos : SV_Position, in float2 coord : TEXCOORD) : SV_TARGET
{
	//float4 thres = tex2D(thresSampler, coord);
	float4 blur = tex2D(frameSampler, coord);
	float4 color = tex2D(ColorSampler, length(float2(0.5,0.5) - coord) / length(float2(0.5,0.5)));
	float4 ghosts;
	float4 halo;
	float4 features;
	
	float2 ghostVec = (float2(0.5, 0.5) - coord)*(1.0/(nGhosts/1.5));
	float2 haloVec = normalize(ghostVec) *.4;
	
	// Ghost pass
	for (int i = 0; i < nGhosts; i++)
	{
		float2 offset = frac(coord + ghostVec*float(i));
		
		float lenswgt = length(float2(0.5,0.5) - offset) / length(float2(0.5, 0.5));
		lenswgt = pow(1-lenswgt*.9+.1, 4.0);
		
		ghosts += tex2D(frameSampler, offset)*lenswgt;
	}
	
	// Halo Pass
	float halowgt = length(float2(0.5,0.5) - frac(coord+haloVec))/length(float2(0.5,0.5));
	halowgt = pow(1.0 - halowgt, 30.0);
	
	halo = tex2D(frameSampler, coord+haloVec)*halowgt;
	
	// Coloring pass
	ghosts = ghosts * (color*LensColorLevel+(1-LensColorLevel));
	
	// Combine Halo+Flares
	features = ghosts*BlurStrength;
	features += halo*HaloStrength;
	
	return features;
}

float4 CombinePS (in float4 pos : SV_Position, in float2 coord : TEXCOORD) : SV_TARGET
{
	float2 direction = normalize((float2(0.5, 0.5) - coord));
	float3 distortion = float3(-ChromaticAberrationStrength, 0.0, ChromaticAberrationStrength);
	
	// Quick chromatic aberration
	float4 lensflare =  float4(
							tex2D(frameSampler, coord + direction*distortion.x).x,
							tex2D(frameSampler, coord + direction*distortion.y).y,
							tex2D(frameSampler, coord + direction*distortion.z).z, 1.0);
							
	float4 original = tex2D(origframeSampler, coord);
	
	// Lens Dirt
	float4 lensDirt = tex2D(DirtSampler, coord);
	float4 lensStar = tex2D(StarSampler, coord);
	float4 lensMod = lensDirt+lensStar+.5;
	
	
	return original+lensflare*(lensMod*DirtStrength+(1-DirtStrength));
}

float4 PassThrough(in float4 pos : SV_Position, in float2 coord : TEXCOORD) : SV_TARGET
{
	return tex2D(frameSampler, coord);
}

technique t0 < enabled = true; toggle = 0x2D>
{
	pass PT
	{
		VertexShader = FrameVS;
		PixelShader = PassThrough;
		RenderTarget = origframeTex2D;
	}
	
	// Leveling pass
	pass LV
	{
		VertexShader = FrameVS;
		PixelShader = ThresholdPS;
	} 
	
	// LensFlare Gen pass
	pass LS
	{
		VertexShader = FrameVS;
		PixelShader = LensPS;
	}
	
	// Blur pass
	pass P1
	{
		VertexShader = FrameVS;
		PixelShader = HGaussianBlurPS;
	}
	pass P2
	{
		VertexShader = FrameVS;
		PixelShader = VGaussianBlurPS;
	}
	pass P3
	{
		VertexShader = FrameVS;
		PixelShader = SGaussianBlurPS;
	}
	pass P4
	{
		VertexShader = FrameVS;
		PixelShader = BGaussianBlurPS;
	}
	
	// Combine Pass
	pass CB
	{
		VertexShader = FrameVS;
		PixelShader = CombinePS;
	}
}