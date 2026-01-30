#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462 {


// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  std::fill(this->ss_buffer.begin(), this->ss_buffer.end(), 255);

  // set top level transformation
  transformation = svg_2_screen;

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;
  // resize the super sampling buffer
  this->ss_buffer.resize(this->target_w * this->target_h * 4 * this->sample_rate * this->sample_rate);
  this->ss_buffer_h = this->target_h * this->sample_rate;
  this->ss_buffer_w = this->target_w * this->sample_rate;
  // fill the super smapling buffer with default values aka clearing them
  std::fill(this->ss_buffer.begin(), this->ss_buffer.end(), 255);

}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;
  this->set_sample_rate(this->sample_rate); // call this set up super sampling buffers

}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

  // Task 5 (part 1):
  // Modify this to implement the transformation stack

  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);
 
  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color, bool sampling) {

  // fill in the nearest pixel
  int sx = (int) floor(x);
  int sy = (int) floor(y);

  // offsets in the super sampling buffer
  if (sampling == false){
    //std::cout << "Inside the sample scaling..." << std::endl;
    sx *= this->sample_rate;
    sy *= this->sample_rate;
  }

  // check bounds
  if ( sx < 0 || sx >= this->ss_buffer_w ) return;
  if ( sy < 0 || sy >= this->ss_buffer_h ) return;

  //std::cout << "x, y: " << x << ", " << y << std::endl;
  //std::cout << "sx, sy: " << sx << ", " << sy << std::endl;
  //std::cout << "Color: " << color << std::endl;
  //std::cout << "Index: " << 4 * ((sx * this->sample_rate) + ((sy * this->sample_rate) * this->target_w)) << std::endl;


  //int ssx = sx * this->sample_rate;
  //int ssy = sy * this->sample_rate;

  if (sampling == false){
    for(int ssy = 0; ssy < this->sample_rate; ssy++){
        for(int ssx = 0; ssx < this->sample_rate; ssx++){
          int ind = 4 * ((sx + ssx) + (sy + ssy) * ss_buffer_w);
          ss_buffer[ind    ] = (uint8_t) (color.r * 255);
          ss_buffer[ind + 1] = (uint8_t) (color.g * 255);
          ss_buffer[ind + 2] = (uint8_t) (color.b * 255);
          ss_buffer[ind + 3] = (uint8_t) (color.a * 255);
      }
    }
  }
  else {
    int ind = 4 * (sx + sy * ss_buffer_w);
    ss_buffer[ind    ] = (uint8_t) (color.r * 255);
    ss_buffer[ind + 1] = (uint8_t) (color.g * 255);
    ss_buffer[ind + 2] = (uint8_t) (color.b * 255);
    ss_buffer[ind + 3] = (uint8_t) (color.a * 255);
  }

  // for(int h = 0; h < this->sample_rate; h++){
  //   for(int w = 0; w < this->sample_rate; w++){
  //     int ssx = sx * this->sample_rate + w;
  //     int ssy = sy * this->sample_rate + h;

  //     int ind = 4 * (ssx + ssy * ss_buffer_w);
  //     ss_buffer[ind    ] = (uint8_t) (color.r * 255);
  //     ss_buffer[ind + 1] = (uint8_t) (color.g * 255);
  //     ss_buffer[ind + 2] = (uint8_t) (color.b * 255);
  //     ss_buffer[ind + 3] = (uint8_t) (color.a * 255);

  //   }
  // }

  

  // ss_buffer[4 * ((sx * this->sample_rate) + ((sy * this->sample_rate) * this->target_w))    ] = (uint8_t) (color.r * 255);
  // ss_buffer[4 * ((sx * this->sample_rate) + ((sy * this->sample_rate) * this->target_w)) +1 ] = (uint8_t) (color.g * 255);
  // ss_buffer[4 * ((sx * this->sample_rate) + ((sy * this->sample_rate) * this->target_w)) +2 ] = (uint8_t) (color.b * 255);
  // ss_buffer[4 * ((sx * this->sample_rate) + ((sy * this->sample_rate) * this->target_w)) +3 ] = (uint8_t) (color.a * 255);


  // fill sample - NOT doing alpha blending!
  // render_target[4 * (sx + sy * target_w)    ] = (uint8_t) (color.r * 255);
  // render_target[4 * (sx + sy * target_w) + 1] = (uint8_t) (color.g * 255);
  // render_target[4 * (sx + sy * target_w) + 2] = (uint8_t) (color.b * 255);
  // render_target[4 * (sx + sy * target_w) + 3] = (uint8_t) (color.a * 255);

}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {

  // Task 2: 
  // Implement line rasterization

  x0 = x0 * this->sample_rate;
  x1 = x1 * this->sample_rate;
  y0 = y0 * this->sample_rate;
  y1 = y1 * this->sample_rate;

  float dx = x1 - x0;
  float dy = y1 - y0;
  if(std::abs(dx) > std::abs(dy)) {
    if(x0 > x1){
      this->rasterize_line_x(x1, y1, x0, y0, color);
    } else{
      this->rasterize_line_x(x0, y0, x1, y1, color);
    }
  }
  else{
    if(y0 > y1){
      this->rasterize_line_y(x1, y1, x0, y0, color);
    } else{
      this->rasterize_line_y(x0, y0, x1, y1, color);
    }
  }
  

  

}

void SoftwareRendererImp::rasterize_line_x( float x0, float y0,
                                            float x1, float y1,
                                            Color color){

    // x0 = x0 * this->sample_rate;
    // x1 = x1 * this->sample_rate;
    // y0 = y0 * this->sample_rate;
    // y1 = y1 * this->sample_rate;
    
    int dx = x1 - x0;
    int dy = y1 - y0;
    int eps = 0;
    int y =  y0;

    int dir = dy < 0 ? -1 : 1;
    dy *= dir;

    for(int x = (int) x0; x <= x1; x++){
      for(int sy = 0; sy < this->sample_rate; sy++){
        for(int sx = 0; sx < this->sample_rate; sx++){
          this->rasterize_point(x + sx, y + sy, color, true);
      }
    }
    eps += dy;
    if((2 * eps) >= dx){
      y += dir;
      eps -= dx;
    }
  }
                                          
}

void SoftwareRendererImp::rasterize_line_y( float x0, float y0,
                                            float x1, float y1,
                                            Color color){
    // x0 = x0 * this->sample_rate;
    // x1 = x1 * this->sample_rate;
    // y0 = y0 * this->sample_rate;
    // y1 = y1 * this->sample_rate;

    int dx = x1 - x0;
    int dy = y1 - y0;
    int eps = 0;
    int x = floor(x0);

    int dir = dx < 0 ? -1 : 1;
    dx *= dir;

    for(int y = (int) y0; y <= y1; y++){
      //this->rasterize_point(x, y, color, true);
      for(int sy = 0; sy < this->sample_rate; sy++){
        for(int sx = 0; sx < this->sample_rate; sx++){
          this->rasterize_point(x + sx, y + sy , color, true);
        }
      }
      eps += dx;
      if((2 * eps) >= dy){
        x += dir;
        eps -= dy;  
    }
  }
                                          
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 3: 
  // Implement triangle rasterization

  x0 = x0 * this->sample_rate;
  x1 = x1 * this->sample_rate;
  x2 = x2 * this->sample_rate;
  y0 = y0 * this->sample_rate;
  y1 = y1 * this->sample_rate;
  y2 = y2 * this->sample_rate;
  
  float lx, rx, ty, by;
  lx = rx = x0;
  ty = by = y0; // rectangle around triangle
  

  // find the left most x-ordinate
  if(lx > x1) lx = x1;
  if(lx > x2) lx = x2;

  // find the right most x-ordinate
  if(rx < x1) rx = x1;
  if(rx < x2) rx = x2;

  // find the bottom most y-ordinate
  if(by > y1) by = y1;
  if(by > y2) by = y2;

  // find the top most y-ordinate
  if(ty < y1) ty = y1;
  if(ty < y2) ty = y2;

  // iterate over the rectangle enclosing the triangle
  for(int y = floor(by); y <= ty; y += this->sample_rate){
    for(int x = floor(lx); x <= rx; x += this->sample_rate){
      //float px = x;
      //float py = y;
      for(int sy = 0; sy < this->sample_rate; sy++){
        for(int sx = 0; sx < this->sample_rate; sx++){
          //this->rasterize_point(x + sx, y + sy, color, true);
          //px += 1.0f * sx / this->sample_rate;
          //py += 1.0f * sy / this->sample_rate; 
          int px = x + sx ;
          int py = y + sy ;
          int p1 = ((x1 - x0) * (py - y0)) - ((y1 - y0) * (px - x0));
          int p2 = ((x2 - x1) * (py - y1)) - ((y2 - y1) * (px - x1));
          int p3 = ((x0 - x2) * (py - y2)) - ((y0 - y2) * (px - x2));


          if((p1 <= 0 && p2 <= 0 && p3 <=   0) || (p1 >= 0 && p2 >= 0 && p3 >= 0)){
            this->rasterize_point(px, py, color, true);
          }
          else{
            //this->rasterize_point(x,y, Color(0, 256, 256, 1));
          }

          
        }
      }
      
      // int p1 = ((x1 - x0) * (y - y0)) - ((y1 - y0) * (x - x0));
      // int p2 = ((x2 - x1) * (y - y1)) - ((y2 - y1) * (x - x1));
      // int p3 = ((x0 - x2) * (y - y2)) - ((y0 - y2) * (x - x2));


      // if((p1 <= 0 && p2 <= 0 && p3 <=   0) || (p1 >= 0 && p2 >= 0 && p3 >= 0)){
      //   this->rasterize_point(x, y, color, true);
      // }
      // else{
      //   //this->rasterize_point(x,y, Color(0, 256, 256, 1));
      // }
      
    }
  }
  

}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 6: 
  // Implement image rasterization

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 4: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 4".
  //clear_target();
  //std::cout << "Dimensions: " << this->target_w << " x " << this->target_h << std::endl; 
  for(int y = 0; y < this->target_h; y++){
    for(int x = 0; x < this->target_w; x++){
      //std::cout << "x: " << x << " y: " << y << std::endl;
     
      int ind_r = 4 * (x + y * this->target_w);
      Color color_s(0,0,0,0);
      //std::cout << "Color: " << color_s.r << " " << color_s.g << " " << color_s.b << " " << color_s.a << std::endl;

      for(int sy = 0; sy < this->sample_rate; sy++){
        for(int sx = 0; sx < this->sample_rate; sx++){
          int ssx = x * this->sample_rate + sx;
          int ssy = y * this->sample_rate + sy;

          int ind_s = 4 * (ssx + ssy * this->ss_buffer_w);
          color_s.r += ss_buffer[ind_s     ];
          color_s.g += ss_buffer[ind_s  + 1];
          color_s.b += ss_buffer[ind_s  + 2];
          color_s.a += ss_buffer[ind_s  + 3];

        }
      }
      float avg_s = 1.0f / (this->sample_rate * this->sample_rate);
      //std::cout << "Color after addition: " << color_s.r * avg_s << " " << color_s.g * avg_s << " " << color_s.b * avg_s << " " << color_s.a * avg_s << std::endl;
      this->render_target[ind_r    ] = (uint8_t) (color_s.r * avg_s);
      this->render_target[ind_r + 1] = (uint8_t) (color_s.g * avg_s);
      this->render_target[ind_r + 2] = (uint8_t) (color_s.b * avg_s);
      this->render_target[ind_r + 3] = (uint8_t) (color_s.a * avg_s);

    }
  }

}


} // namespace CMU462
