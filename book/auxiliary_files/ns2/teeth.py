#!/usr/bin/python3
import nstrace
import sys

# This script counts individual teeth AND tooth clusters. 
# Two teeth are considered part of the same cluster if both are within TEETH_GRANULARITY of the start of the cluster.
# A tooth more than TEETH_GRANULARITY after the start of the last cluster begins a new cluster
# We also count drop clusters, too, using the same principle but using DC_GRANULARITY

# Teeth are TCP responses and are analyzed with the nstrace.isVar() records
# Drops are router actions and are analyzed with the nstrace.isEvent() records

# This script assumes there are only two TCP flows present, 0 and 1.

STARTPOINT = 3.0
DC_GRANULARITY= 3.0		# use for drop clusters
TEETH_GRANULARITY=1 		# 0.45 = 2 RTTs; can also be set to 0.5
DEBUG = False

class flowstats:
	def __init__(self):
		self.toothcount = 0		# count of all teeth for this flow
		self.toothclusters = 0		# count of teeth clusters for this flow; see prologue above
		self.teeth_this_cluster = 0	# count of teeth in current cluster
		self.prevcwnd = 0		# previous cwnd trace value
		self.prevtime = 0		# time of previous cwnd trace value
		self.prevlosstime = 0		# start time of current tooth
		#self.count = 0			# counts times cwnd_ is updated; not really used
		self.CTOcount = 0		# count of Coarse TimeOuts
		self.nonhalvingcount = 0	# count of events where cwnd_ is reduced, but not by half; should equal CTOcount for Reno-based TCPs
		

def countpeaks(filename):
	global STARTPOINT, DC_GRANULARITY, TEETH_GRANULARITY, DEBUG	# CWND_CTO_GRANULARITY

	sharedteethclusters = 0
	clusterstarttime = 0.0			# start of current tooth/drop cluster
	lastflow = 0

	nstrace.nsopen(filename)

	flow0 = flowstats()
	flow1 = flowstats()
	maxclusterlen = 0
	dcstart  = 0
	dropcluster = (0,0)	# (x,y) represents x drops for flow 0 and y drops for flow 1
	clusterdict = {}	# a map of dropcluster pairs, above, to counts
	clusterstart = clusterfinish = 0;
	if (DEBUG): print("DC_GRANULARITY =", DC_GRANULARITY)
	while not nstrace.isEOF():
		if nstrace.isVar():		# counting cwnd_ trace lines
			(time, snode, dummy, dummy, dummy, varname, varvalue) = nstrace.getVar()
			if snode == 0: flow=flow0 
			else: flow=flow1	
			if (time < STARTPOINT):	continue
			if varname != "cwnd_": 	continue
			cwnd = varvalue
			# count first drop in cwnd after more than CWND_GRANULARITY elapsed time since any previous drop
			# also don't count drops to 1.0 (here we don't count drops to less than 2.0)
			if cwnd < flow.prevcwnd: 
				if time > clusterstarttime+TEETH_GRANULARITY:	# start a new tooth
					prevlosstime = time
					if flow0.teeth_this_cluster > 0: flow0.toothclusters += 1
					if flow1.teeth_this_cluster > 0: flow1.toothclusters += 1
					if flow0.teeth_this_cluster > 0 and flow1.teeth_this_cluster > 0: sharedteethclusters += 1
					flow0.teeth_this_cluster = 0
					flow1.teeth_this_cluster = 0
					clusterstarttime = time
					
				flow.teeth_this_cluster += 1

				if cwnd < 1.10 : #  checking for cwnd = 1, with some wiggle room
					flow.CTOcount += 1
				if not is_half(cwnd, flow.prevcwnd):	# counts CTOs and other non-Reno reductions in cwnd
					flow.nonhalvingcount += 1
				flow.toothcount += 1		# count all drops as a peak
				flow.prevlosstime = time		# record time peak occurred


			#flow.count  += 1
			flow.prevcwnd=cwnd
			flow.prevtime = time

		elif nstrace.isEvent():		# counting regular trace lines for DROP CLUSTERS
			(event, time, sendnode, dnode, proto, dummy, dummy, flow, dummy, dummy, seqno, pktid) = nstrace.getEvent()
			if (time < STARTPOINT): continue
			if (event=='d'):					# ignore all others
				if (time > dcstart + DC_GRANULARITY):		# save old cluster info, start new
					dcstart = time
					if (dropcluster != (0,0)):		# dropcluster==(0,0) means initial dc 
						inc_cluster(dropcluster, clusterdict)	# add dropcluster to dictionary
						
					dropcluster = addtocluster((0,0), flow)		# start new cluster
					clusterlen = clusterfinish - clusterstart	# length of old cluster
					maxclusterlen = max(clusterlen, maxclusterlen)
					clusterstart = clusterfinish = time
				else: 				# add to dropcluster
					dropcluster = addtocluster(dropcluster, flow)
					clusterfinish = time
		else:
			print("unknown line")
			nstrace.skipline()

	(drops_total, drops_flow0_only, drops_flow1_only, dictstring) = cluster_info(clusterdict)

	print (flow0.toothcount, flow0.nonhalvingcount, flow0.toothclusters, flow1.toothcount, flow1.nonhalvingcount, flow1.toothclusters, sharedteethclusters, drops_total, drops_flow0_only, drops_flow1_only, maxclusterlen, dictstring)
	
# increments the count associated with the given cluster, or inserts it with count=1 if not present
def inc_cluster(cluster, clusterdict):
	if (cluster in clusterdict): clusterdict[cluster] += 1;
	else: clusterdict[cluster] = 1;
	
def is_half(a,b):
	if (abs(2*a-b) < b*0.1): return True
	return False

# This and the following function return Python tuples rather than single values
def addtocluster(cluster,flow):
	(f0,f1) = cluster
	if (flow==0): f0+=1
	if (flow==1): f1+=1
	return (f0,f1)

def cluster_info(clusterdict):
	drops_total=0
	drops_flow0_only=0
	drops_flow1_only=0
	for cluster,count in clusterdict.items():
		drops_total += count
		(f0,f1) = cluster
		if (f0==0): drops_flow1_only+= count
		if (f1==0): drops_flow0_only+= count
	return (drops_total, drops_flow0_only, drops_flow1_only, '"' + dict2string(clusterdict) + '"')
		
# converts a dictionary object to a string without space characters
def dict2string(dict):
	s = str(dict)
	s = s.replace(' ', '')
	return s
	
countpeaks(sys.argv[1])
