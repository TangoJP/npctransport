#!/usr/bin/python
from IMP.npctransport import *
import sys
import math
import read_nups
import re
import os

# defaults
kaps_R = 30.0
k_fgfg=0.25
range_fgfg=4.0
k_fgkap=3.0
range_fgkap=4.0
rest_length_factor = 1.25 # 1
obstacle_inflate_factor = 1.3
fg_coarse_factor=3.0 # 3
# fetch params from cmd-line
if(len(sys.argv)<=1):
    print " Usage: <cmd> <outfile> [kaps_R=%.1f] [k_fgfg=%.1f] [rest_length_factor]" % (kaps_R, k_fgfg,rest_length_factor)
    exit(-1)
outfile = sys.argv[1]
if len(sys.argv) > 2:
    kaps_R = float(sys.argv[2])
print "kaps_R = %.2f" % (kaps_R)
if len(sys.argv) > 3:
    k_fgfg = float(sys.argv[3])
print "k_fgfg = %.2f" %k_fgfg
if len(sys.argv) > 4:
    rest_length_factor = float(rest_length_factor)
print "rest_length_factor = %.2f" %rest_length_factor

def get_basic_config():
    config = Configuration()
    IMP.npctransport.set_default_configuration(config)
    config.statistics_fraction.lower=1.0
    config.interaction_k.lower=10
    config.interaction_range.lower=1
    config.backbone_k.lower=0.25
    config.time_step_factor.lower=0.5 #### NOTE THIS ####
    config.time_step_wave_factor.lower=10.0 ### NOTE THIS ###
    config.excluded_volume_k.lower=2
    config.nonspecific_range.lower=4
    config.nonspecific_k.lower=0.1
    ################## ACTIVE RANGE ####################
    create_range(config.nonspecific_k, .1, 1, 5, 1)
    ####################################################
    config.slack.lower = 7.5
    config.number_of_trials=1
    config.dump_interval_ns=5
    config.simulation_time_ns=500
    config.angular_D_factor.lower=0.05 #lower to account for increased dynamic viscosity
                                      # in crowded environment and for coarse graining
    ################## ACTIVE RANGE ####################
    create_range(config.angular_D_factor, .05, .15, 3, 1)
    ####################################################
    config.statistics_interval_ns=0.1
    config.fg_anchor_inflate_factor=3.0/math.sqrt(fg_coarse_factor)
    return config



def add_interactions_for_fg(fg_name,
                            k_kap_lower,
                            k_kap_upper = 0, # relevant only if k_kap_steps > 1
                            k_kap_steps = 1,
                            k_kap_base = 1):
    interactionFG_KAP= IMP.npctransport.add_interaction(config,
                                                        name0=fg_name,
                                                        name1="kap",
                                                        interaction_k=k_kap_lower,
                                                        interaction_range=range_fgkap)
    if(k_kap_steps > 1):
        create_range(interactionFG_KAP.interaction_k,
                     k_kap_lower, k_kap_upper,
                     steps = k_kap_steps,
                     base = k_kap_base)
    interactionFG_CRAP= IMP.npctransport.add_interaction(config,
                                       name0=fg_name,
                                       name1="crap0",
                                       interaction_k=0,
                                       interaction_range=0)

def add_fg_based_on(config, mrc_filename, k, nbeads, origin=None,
                    coarse_factor=fg_coarse_factor):
    ''' Read mrc_filename, cluster to k clusters, and create k
        fgs with nbeads/coarse_factor beads, anchored at the clusters,
        normalized by mean_loc. An additional bead is used for the
        anchor residue

        @param coarse_factor - a factor by which to coarse grain beads
                               (e.g. 3 beads : 1)

        @note if origin==None, initiate it as a 3D coordinate that
        is the mean coordinate of the MRC file in mrc_filename.
        @note also updates global variables max_xy, max_x, max_y, max_z

        @return the mean location of the MRC file
        '''
    global max_r, max_x, max_y, max_z
    # get type name as filename without folder and extension parts
    type_search= re.search("([^/]*?)(?:[.].*)*$", mrc_filename)
    type_name = type_search.groups(0)[0]
    # cluster anchors from MRC file
    centers, mean_loc = read_nups.cluster_MRC_file_with_cache(mrc_filename, k)
    if(origin is None):
        origin = mean_loc
    coarse_nbeads = 1 + int(math.ceil(nbeads / coarse_factor)) # +1 for anchor
    fgs= IMP.npctransport.add_fg_type(config,
                                      type_name= type_name,
                                      number_of_beads= coarse_nbeads,
                                      number=len(centers),
                                      radius=7 * math.sqrt(coarse_factor),
                                      interactions= int(math.ceil(1 * coarse_factor)),
                                      rest_length_factor = rest_length_factor)
    add_interactions_for_fg(type_name, k_fgkap)
    for center in centers:
        pos=fgs.anchor_coordinates.add()
        pos.x=center[0] - origin[0]
        pos.y=center[1] - origin[1]
        pos.z=center[2] - origin[2]
        r = math.sqrt(pos.x**2 + pos.y**2)
        print mrc_filename, "z=", pos.z, "r=", r
        max_r = max(max_r, r)
        max_x = max(max_x, abs(pos.x))
        max_y = max(max_y, abs(pos.y))
        max_z = max(max_z, abs(pos.z))
    return mean_loc

def add_obstacle(config, mrc_filename, k, R, origin=None):
    ''' Read mrc_filename, cluster to k clusters, and create k
        obstacles anchored at the clusters, normalized about origin.

        @param k the number of obstacles
        @param R the obstacle radius, to be inflated by obstacle_inflate_factor
        @param origin the new origin

        @note if origin==None, initiate it as a 3D coordinate that
        is the mean coordinate of the MRC file in mrc_filename.

        @return the mean location of the MRC file clusters
        '''
    # get type name as filename without folder and extension parts
    print mrc_filename
    type_search= re.search("([^/]*?)(?:[.].*)*$", mrc_filename)
    type_name = type_search.groups(0)[0]
    # cluster anchors from MRC file
    centers, mean_loc = read_nups.cluster_MRC_file_with_cache(mrc_filename, k)
    if(origin is None):
        origin = mean_loc
    obstacle = IMP.npctransport.add_obstacle_type \
        (config, type_name=type_name, R = R * obstacle_inflate_factor)
    for center in centers:
        pos=obstacle.xyzs.add()
        pos.x=center[0] - origin[0]
        pos.y=center[1] - origin[1]
        pos.z=center[2] - origin[2]
        r = math.sqrt(pos.x**2 + pos.y**2)
        print "OBSTACLE", mrc_filename, "z=", pos.z, "r=", r, "R=", R
    return mean_loc



# ************** MAIN: *************
IMP.set_log_level(IMP.base.SILENT)
config= get_basic_config()

# Add floaters
n_kap_interactions=12/3
kaps= IMP.npctransport.add_float_type(config,
                                     number=100,
                                     radius=kaps_R,
                                      interactions= n_kap_interactions,
                                      type_name="kap")
############### ACTIVE RANGE #############
create_range(kaps.interaction_k_factor, lb=0.01, ub=5, steps = 10, base=1.33)
##########################################
#create_range(kaps.radius, lb = 10, ub = 30, steps = 5, base = 1)
nonspecifics= IMP.npctransport.add_float_type(config,
                                              number=100,
                                              radius=kaps_R, #-1,
                                              interactions=0,
                                              type_name="crap0")
#create_range(nonspecifics.radius, lb = 10, ub = 30, steps = 5, base = 1)
# fg with kaps / craps
#add_interactions_for_fg("fg0", 2.5, 7.5, k_kap_steps = 10, k_kap_base=1)
#add_interactions_for_fg("fg1", 2.5, 7.5, k_kap_steps = 10, k_kap_base=1)

# non-specific attraction
#create_range(config.nonspecific_k,lb=1.0,ub=2.0,steps = 3,base=1)


# Add FGs with anchors
# (Stoicheometries from Alber et al. 2007b, Determining..., Fig. 3)
max_r=0
max_x=0
max_y=0
max_z=0
mean_loc=(add_fg_based_on(config, "MRCs/Nup57_16copies_chimera.mrc", k=16, nbeads=16))
add_fg_based_on(config, "MRCs/Nup49_16copies.mrc", k=16, nbeads = 17, origin=mean_loc)
add_fg_based_on(config, "MRCs/Nsp1_16copies_1.mrc", k=16, nbeads = 33, origin=mean_loc)
add_fg_based_on(config, "MRCs/Nsp1_16copies_2.mrc", k=16, nbeads = 33, origin=mean_loc)
add_fg_based_on(config, "MRCs/Nup159_8copies.mrc", k=8, nbeads=20, origin=mean_loc) # nbeads 20-24 = real number for Nup159, depending how you count double motifs
add_fg_based_on(config, "MRCs/Nup116_8copies_chimera.mrc", k=8, nbeads=46, origin=mean_loc)
add_fg_based_on(config, "MRCs/Nup42_8copies_chimera.mrc", k=8, nbeads=21, origin=mean_loc) # nbeads 21-27, depending on treratment of double motifs
add_fg_based_on(config, "MRCs/Nup100_8copies_chimera.mrc", k=8, nbeads=44, origin=mean_loc)
add_fg_based_on(config, "MRCs/Nup145N_8copies_1_chimera.mrc", k=8, nbeads=44, origin=mean_loc)
add_fg_based_on(config, "MRCs/Nup145N_8copies_2_chimera.mrc", k=8, nbeads=44, origin=mean_loc)
# Nuclear:
add_fg_based_on(config, "MRCs/Nup1_8copies.mrc", k=8, nbeads=27, origin=mean_loc)
#create_range(config.fgs[-1].interaction_k_factor, lb=k_fgkap, ub=k_fgkap*4, steps=10, base=1)
#add_fg_based_on(config, "MRCs/Nup59_16copies.mrc", k=16, nbeads=5, origin=mean_loc) # contains also RRM Nup35-type domain for RNA binding (res 265-394 ; Uniprot), which supposedly overlaps some of the FGs
add_fg_based_on(config, "MRCs/Nup60_8copies.mrc", k=8, nbeads=11, origin=mean_loc) # nup60 is supposed to tether Nup2 (depending on Gsp1p-GTP (Ran) switch, for which Nup2 has a binding site 583-720 ; Denning, Rexach et al. JCB 2001) ; Nup2 also interacts with cargo 35-50 and RNA (RRM Nup35-type domain) - from Uniprot)
#add_fg_based_on(config, "MRCs/Nup53_16copies_chimera.mrc", k=16, nbeads=4, origin=mean_loc) # contains also RRM Nup35-type domain for RNA binding (res 247-352 ; Uniprot), which supposedly overlaps some of the FGs ; and also a PSE1/Kap121 binding domain in a non-FG fashion, for which there might be a crystal structure (405-438 ; Uniprot)

# Add Structural nups as obstacles
# (Alber et al. 2007b, Deteriming..., Figure 3)
add_obstacle(config, "MRCs/Gle1_8copies.mrc", k=8*2, R=21, origin=mean_loc)
add_obstacle(config, "MRCs/Gle2_16copies.mrc", k=16, R=23, origin=mean_loc)
add_obstacle(config, "MRCs/Ndc1_16copies.mrc", k=16*2, R=22, origin=mean_loc)
add_obstacle(config, "MRCs/Nic96_16copies_1.mrc", k=16*2, R=24, origin=mean_loc)
add_obstacle(config, "MRCs/Nic96_16copies_2.mrc", k=16*2, R=24, origin=mean_loc)
add_obstacle(config, "MRCs/Nup120_16copies.mrc", k=16*2, R=26, origin=mean_loc)
add_obstacle(config, "MRCs/Nup133_16copies.mrc", k=16*2, R=27, origin=mean_loc)
add_obstacle(config, "MRCs/Nup145C_16copies.mrc", k=16*2, R=23, origin=mean_loc)
add_obstacle(config, "MRCs/Nup157_16copies.mrc", k=16*3, R=25, origin=mean_loc)
add_obstacle(config, "MRCs/Nup170_16copies.mrc", k=16*2, R=29, origin=mean_loc)
add_obstacle(config, "MRCs/Nup188_16copies.mrc", k=16*2, R=30, origin=mean_loc)
add_obstacle(config, "MRCs/Nup192_16copies.mrc", k=16*2, R=30, origin=mean_loc)
add_obstacle(config, "MRCs/Nup53_16copies_chimera.mrc", k=16, R=32, origin=mean_loc) #### omit if include as FG
add_obstacle(config, "MRCs/Nup59_16copies.mrc", k=16, R=32, origin=mean_loc) #### omit if include as FG
add_obstacle(config, "MRCs/Nup82_8copies_1.mrc", k=8*2, R=23, origin=mean_loc)
add_obstacle(config, "MRCs/Nup82_8copies_2.mrc", k=8*2, R=23, origin=mean_loc)
add_obstacle(config, "MRCs/Nup84_16copies.mrc", k=16*3, R=20, origin=mean_loc)
add_obstacle(config, "MRCs/Nup85_16copies.mrc", k=16*3, R=20, origin=mean_loc)
add_obstacle(config, "MRCs/Pom34_16copies.mrc", k=16*3, R=15, origin=mean_loc)
add_obstacle(config, "MRCs/Sec13_16copies.mrc", k=16, R=21, origin=mean_loc)
add_obstacle(config, "MRCs/Seh1_16copies.mrc", k=16, R=22, origin=mean_loc)

# add bounding volumes
config.box_is_on.lower=1
config.box_side.lower=max(max_z,max_x,max_y)*4 # 2000
config.slab_is_on.lower=1
config.tunnel_radius.lower=max_r - config.fgs[0].radius.lower # or also upper when there's steps?
config.slab_thickness.lower=250.0 # yeast nuclear envelope - see http://books.google.com/books?id=GvxdK1mdqQwC&pg=PA278&lpg=PA278&dq=yeast+nuclear+envelope+dimensions+nanometer&source=bl&ots=tHQoLfXHI1&sig=nRgZmLYnKuiRNP8n6vhm3bapjpI&hl=en&sa=X&ei=VtwKUtvAAsTAyAHOmIDYBg&ved=0CHsQ6AEwCA#v=onepage&q=yeast%20nuclear%20envelope%20dimensions%20nanometer&f=false
# config.slab_thickness.lower = max_z - config.fgs[0].radius.lower  # or also upper when there's steps?
config.are_floaters_on_one_slab_side = 1 # all on top side


#create_range(interactionFG_FG.interaction_k, lb = 2.5, ub = 7.5, steps = 10, base = 1)
# internal FG-FG
#n_fg_types = len(config.fgs)
for i,fg0 in enumerate(config.fgs):
    for j,fg1 in enumerate(config.fgs):
        if(i>j): continue # avoid duplicates
        interactionFG_FG= IMP.npctransport.add_interaction(config,
                                                           name0= fg0.type,
                                                           name1= fg1.type,
                                                           interaction_k= k_fgfg,
                                                           interaction_range= range_fgfg)

# dump to file
f=open(outfile, "wb")
f.write(config.SerializeToString())
# dump text
outfile_txt = os.path.splitext(outfile)[0] + ".txt"
f_txt = open(outfile_txt, "w")
print >>f_txt, config
f_txt.close()
