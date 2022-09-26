namespace hr {

namespace ads_game {

vector<ld> shape_disk;

void set_default_keys();

/* In DS, we also use ads_matrix, but the meaning of the shift parameter is different:
 * 
 */

vector<unique_ptr<ads_object>> rocks;

int ds_rocks = 500;
bool mark_origin = false;

ads_object *main_rock;

void init_ds_game() {

  /* create the main rock first */
  if(1) {
    dynamicval<eGeometry> g(geometry, gSpace435);
    auto r = std::make_unique<ads_object> (oRock, nullptr, ads_matrix(Id, 0), 0xFFD500FF);
    r->shape = &shape_gold;
    main_rock = &*r;
    rocks.emplace_back(std::move(r));
    }

  for(int i=0; i<500; i++) {
    hyperpoint h = random_spin3() * C0;
    println(hlog, "h = ", h);

    transmatrix T = gpushxto0(h);
    dynamicval<eGeometry> g(geometry, gSpace435);
    for(int i=0; i<4; i++) T[i][3] = T[3][i] = i == 3;
    transmatrix worldline = inverse(T);
    worldline = worldline * spin(randd() * TAU);
    worldline = worldline * lorentz(0, 3, randd());

    auto r = std::make_unique<ads_object> (oRock, nullptr, ads_matrix(worldline, 0), 0xFFFFFFFF);
    r->shape = &shape_disk;
    rocks.emplace_back(std::move(r));
    }
  shape_disk.clear();
  for(int d=0; d<=360; d += 15) {
    shape_disk.push_back(sin(d*degree) * 0.1 * scale);
    shape_disk.push_back(cos(d*degree) * 0.1 * scale);
    }
  }

void ds_gen_particles(int qty, transmatrix from, ld shift, color_t col, ld spd, ld t, ld spread = 1) {
  for(int i=0; i<qty; i++) {
    auto r = std::make_unique<ads_object>(oParticle, nullptr, ads_matrix(from * spin(randd() * TAU * spread) * lorentz(0, 3, (.5 + randd() * .5) * spd), shift), col );
    r->shape = &shape_particle;
    r->life_end = randd() * t;
    r->life_start = 0;
    rocks.emplace_back(std::move(r));
    }
  }

void ds_handle_crashes() {
  if(paused) return;
  vector<ads_object*> dmissiles;
  vector<ads_object*> drocks;
  for(auto m: displayed) {
    if(m->type == oMissile)
      dmissiles.push_back(m);
    if(m->type == oRock)
      drocks.push_back(m);
    }

  for(auto m: dmissiles) {
    hyperpoint h = kleinize(m->pt_main.h);
    for(auto r: drocks) {
      if(pointcrash(h, r->pts)) {
        m->life_end = m->pt_main.shift;
        r->life_end = r->pt_main.shift;
        dynamicval<eGeometry> g(geometry, gSpace435);
        ds_gen_particles(rpoisson(crash_particle_qty), m->at.T * lorentz(2, 3, m->life_end), m->at.shift, missile_color, crash_particle_rapidity, crash_particle_life);
        ds_gen_particles(rpoisson(crash_particle_qty), r->at.T * lorentz(2, 3, r->life_end), r->at.shift, r->col, crash_particle_rapidity, crash_particle_life);
        pdata.score++;
        int qty = 2 + rpoisson(1);
        for(int i=0; i<qty; i++) {
          auto r1 = std::make_unique<ads_object> (oRock, nullptr, ads_matrix(r->at.T * lorentz(2, 3, r->life_end) * spin(randd() * TAU) * lorentz(0, 3, randd() * ds_split_speed), 0), 0xFFFFFFFF);
          r1->shape = &shape_disk;
          r1->life_start = 0;
          rocks.emplace_back(std::move(r1));
          }
        break;
        }
      }
    }
  }

void ds_fire() {
  if(!pdata.ammo) return;
  pdata.ammo--;
  dynamicval<eGeometry> g(geometry, gSpace435);

  transmatrix S0 = inverse(current.T) * spin(ang*degree);

  transmatrix S1 = S0 * lorentz(0, 3, missile_rapidity);

  auto r = std::make_unique<ads_object> (oMissile, nullptr, ads_matrix(S1, current.shift), 0xC0C0FFFF);
  r->shape = &shape_missile;
  r->life_start = 0;

  rocks.emplace_back(std::move(r));
  }

bool ds_turn(int idelta) {
  multi::handleInput(idelta);
  ld delta = idelta / anims::period;
  
  if(!(cmode & sm::NORMAL)) return false;

  ds_handle_crashes();

  auto& a = multi::actionspressed;
  auto& la = multi::lactionpressed;

  if(a[16+4] && !la[16+4] && !paused) ds_fire();
  if(a[16+5] && !la[16+5]) {
    paused = !paused;
    if(paused) {
      current_ship = current;
      view_pt = 0;
      }
    else {
      current = current_ship;
      }
    }

  if(a[16+6] && !la[16+6]) view_proper_times = !view_proper_times;
  if(a[16+7] && !la[16+7]) auto_rotate = !auto_rotate;
  if(a[16+8] && !la[16+8]) pushScreen(game_menu);

  if(true) {
    dynamicval<eGeometry> g(geometry, gSpace435);
    
    ld pt = delta * simspeed;
    ld mul = read_movement();

    if(paused && a[16+11]) {
      current.T = spin(ang*degree) * cspin(0, 2, mul*delta*-pause_speed) * spin(-ang*degree) * current.T;
      }
    else {
      current.T = spin(ang*degree) * lorentz(0, 3, -delta*accel*mul) * spin(-ang*degree) * current.T;
      }
    
    if(!paused) {
      pdata.fuel -= delta*accel*mul;
      ds_gen_particles(rpoisson(delta*accel*mul*fuel_particle_qty), inverse(current.T) * spin(ang*degree+M_PI) * rots::uxpush(0.06 * scale), current.shift, rsrc_color[rtFuel], fuel_particle_rapidity, fuel_particle_life, 0.02);
      }

    ld tc = 0;
    if(!paused) tc = pt;
    else if(a[16+9]) tc = pt;
    else if(a[16+10]) tc = -pt;

    if(!paused && !game_over) {
      shipstate ss;
      ss.at.T = inverse(current.T) * spin(ang*degree);
      ss.at.shift = current.shift;
      ss.start = ship_pt;
      ss.duration = pt;
      ss.ang = ang;
      history.emplace_back(ss);
      }
    
    current.T = lorentz(3, 2, -tc) * current.T;
    
    auto& mshift = main_rock->pt_main.shift;
    if(mshift) {
      current.shift += mshift;
      current.T = current.T * lorentz(2, 3, mshift);
      mshift = 0;
      }
    fixmatrix(current.T);
    
    if(!paused) {
      ship_pt += pt;
      pdata.oxygen -= pt;
      if(pdata.oxygen < 0) {
        pdata.oxygen = 0;
        game_over = true;
        }
      }
    else view_pt += tc;

    if(a[16+4] && !la[16+4] && false) {
      if(history.size())
        history.back().duration = HUGE_VAL;
      current = random_spin3();
      }
    }

  return true;
  }

hyperpoint pov = point30(0, 0, 1);

cross_result ds_cross0(transmatrix T) {
  // h = T * (0 0 cosh(t) sinh(t))
  // h[3] == 0
  // T[3][2] * cosh(t) + T[3][3] * sinh(t) = 0
  // T[3][2] + T[3][3] * tanh(t) = 0
  ld tt = - T[3][2] / T[3][3];
  if(tt < -1 || tt > 1) return cross_result{ Hypc, 0 };
  cross_result res;
  ld t = atanh(tt);
  res.shift = t;
  res.h = T * hyperpoint(0, 0, cosh(t), sinh(t));
  return res;
  }

cross_result ds_cross0_light(transmatrix T) {
  // h = T * (t 0 1 t); h[3] == 0
  ld t = T[3][2] / -(T[3][0] + T[3][3]);
  cross_result res;
  res.shift = t;
  res.h = T * hyperpoint(t, 0, 1, t);
  return res;
  }

transmatrix tpt(ld x, ld y) {
  return cspin(0, 2, x * scale) * cspin(1, 2, y * scale);
  }

void view_ds_game() {
  displayed.clear();

  main_rock->at.shift = current.shift;

  if(1) {
    make_shape();
    
    for(auto& r: rocks) {
      auto& rock = *r;
      poly_outline = 0xFF;
      if(rock.at.shift < current.shift - 4 * TAU) continue;
      if(rock.at.shift > current.shift + 4 * TAU) continue;
      
      if(1) {
        dynamicval<eGeometry> g(geometry, gSpace435);
        transmatrix at = current.T * lorentz(2, 3, rock.at.shift - current.shift) * rock.at.T;
        rock.pt_main = ds_cross0(at);
        
        if(rock.pt_main.shift < rock.life_start) continue;
        if(rock.pt_main.shift > rock.life_end) continue;

        transmatrix at1 = at * lorentz(2, 3, rock.pt_main.shift);
        rock.pts.clear();
        
        auto& sh = *rock.shape;

        for(int i=0; i<isize(sh); i+=2) {
          transmatrix at2 = at1 * tpt(sh[i], sh[i+1]);
          auto cr1 = ds_cross0(at2);
          rock.pts.push_back(cr1);
          }
        }
      
      vector<hyperpoint> circle_flat;
      for(auto c: rock.pts) circle_flat.push_back(c.h / (1 + c.h[2]));
      
      ld area = 0;
      for(int i=0; i<isize(circle_flat)-1; i++)
        area += (circle_flat[i] ^ circle_flat[i+1]) [2];
      
      if(area > 0) continue;

      // queuepolyat(shiftless(rgpushxto0(cr.h)), cgi.shGem[0], 0xFFFFFFF, PPR::LINE);
      for(auto p: rock.pts) curvepoint(p.h);
      color_t out = rock.col;
      queuecurve(shiftless(Id), out, rock.col, PPR::LINE);

      if(view_proper_times && rock.type != oParticle) {
        ld t = rock.pt_main.shift;
        if(&rock == main_rock) t += current.shift;
        string str = format(tformat, t / time_unit);
        queuestr(shiftless(rgpushxto0(rock.pt_main.h)), .1, str, 0xFFFF00, 8);
        }
      
      if(rock.pt_main.h[2] > 0.1 && rock.life_end == HUGE_VAL) {
        displayed.push_back(&rock);
        }
      }      

    ld delta = paused ? 1e-4 : -1e-4;
    for(auto& ss: history) {
      if(ss.at.shift < current.shift - 4 * TAU) continue;
      if(ss.at.shift > current.shift + 4 * TAU) continue;
      dynamicval<eGeometry> g(geometry, gSpace435);
      cross_result cr = ds_cross0(current.T * lorentz(2, 3, ss.at.shift - current.shift) * ss.at.T);
      if(cr.shift < delta) continue;
      if(cr.shift > ss.duration + delta) continue;
      transmatrix at = current.T * lorentz(2, 3, cr.shift) * ss.at.T;
      
      vector<hyperpoint> pts;
      
      auto& shape = shape_ship;
      for(int i=0; i<isize(shape); i += 2) {
        transmatrix at1 = at * tpt(shape[i], shape[i+1]);
        pts.push_back(ds_cross0(at1).h);
        }
      
      geometry = g.backup;
      for(auto pt: pts) curvepoint(pt);
      queuecurve(shiftless(Id), 0xFF, shipcolor, PPR::LINE);

      if(view_proper_times) {
        string str = format(tformat, (cr.shift + ss.start) / time_unit);
        queuestr(shiftless(rgpushxto0(cr.h)), .1, str, 0xC0C0C0, 8);
        }
      }

    if(!game_over && !paused) {
      poly_outline = 0xFF;
      if(ship_pt < invincibility_pt) {
        ld u = (invincibility_pt-ship_pt) / how_much_invincibility;
        poly_outline = gradient(shipcolor, rsrc_color[rtHull], 0, 0.5 + cos(5*u*TAU), 1);
        }
      queuepolyat(shiftless(spin(ang*degree) * Id), shShip, shipcolor, PPR::LINE);
      poly_outline = 0xFF;

      if(view_proper_times) {
        string str = format(tformat, ship_pt / time_unit);
        queuestr(shiftless(Id), .1, str, 0xFFFFFF, 8);
        }
      }
    
    if(paused && view_proper_times) {
      string str = format(tformat, view_pt / time_unit);
      queuestr(shiftless(Id), .1, str, 0xFFFF00, 8);
      }

    if(paused && !game_over) {
      vector<hyperpoint> pts;
      int ok = 0, bad = 0;
      for(int i=0; i<=360; i++) {
        dynamicval<eGeometry> g(geometry, gSpace435);
        auto h = inverse(current_ship.T) * spin(i*degree);
        auto cr = ds_cross0_light(current.T * lorentz(2, 3, current_ship.shift - current.shift) * h);
        pts.push_back(cr.h);
        if(cr.shift > 0) ok++; else bad++;
        }
      if(bad == 0) {
        for(auto h: pts) curvepoint(h);
        queuecurve(shiftless(Id), 0xFF0000C0, 0x00000060, PPR::SUPERLINE);
        }
      }
    }
  }

void run_ds_game() {

  set_default_keys();
  init_ds_game();

  rogueviz::rv_hook(hooks_frame, 100, view_ds_game);
  rogueviz::rv_hook(shmup::hooks_turn, 0, ds_turn);
  rogueviz::rv_hook(hooks_prestats, 100, display_rsrc);
  
  if(true) {
    dynamicval<eGeometry> g(geometry, gSpace435);
    current = Id;
    }

  ship_pt = 0;
  init_rsrc();
  
  rogueviz::rv_change(nohelp, true);
  rogueviz::rv_change(nomenukey, true);
  rogueviz::rv_change(nomap, true);
  rogueviz::rv_change(no_find_player, true);
  rogueviz::rv_change(showstartmenu, false);
  rogueviz::rv_hook(hooks_handleKey, 0, handleKey);
  }

void ds_record() {
  ld full = anims::period;
  anims::period = full * history.back().start / simspeed;
  anims::noframes = anims::period * 60 / 1000;
  dynamicval<bool> b(paused, true);
  int a = addHook(anims::hooks_anim, 100, [&] {
    view_pt = (ticks / full) * simspeed;
    for(auto& ss: history)
      if(ss.start + ss.duration > view_pt) {
        if(sphere) {
          dynamicval<eGeometry> g(geometry, gSpace435);
          current.shift = ss.at.shift;
          current.T = inverse(ss.at.T * spin(-ss.ang*degree));
          current.T = lorentz(3, 2, view_pt - ss.start) * current.T;
          }
        else PIA([&] {
          current = ads_inverse(ss.at * spin(-ss.ang*degree));
          vctr = ss.vctr;
          vctrV = ss.vctrV;
          current.T = cspin(3, 2, view_pt - ss.start) * current.T;
          if(auto_rotate)
            current.T = cspin(1, 0, view_pt - ss.start) * current.T;
          });
        break;
        }
    });
  anims::record_video_std();
  delHook(anims::hooks_anim, a);
  }

auto ds_hooks = 
  arg::add3("-ds-game", run_ds_game)
+ arg::add3("-ds-record", ds_record)
;

}
}