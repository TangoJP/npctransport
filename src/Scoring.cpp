/**
 *  \file Scoring.cpp
 *  \brief description.
 *
 *  Copyright 2007-2013 IMP Inventors. All rights reserved.
 *
 */

#include <IMP/npctransport/SimulationData.h>
#include <IMP/npctransport/Scoring.h>
#include <IMP/npctransport/SitesPairScore.h>
#include <IMP/npctransport/SlabSingletonScore.h>
#ifdef IMP_NPC_GOOGLE
IMP_GCC_PUSH_POP(diagnostic push)
IMP_GCC_PRAGMA(diagnostic ignored "-Wsign-compare")
#include "third_party/npc/npctransport/data/npctransport.pb.h"
IMP_GCC_PUSH_POP(diagnostic pop)
#else
#include <IMP/npctransport/internal/npctransport.pb.h>
#endif
#include <IMP/npctransport/typedefs.h>
#include <IMP/base_types.h>
#include <IMP/algebra/vector_generators.h>
#include <IMP/atom/estimates.h>
#include <IMP/atom/distance.h>
#include <IMP/atom/Diffusion.h>
#include <IMP/atom/Selection.h>
#include <IMP/base/exception.h>
#include <IMP/base/log.h>
#include <IMP/container/CloseBipartitePairContainer.h>
#include <IMP/container/ConsecutivePairContainer.h>
#include <IMP/container/ListSingletonContainer.h>
#include <IMP/container/generic.h>
#include <IMP/core/BoundingBox3DSingletonScore.h>
#include <IMP/core/DistancePairScore.h>
#include <IMP/core/SphereDistancePairScore.h>
#include <IMP/core/HarmonicUpperBound.h>
#include <IMP/core/pair_predicates.h>
#include <IMP/core/RestraintsScoringFunction.h>
#include <IMP/core/XYZ.h>
#include <numeric>
#include <set>

IMPNPCTRANSPORT_BEGIN_NAMESPACE
#define GET_ASSIGNMENT(name) name##_ = data.name().value()
#define GET_VALUE(name) name##_ = data.name()

Scoring::Scoring
(SimulationData* owner_sd,
 const ::npctransport_proto::Assignment &data)
: Object("Scoring%1%"),
  owner_sd_(owner_sd),
  is_updating_particles_(false)
{
  IMP_ALWAYS_CHECK(owner_sd_ != nullptr,
                   "Must have non-null owner simulation data",
                   IMP::base::ValueException);
  GET_ASSIGNMENT(box_side);
  GET_ASSIGNMENT(tunnel_radius);
  GET_ASSIGNMENT(slab_thickness);
  GET_ASSIGNMENT(slab_is_on);
  GET_ASSIGNMENT(box_is_on);
  GET_ASSIGNMENT(interaction_k);
  GET_ASSIGNMENT(interaction_range);
  GET_ASSIGNMENT(backbone_k);
  GET_ASSIGNMENT(slack);
  GET_ASSIGNMENT(nonspecific_k);
  GET_ASSIGNMENT(nonspecific_range);
  GET_ASSIGNMENT(excluded_volume_k);
  GET_VALUE(range);
  update_particles();
}

IMP::ScoringFunction*
Scoring::get_scoring_function() const
{
  // set up the restraints for the BD simulation:
  RestraintsTemp rs = chain_restraints_;
  if (get_has_bounding_box())
    rs.push_back(get_box_restraint());
  if (get_has_slab())
    rs.push_back(get_slab_restraint());
  rs.push_back(this->get_predr());
  is_updating_particles_ = false; // everything is supposed to be updated now
  IMP_NEW(core::RestraintsScoringFunction, rsf, (rs));
  return rsf.release();
}

// a close pair container for all diffusers
IMP::PairContainer*
Scoring::get_close_diffusers_container() const
{
  // TODO: only get_diffusers() sets updated to true, but that's ok, cause we
  //       call get_diffusers(). But it may be bug prone - check it out in
  //       the future.
  if (!close_diffusers_container_ || is_updating_particles_)
    {
      // populate a list of optimizable (spatially dynamic) diffusers
      IMP::ParticlesTemp all_diffusers;
      IMP::ParticlesTemp optimizable_diffusers;
      IMP::Model* m =
        const_cast<Scoring*>(this)->get_sd()->get_model();
      IMP_CONTAINER_FOREACH
        (container::ListSingletonContainer,
         get_diffusers(),
         {
           Particle* p = m->get_particle(_1);
           all_diffusers.push_back(p);
           if(core::XYZ::get_is_setup(p)) {
             core::XYZ p_xyz(p);
             if(p_xyz.get_coordinates_are_optimized()){
               optimizable_diffusers.push_back ( p );
             }
           }
         }
         );
      // create the cbpc and store it in close_diffusers_container_
      IMP_NEW( container::CloseBipartitePairContainer, cpc,
               ( optimizable_diffusers, all_diffusers,
                 get_range(), slack_) );
      IMP_NEW( core::AllSamePairPredicate, aspp, () );
      cpc->add_pair_filter( aspp ); // only relevant for bipartite
      // IMP_NEW( container::ClosePairContainer, cpc,
      //          ( all_diffusers,
      //            get_range(), slack_) );
      IMP_NEW( container::ExclusiveConsecutivePairFilter, ecpf, () );
      cpc->add_pair_filter( ecpf );
      close_diffusers_container_ = cpc;
      // if predicate pair restraint was already set - need to update it
      // note that this will cause a recursion, which should be fine
      if(predr_) {
        predr_=nullptr;
        get_predr(); // just to update
      }
    }
  return close_diffusers_container_;
}

container::PredicatePairsRestraint *Scoring::get_predr() const {
  if (!predr_ || is_updating_particles_) {
    // set linear repulsion upon penetration between all close pairs
    // returned by get_close_diffusers_container(), with different scores for interactions
    // between particles of different (ordered) types
    IMP_NEW(core::OrderedTypePairPredicate, otpp, ());
    otpp_ = otpp;
    IMP_NEW(container::PredicatePairsRestraint, ppr,
            (otpp, get_close_diffusers_container()) );
    IMP_NEW(LinearSoftSpherePairScore, ssps, (excluded_volume_k_));
    ppr->set_unknown_score(ssps.get());
    predr_ = ppr;
  }
  return predr_;
}

/**
   add a SitesPairScore restraint that applies to particles of
   types t0 and t1 to the PredicatePairsRestraint object returned by
   get_predr().

   A SitesPairScore interaction means site-specific
   attractive forces between bidning sites on each particle,
   and non-specific attraction and repulsion (upon penetraion)
   between the particles themselves.
*/
void Scoring::add_interaction
( const ::npctransport_proto::Assignment_InteractionAssignment &idata)
{
  // extract interaction params
  core::ParticleType type0(idata.type0());
  core::ParticleType type1(idata.type1());
  double base_k = interaction_k_;
  if (idata.has_interaction_k()) {
    base_k = idata.interaction_k().value();
  }
  // no particles so drop it
  if (interaction_k_factors_.find(type0) == interaction_k_factors_.end() ||
      interaction_k_factors_.find(type1) == interaction_k_factors_.end()) {
    return;
  }
  double interaction_k = base_k * interaction_k_factors_.find(type0)
                                      ->second  // TODO: validate type exists
                         *
                         interaction_k_factors_.find(type1)->second;
  double base_range = interaction_range_;
  if (idata.has_interaction_range()) {
    base_range = idata.interaction_range().value();
  }
  double interaction_range =
      base_range * interaction_range_factors_.find(type0)->second *
      interaction_range_factors_.find(type1)->second;

  IMP_LOG(IMP::base::PROGRESS,
          "creating interaction "
            << idata.type0() << "," << idata.type1()
            << " effective_k = " << interaction_k
            << ", effective_range = " << interaction_range
            << ", nonspecific k = " << nonspecific_k_
            << ", nonspecific range = " << nonspecific_range_
            << ", excluded volume k = " << excluded_volume_k_
            << std::endl
          );

  // create interaction into get_predr()
  container::PredicatePairsRestraint *ppr = get_predr();
  // add the interaction restraint both for (t0,t1) and (t1,t0)
  {
    // TODO: repulsion was also added in get_predr - do we double count here?
    core::ParticleTypes ts;
    ts.push_back(type0);
    ts.push_back(type1);
    int interaction_id = otpp_->get_value(ts);
    set_sites_score(
        interaction_range,                  // site-specific
        interaction_k, nonspecific_range_,  // non-specific = entire particle
        nonspecific_k_, excluded_volume_k_,
        get_sd()->get_sites(type0),
        get_sd()->get_sites(type1),
        interaction_id, ppr );
  }
  {
    core::ParticleTypes ts;
    ts.push_back(type1);
    ts.push_back(type0);
    int interaction_id = otpp_->get_value(ts);
    set_sites_score
      (  interaction_range,                  // site-specific
         interaction_k, nonspecific_range_,  // non-specific = entire particle
         nonspecific_k_, excluded_volume_k_,
         get_sd()->get_sites(type0),
         get_sd()->get_sites(type1),
         interaction_id, ppr );
  }
}



/** add a restraint on particles in an FG nup chain */
Restraint*
Scoring::add_chain_restraint(const ParticlesTemp &particless,
                             double rest_length,
                             std::string name)
{
  IMP_ALWAYS_CHECK( !particless.empty(), "No Particles passed.",
                   IMP::base::ValueException );
  IMP_ALWAYS_CHECK( rest_length > 0, "negative rest length invalid",
                   IMP::base::ValueException );

  // Exclusive means that the particles will be in no other
  // ConsecutivePairContainer
  // this assumption accelerates certain computations
  IMP_NEW(container::ExclusiveConsecutivePairContainer, xcpc,
          (particless, name + " consecutive pairs"));
  // add chain restraint
  LinearWellPairScore* lwps =
    new LinearWellPairScore( rest_length, get_backbone_k() );
  base::Pointer<Restraint> cr =
    IMP::container::create_restraint
    ( lwps, xcpc.get(), "chain restraint %1%" );
  chain_scores_.push_back(lwps);
  chain_restraints_.push_back(cr);

  return cr;
}


/**
   Creates bounding volume restraints such as box restraint and slab restraints,
   based on the box_size_, slab_height_, slab_radius_, etc. class variables
*/
void Scoring::create_bounding_box_restraint_on_diffusers()  {
  // Add bounding box restraint
  // TODO: what does backbone_spring_k_ has to do
  //       with bounding box constraint?
  IMP_NEW(core::HarmonicUpperBound, hub, (0, excluded_volume_k_));
  IMP_NEW(core::GenericBoundingBox3DSingletonScore<core::HarmonicUpperBound>,
          bbss, (hub.get(), get_sd()->get_box()));
  box_restraint_ =
    container::create_restraint(bbss.get(),
                                get_sd()->get_diffusers(),
                                "bounding box");
}

void Scoring::create_slab_restraint_on_diffusers()  {
  // Add cylinder restraint
  IMP_NEW(SlabSingletonScore, slab_score,
          (slab_thickness_ /* h */, tunnel_radius_ /*r*/, excluded_volume_k_));
  slab_restraint_ = container::create_restraint(slab_score.get(),
                                                get_sd()->get_diffusers(),
                                                "bounding slab");
}

container::ListSingletonContainer const*
Scoring::get_diffusers() const
{
  return const_cast<Scoring*>(this)->get_sd()->get_diffusers();
}


#undef GET_ASSIGNMENT
#undef GET_VALUE

IMPNPCTRANSPORT_END_NAMESPACE
