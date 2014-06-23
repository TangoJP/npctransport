/**
 *  \file SitesPairScore.h
 *  \brief A Score on the distance between a pair of particles.
 *
 *  Copyright 2007-2010 IMP Inventors. All rights reserved.
 */

#ifndef IMPNPCTRANSPORT_INTERNAL_SITES_H
#define IMPNPCTRANSPORT_INTERNAL_SITES_H

#include "../npctransport_config.h"
#include "RigidBodyInfo.h"
#include <IMP/core/rigid_bodies.h>


IMPNPCTRANSPORT_BEGIN_INTERNAL_NAMESPACE

inline double evaluate_one_site(
    double k, double range, core::RigidBody rb0, core::RigidBody rb1,
    algebra::Transformation3D tr0, algebra::Transformation3D tr1,
    algebra::Rotation3D irot0, algebra::Rotation3D irot1,
    const algebra::Vector3D &local0, const algebra::Vector3D &local1,
    DerivativeAccumulator *da) {
  algebra::Vector3D g0 = tr0.get_transformed(local0);
  algebra::Vector3D g1 = tr1.get_transformed(local1);
  algebra::VectorD<3> delta = g0 - g1;
  static const double MIN_DISTANCE = .00001;
  double distance2 = delta.get_squared_magnitude();
  if (distance2 > square(range)) return 0;
  double distance = std::sqrt(distance2);
  double score;
  score = .5 * k * distance2 - .5 * k * square(range);
  if (da && distance > MIN_DISTANCE) {
    algebra::Vector3D duv = k * delta;
    rb0.add_to_derivatives(irot0.get_rotated(duv), duv, local0,
                           tr0.get_rotation(), *da);
    rb1.add_to_derivatives(irot1.get_rotated(-duv), -duv, local1,
                           tr1.get_rotation(), *da);
  }
  return score;
}
/*inline float Q_rsqrt( float number )
{
  int i;
  float x2, y;
  const float threehalfs = 1.5F;

  x2 = number * 0.5F;
  y  = number;
  i  = *reinterpret_cast<int*>(&y);
  i  = 0x5f3759df - ( i >> 1 );
  y  = *reinterpret_cast<float*>(&i);
  y  = y * ( threehalfs - ( x2 * y * y ) );
  y  = y * ( threehalfs - ( x2 * y * y ) );

  return y;
  }*/

/**
   Evaluates the attraction score between two sites (local0 and local1),
   transformed by tr0 and tr1 respectively, based on their distance

   The returned score is linear -k*(2-distance) for distance < 1.0
   or -k/distance for distance > 1.0
   TODO: why arbitrary threshold distance for linear range set to 1.0?

   @param rb0[out],rb1[out] the particles that contain the sites
   @param tr0, tr1 the particles frames of reference
   @param irot0, irot1 the reverse rotations from each particle frame of ref.
   @param local0,local1 the sites
   @param da[out] derivative accumulator that gets updated
                  with the score derivative

   @return the score
*/
inline double evaluate_one_site_2(
    double k, double range, core::RigidBody rb0, core::RigidBody rb1,
    algebra::Transformation3D tr0, algebra::Transformation3D tr1,
    algebra::Rotation3D irot0, algebra::Rotation3D irot1,
    const algebra::Vector3D &local0, const algebra::Vector3D &local1,
    DerivativeAccumulator *da) {
  algebra::Vector3D g0 = tr0.get_transformed(local0);
  algebra::Vector3D g1 = tr1.get_transformed(local1);
  algebra::VectorD<3> delta = g0 - g1;
  static const double MIN_DISTANCE = .00001;
  double distance2 = delta.get_squared_magnitude();
  if (distance2 > algebra::get_squared(range)) return 0;
  if (distance2 < MIN_DISTANCE) return -k * range;
  // double distance=std::sqrt(distance2);
  // double dp1= distance+1;
  // double dp12= algebra::get_squar4ed(dp1);
  double idistance = 1.0f / sqrtf(distance2);
  double distance = distance2 * idistance;
  double kidistance = k * idistance;
  algebra::VectorD<3> deriv = kidistance * delta;  // magnitude k
  double score = -k * (range - distance);
  if (da && distance2 > MIN_DISTANCE) {
    rb0.add_to_derivatives(irot0.get_rotated(deriv), deriv, local0,
                           tr0.get_rotation(), *da);
    rb1.add_to_derivatives(irot1.get_rotated(-deriv), -deriv, local1,
                           tr1.get_rotation(), *da);
  }
  return score;
}

/**
    Evaluate interaction on pair of sites

    @param k - force constant
    @param range - attraction range
    @param range - cached range**2
    @param rbi0 - cached information on rigid body 0
    @param rbi1 - cached information on rigid body 1
    @param l0 - site0 local coordinates
    @param l1 - site1 local coordinates
    @param g0 - site0 global coordinates
    @param g1 - site1 global coordinates
    @param da - accumulator for reweighting derivatives
    */
inline double evaluate_one_site_3
( double k,
  double range, double range2,
  RigidBodyInfo& rbi0, RigidBodyInfo& rbi1,
  const algebra::Vector3D &l0, const algebra::Vector3D &l1,
  algebra::Vector3D& g0, algebra::Vector3D& g1,
  DerivativeAccumulator *da)
{
  static const double MIN_DISTANCE2 = .00001;
  algebra::VectorD<3> delta = g0 - g1;
  if(delta[0] > range || delta[1] > range || delta[2] > range) return 0;
  double distance2 = delta.get_squared_magnitude();
  if (distance2 > range2) return 0;
  if (distance2 < MIN_DISTANCE2) return -k * range;
  double idistance = 1.0f / sqrtf(distance2);
  double distance = distance2 * idistance;
  double score = -k * (range - distance);
  if (da && distance2 > MIN_DISTANCE2) {
    algebra::Vector3D gderiv0 = (k * idistance) * delta;
    algebra::Vector3D lderiv0 = rbi0.irot.get_rotated(gderiv0);
    rbi0.rb.add_to_derivatives(lderiv0, gderiv0, l0,
                               rbi0.tr.get_rotation(),*da);
    algebra::Vector3D gderiv1 = -gderiv0;
    algebra::Vector3D lderiv1 = rbi1.irot.get_rotated(gderiv1);
    rbi1.rb.add_to_derivatives(lderiv1, gderiv1, l1,
                               rbi1.tr.get_rotation(),*da);
  }
  return score;
}

IMPNPCTRANSPORT_END_INTERNAL_NAMESPACE

#endif /* IMPNPCTRANSPORT_INTERNAL_SITES_H */
