#ifndef _ROGUEVIZ_H_
#define _ROGUEVIZ_H_
// See: http://www.roguetemple.com/z/hyper/rogueviz.php

#include "../hyper.h"

namespace rogueviz {
  using namespace hr;

  enum eVizkind { kNONE, kAnyGraph, kTree, kSpiral, kSAG, kCollatz, kFullNet, kKohonen, kFlocking };
  extern eVizkind kind;

  extern bool on;
  void drawExtra();
  void close();

  void init();
  
  struct edgetype {
    double visible_from;
    double visible_from_hi;
    double visible_from_help;
    unsigned color, color_hi;
    string name;
    };
  
  static const unsigned DEFAULT_COLOR = 0x471293B5;

  extern edgetype default_edgetype;
  
  extern vector<shared_ptr<edgetype>> edgetypes;
    
  struct edgeinfo {
    int i, j;
    double weight, weight2;
    vector<glvertex> prec;
    basic_textureinfo tinf;
    cell *orig;
    int lastdraw;
    edgetype *type;
    edgeinfo(edgetype *t) { orig = NULL; lastdraw = -1; type = t; }
    };
  
  struct rvimage {
    basic_textureinfo tinf;
    texture::texture_data tdata;
    vector<hyperpoint> vertices;
    };

  struct colorpair {
    color_t color1, color2;
    char shade;
    shared_ptr<rvimage> img;
    colorpair(color_t col = 0xC0C0C0FF) { shade = 0; color1 = color2 = col; }
    };
  
  struct vertexdata {
    vector<pair<int, edgeinfo*> > edges;
    string name;
    colorpair cp;
    edgeinfo *virt;
    bool special;
    int data;
    string *info;
    shmup::monster *m;
    vertexdata() { virt = NULL; m = NULL; info = NULL; special = false; }
    };
  
  extern vector<vertexdata> vdata;
 
  void storeall(int from = 0);

  namespace anygraph {
    extern double R, alpha, T;
    extern vector<pair<double, double> > coords;
    
    void fixedges();
    void read(string fn, bool subdiv = true, bool doRebase = true, bool doStore = true);
    extern int N;
    }
  
  extern bool showlabels;

  extern bool rog3;
  extern bool rvwarp;

  extern colorpair dftcolor;
  
  inline hookset<void(vertexdata&, cell*, shmup::monster*, int)> *hooks_drawvertex;
  
  namespace collatz { 
    extern double s2, s3, p2, p3; 
    void start(); 
    void act(vertexdata&, cell*, shmup::monster*, int); 
    void lookup(long long reached, int bits);
    }
  void readcolor(const string& cfname);
  extern bool on;

  void close();
  extern bool showlabels;
  
  namespace rvtour {
    using namespace hr::tour;
    inline hookset<void(vector<slide>&)> *hooks_build_rvtour;
    slide *gen_rvtour();
    }

  void createViz(int id, cell *c, transmatrix at);
  }

#endif
