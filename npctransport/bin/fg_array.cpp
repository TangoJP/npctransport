/**
 * \file fg_array.cpp
 * \brief Simulate an fg and a kap interacting
 * Copyright 2007-2012 IMP Inventors. All rights reserved.
 */

#define IMP_NPC_MAIN
#include <IMP/npctransport/main.h>
#include <IMP/ParticleTuple.h>
#include <RMF/utility.h>


int main(int argc, char *argv[]) {
  using namespace IMP;
  using namespace IMP::npctransport;
  using namespace IMP::algebra;

  IMP_NPC_STARTUP(sd);
  RMF:: set_show_hdf5_errors(true);
  //  sd->add_interaction(type_of_float[0], type_of_fg[0]);
  //  sd->add_interaction(type_of_float[1], type_of_fg[0]);

  //place fg anchors in grid and don't optimize them
  BoundingBox2D base(Vector2D(sd->get_box().get_corner(0)[0],
                              sd->get_box().get_corner(0)[1]),
                     Vector2D(sd->get_box().get_corner(1)[0],
                              sd->get_box().get_corner(1)[1]));
  atom::Hierarchy root= sd->get_root();
  atom::Hierarchies chains =get_fg_chains(root);
  // create a set of random sites (for now)
  Vector2Ds sites;
  std::cout << base::Showable(sites) << std::endl;
  double r= core::XYZR(chains[0].get_child(0)).get_radius();
  std::cout << "Base is " << base << std::endl;
  do {
    // add a site that is not too close to an existing sites (at least 2*r)
    // - a site for every cain
    Vector2D cur=(get_random_vector_in(base));
    bool bad=false;
    for (unsigned int i=0; i< sites.size(); ++i) {
      if (get_distance(sites[i], cur) < 2*r) {
        bad=true;
        break;
      }
    }
    if (!bad) {
      sites.push_back(cur);
    }
  } while (sites.size() < chains.size());
  // anchor each fg chain to the (x,y) site (only by x,y coords,
  // all anchored to the same z plane)
  for (unsigned int i=0; i< chains.size(); ++i) {
    atom::Hierarchy r(chains[i]);
    core::XYZ d(r.get_child(0));
    d.set_coordinates(Vector3D(sites[i][0], sites[i][1],
                               sd->get_box().get_corner(0)[2]));
    d.set_coordinates_are_optimized(false);
  }
  IMP_NPC_LOOP(sd, IMP::ParticlePairsTemp());
  return 0;
}
