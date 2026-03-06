#include "viewport.h"
#include <iostream>
#include "CMU462.h"

namespace CMU462 {

void ViewportImp::set_viewbox( float centerX, float centerY, float vspan ) {

  Matrix3x3 norm_matrix = Matrix3x3::identity();
  this->vspan = vspan;
  this->centerX = centerX;
  this->centerY = centerY;

  float s = 1.0f / (2.0f * this->vspan);
  norm_matrix(0,0) = s;
  norm_matrix(1,1) = s;
  norm_matrix(0,2) = 0.5f - (this->centerX *  s );
  norm_matrix(1,2) = 0.5f - (this->centerY *  s);
  set_svg_2_norm(norm_matrix); 


}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) { 
  
  this->centerX -= dx;
  this->centerY -= dy;
  this->vspan *= scale;
  set_viewbox( centerX, centerY, vspan );
}

} // namespace CMU462
