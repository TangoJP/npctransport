#!/usr/bin/python
from IMP.npctransport import *
import sys
def create_range(field, lb, ub=None, steps=None, base=None):
    field.lower=lb
    if ub:
        field.upper=ub
        field.steps=steps
        if base:
            field.base=base
        else:
            try:
                field.base=2
            except:
                pass

config= Configuration()
config.dump_interval=20000
#config.dump_interval=1
config.interaction_k.lower=10
config.interaction_range.lower=1
# create_range(config.backbone_k, .2, 1, 10)
config.backbone_k.lower=.02
#config.time_step_factor.lower=0.3
config.time_step_factor.lower=3
#create_range(config.rest_length_factor, .5, 1, 10)
#config.default_repulsive_range.lower=1 # this doesn't do anything for now
config.excluded_volume_k.lower=.2
config.nonspecific_range.lower=1
config.nonspecific_k.lower=0.1
config.slack.lower=10
config.number_of_trials=1
config.number_of_frames=5000000
#config.number_of_frames.lower=500
config.angular_D_factor.lower=200
config.statistics_interval=20000

#simulation bounding volumes:
config.box_is_on.lower=1
config.box_side.lower=600
config.slab_is_on.lower=1
config.slab_thickness.lower=225
config.tunnel_radius.lower=100

fg= config.fgs.add()
fg.number_of_beads.lower=8
fg.number.lower=24
#fg.number.lower=5
fg.radius.lower=8
fg.interactions.lower=1
fg.rest_length_factor.lower=1
fg.d_factor.lower=1
fg.interaction_k_factor.lower=1.0
fg.interaction_range_factor.lower=1.0

kap=config.floaters.add()
# kap.number.lower=10
kap.number.lower=6
kap.radius.lower=40
kap.interactions.lower=10
kap.d_factor.lower=1
kap.interaction_k_factor.lower=1.0
kap.interaction_range_factor.lower=1.0

nonspecifics= config.floaters.add()
nonspecifics.number.lower=6
nonspecifics.radius.lower=40
nonspecifics.interactions.lower=0
nonspecifics.d_factor.lower=1
nonspecifics.interaction_k_factor.lower=1.0
nonspecifics.interaction_range_factor.lower=1.0

interactionFG_KAP = config.interactions.add()
interactionFG_KAP.type0="fg0"
interactionFG_KAP.type1="kap"
interactionFG_KAP.is_on.lower=1
interactionFG_KAP.interaction_range.lower=1
interactionFG_KAP.interaction_k.lower=10

interactionFG_FG = config.interactions.add()
interactionFG_FG.type0="fg0"
interactionFG_FG.type1="fg0"
interactionFG_FG.is_on.lower=1
interactionFG_FG.interaction_range.lower=1
interactionFG_FG.interaction_k.lower=10

interactionCRAP_KAP = config.interactions.add()
interactionCRAP_KAP.type0="crap"
interactionCRAP_KAP.type1="kap"
interactionCRAP_KAP.is_on.lower=1
interactionCRAP_KAP.interaction_range.lower=1
interactionCRAP_KAP.interaction_k.lower=10

interactionCRAP_FG = config.interactions.add()
interactionCRAP_FG.type0="crap"
interactionCRAP_FG.type1="fg0"
interactionCRAP_FG.is_on.lower=1
interactionCRAP_FG.interaction_range.lower=1
interactionCRAP_FG.interaction_k.lower=10

f=open(sys.argv[1], "wb")
f.write(config.SerializeToString())

print config
if len(sys.argv)>2:
    config.number_of_trials.lower=2
    config.number_of_frames.lower=2
    config.dump_interval=1
    f=open(sys.argv[2], "wb")
    f.write(config.SerializeToString())
