 #include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE: 
  // This starter code allocates the mip levels and generates a level 
  // map by filling each level with placeholder data in the form of a 
  // color that differs from its neighbours'. You should instead fill
  // with the correct data!

  // Task 7: Implement this

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level"; 
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

    std::cout << "Level: " << i << " " << level.width << " " << level.height << " " << level.texels.size() << std::endl;

    for(int y = 0; y < level.height; y++){
      for(int x = 0; x < level.width; x++){
       
        // int trans_y = y * (tex.mipmap[0].height - level.height);
        // int trans_x = x * (tex.mipmap[0].width - level.width);

        int trans_y = y * (tex.mipmap[0].height / level.height);
        int trans_x = x * (tex.mipmap[0].width / level.width);

        int col_ind = (x * 4) + (y * 4 * level.width); 
        int trans_col_ind = (trans_x * 4) + (trans_y * 4  * tex.mipmap[0].width);

        std::cout << "x, y: " << x << ", " <<  y << " | trans_x, trans_y: "  << trans_x << ", " << trans_y << " | col_ind, trans_col_ind: "  << col_ind << ", " << trans_col_ind << std::endl;
    
        level.texels[col_ind + 0] = tex.mipmap[0].texels[trans_col_ind + 0];
        level.texels[col_ind + 1] = tex.mipmap[0].texels[trans_col_ind + 1];
        level.texels[col_ind + 2] = tex.mipmap[0].texels[trans_col_ind + 2];
        level.texels[col_ind + 3] = tex.mipmap[0].texels[trans_col_ind + 3];
    
      }
    }
 
  } 

  // fill all 0 sub levels with interchanging colors (JUST AS A PLACEHOLDER)
  // Color colors[3] = { Color(1,0,0,1), Color(0,1,0,1), Color(0,0,1,1) };
  // for(size_t i = 1; i < tex.mipmap.size(); ++i) {

  //   Color c = colors[i % 3];
  //   MipLevel& mip = tex.mipmap[i];

  //   for(size_t i = 0; i < 4 * mip.width * mip.height; i += 4) {
  //     float_to_uint8( &mip.texels[i], &c.r );
  //   }
  // }

}

Color Sampler2DImp::sample_nearest(Texture& tex, 
                                   float u, float v, 
                                    int level) {

  // Task 6: Implement nearest neighbour interpolation

  // pixel coordinates
  int x = floor(u); 
  int y = floor(v);
  
  // fetching colors
  float r = tex.mipmap[level].texels[(x * 4 * tex.mipmap[level].width) + (y * 4) + 0] / 255.0f; 
  float g = tex.mipmap[level].texels[(x * 4 * tex.mipmap[level].width) + (y * 4) + 1] / 255.0f; 
  float b = tex.mipmap[level].texels[(x * 4 * tex.mipmap[level].width) + (y * 4) + 2] / 255.0f;  
  float a = tex.mipmap[level].texels[(x * 4 * tex.mipmap[level].width) + (y * 4) + 3] / 255.0f;
                                
  return Color(r, g, b, a);

}

Color Sampler2DImp::sample_bilinear(Texture& tex, 
                                    float u, float v, 
                                    int level) {
  
  // Task 6: Implement bilinear filtering

  // pixel coordinates
  int x0 = floor(u);
  int y0 = floor(v);

  Color colors[4]; // 4 pixels to be fetched
  
  // distance b/w pixel and point
  float s = u - (x0); 
  float t = v - (y0); 

  // iterating over the 4 pixels to be fetched from texture
  for(int y = 0; y < 2 && (y0 + y) < tex.mipmap[level].height; y++){
    for(int x = 0; x < 2  && (x0 + x) < tex.mipmap[level].width; x++){
      
      // fetching colors
      float r = tex.mipmap[level].texels[((x0 + x) * 4 ) + ((y0 + y) * 4 * tex.mipmap[level].width) + 0] / 255.0f; 
      float g = tex.mipmap[level].texels[((x0 + x) * 4) + ((y0 + y) * 4 * tex.mipmap[level].width) + 1] / 255.0f; 
      float b = tex.mipmap[level].texels[((x0 + x) * 4) + ((y0 + y) * 4 * tex.mipmap[level].width) + 2] / 255.0f;  
      float a = tex.mipmap[level].texels[((x0 + x) * 4) + ((y0 + y) * 4 * tex.mipmap[level].width) + 3] / 255.0f;

      // storing em'
      colors[x + (2 * y)] = Color(r, g, b, a);

    }
  }
  
  // applying bilinear interpolation
  Color interp_color =  ( 
                          (
                            (1 - t) * 
                            ( 
                              ((1-s) * colors[0]) + 
                              (s * colors[1])
                            )
                          ) +
                          (
                            t *
                            (
                              ((1-s) * colors[2]) +
                              (s * colors[3])
                            )
                          )
                        );

  return interp_color;

}

Color Sampler2DImp::sample_trilinear(Texture& tex, 
                                     float u, float v, 
                                     float u_scale, float v_scale) {

  // Task 7: Implement trilinear filtering
  
  // pixel coordinates
  int x0 = floor(u);
  int y0 = floor(v);

  Color colors[4]; // 4 pixels to be fetched
  
  // distance b/w pixel and point
  float s = u - (x0); 
  float t = v - (y0); 

  // iterating over the 4 pixels to be fetched from texture
  for(int y = 0; y < 2 && (y0 + y) < tex.height; y++){
    for(int x = 0; x < 2  && (x0 + x) < tex.width; x++){
      
      // fetching colors
      float r = tex.mipmap[0].texels[((x0 + x) * 4 ) + ((y0 + y) * 4 * tex.width) + 0] / 255.0f; 
      float g = tex.mipmap[0].texels[((x0 + x) * 4) + ((y0 + y) * 4 * tex.width) + 1] / 255.0f; 
      float b = tex.mipmap[0].texels[((x0 + x) * 4) + ((y0 + y) * 4 * tex.width) + 2] / 255.0f;  
      float a = tex.mipmap[0].texels[((x0 + x) * 4) + ((y0 + y) * 4 * tex.width) + 3] / 255.0f;

      // storing em'
      colors[x + (2 * y)] = Color(r, g, b, a);

    }
  }
  
  // applying bilinear interpolation
  Color interp_color =  ( 
                          (
                            (1 - t) * 
                            ( 
                              ((1-s) * colors[0]) + 
                              (s * colors[1])
                            )
                          ) +
                          (
                            t *
                            (
                              ((1-s) * colors[2]) +
                              (s * colors[3])
                            )
                          )
                        );

  return interp_color;

}

} // namespace CMU462
