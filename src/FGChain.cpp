/**
 *  \file FGChain.cpp
 *  \brief description.
 *
 *  Copyright 2007-2014 IMP Inventors. All rights reserved.
 *
 */

#include <IMP/npctransport/FGChain.h>
#include <IMP/npctransport/internal/TAMDChain.h>
#include <IMP/atom/Diffusion.h>
#include <IMP/atom/Hierarchy.h>
#include <IMP/atom/CenterOfMass.h>
#include <IMP/atom/Mass.h>
#include <IMP/atom/TAMDParticle.h>
#include <IMP/Pointer.h>
#include <IMP/nullptr.h>
#include <IMP/core/ChildrenRefiner.h>
#include <IMP/core/rigid_bodies.h>
#include <IMP/core/XYZR.h>
#include <IMP/display/Colored.h>

#include <boost/tuple/tuple.hpp>
#include <cmath>
#include <cstdio>
#include <sstream>

IMPNPCTRANSPORT_BEGIN_NAMESPACE

namespace {

  void add_chain_and_restraints_to_sd
    (FGChain const* chain,
     SimulationData* sd,
     atom::Hierarchy parent,
     const ::npctransport_proto::Assignment_FGAssignment &fg_data)
  {
    parent.add_child(atom::Hierarchy(chain->root));

    // add chain backbone restraint
    double rlf = fg_data.rest_length_factor().value();
    core::ParticleType type(fg_data.type());
    sd->get_scoring()->add_chain_restraint
      ( chain->beads, rlf, type.get_string() + "chain restraint" );

    if(fg_data.is_tamd()) {
      // add tamd restraints and image particles
      internal::TAMDChain const* tamd_chain =
        dynamic_cast<internal::TAMDChain const*>(chain);
      IMP_USAGE_CHECK(tamd_chain, "chain is not TAMDChain*");
      for(unsigned int k = 0; k < tamd_chain->images.size(); k++) {
        atom::Hierarchy h_k( tamd_chain->images[k]);
        sd->get_root().add_child( h_k );
      }
      sd->get_scoring()->add_custom_restraints(tamd_chain->R);
    }
  }

} // anonymous namespace

FGChain* create_fg_chain
( SimulationData *sd,
  atom::Hierarchy parent,
  const ::npctransport_proto::Assignment_FGAssignment &fg_data,
  display::Color c )
{
  base::Pointer<FGChain> ret_chain = nullptr;

  bool DEBUG=false;
  // set up factory for chain particles:
  core::ParticleType type(fg_data.type());
  int n = fg_data.number_of_beads().value();
  double radius = fg_data.radius().value();
  double D_factor = fg_data.d_factor().value();
  double angular_D_factor = sd->get_angular_d_factor();
  IMP_NEW( ParticleFactory, pf,
           (sd, radius, D_factor, angular_D_factor, c, type) );

  // create TAMD or non-TAMD particles P, within hierarchy of root:
  Particles P; Particle* root;
  if(fg_data.is_tamd() || DEBUG) {
    int d = 2; // outdegree in TAMD hierarchy, TODO: parametrize
    int n_levels = ceil(log(n)/log(d)); // log_d{n}
    std::vector<double> T_factors(n_levels); // temperature scaling
    std::vector<double> F_factors(n_levels); // friction scaling
    std::vector<double> Ks(n_levels); // TAMD spring constant
    for(int i=0; i < n_levels; i++) { // i ~ increasing depth from root
      int level = n_levels - i; // level above leaves
      T_factors[i] = 3 * pow(2,level-1);
      F_factors[i] = 15 * pow(3,level-1);
      Ks[i] = 10;
    }
    ret_chain=
      internal::create_tamd_chain(pf, n, d, T_factors, F_factors, Ks);
  } else {
    Particles P;
    for (int i = 0; i < n; ++i) {
      P.push_back( pf->create() );
    }
    root = atom::Hierarchy::setup_particle
      ( new Particle( sd->get_model() ), P );
    root->set_name( type.get_string() );
    ret_chain = new FGChain(root, P);
  }

  // add chain beads, restraints, anchors
  // TODO: put in a different method for clarity
  add_chain_and_restraints_to_sd(ret_chain, sd, parent, fg_data);

  return ret_chain.release();
}

// gets a chain structure from a root of an FG nup
// (by adding its ordered leaves)
FGChain* get_fg_chain(atom::Hierarchy root){
  Particle* proot = root.get_particle();
  Particles beads = atom::get_leaves(root);
  IMP_NEW(FGChain, ret,
          (proot, beads) );
  return ret.release();
}


IMPNPCTRANSPORT_END_NAMESPACE
