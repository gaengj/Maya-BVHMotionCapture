import pymel.core as pm # use pm, easier than mfn binding ...
import maya.cmds as mc # maya commands
import os
"""
class for parent hierarchy
"""
class node:
	def __init__(self, name, parent=None):
		self.name = name
		self.parent = parent
		
	def __str__(self):
		# name of obj
		return str(self.name)
		
	def objPath(self):
		# return object path 
		if self.parent is not None:
			# recursively go up the parenthood link
			# appending names in the process
			return "%s|%s" % (self.parent.objPath(), self.__str__())
		return str(self.name)
		
"""
get appropriate maya notation from 
bvh file translation and rotation
tokens
"""
def maya_notation(str_):
	if str_ == "Xposition":
		return "translateX"
	elif str_ == "Yposition":
		return "translateY"
	elif str_ == "Zposition":
		return "translateZ"
	elif str_ == "Xrotation":
		return "rotateX"
	elif str_ == "Yrotation":
		return "rotateY"
	elif str_ == "Zrotation":
		return "rotateZ"
		

rootNode = None # root node
# path of bvh file
filename="C:\\Users\\ensimag\\Downloads\\sia2\\GPGPU_TP\\plugin_python\\walkSit.bvh"
curr_parent = None
motion_frame = False # flag used to know if we are in motion or not
channel_lst = [] # channel list

# parameters, how to set?
frame = 0 # int
# "XYZ" "YZX" etc ....
rotOrder = 0



with open(filename) as f:
	if f.next().startswith("HIERARCHY"):
		pass
	else:
		print("bad format, exiting\n") 
		exit()
	
	rootNode = None
	if rootNode is None: # if we are here for the first time 
				path_file = os.path.basename(filename)
				group = pm.group(em=True,name="_mocap_%s_grp" % path_file)
				group.scale.set(1.0, 1.0, 1.0) # scale 1.0
				
				# The group is now the 'root'
				curr_parent = node(str(group), None)
	else:
				curr_parent = node(str(rootNode), None)
				self._clear_animation()
	close_flag = False
	# now begin parsing the bvh
	for line in f:
		# parse the line, change separators, only use spaces.
		line = line.replace("	"," ")
		# one tab is 4 spaces
		# each identation level is 1 space further on the right
		if curr_parent:
			print (curr_parent.objPath())
		if not motion_frame:
			# root joint
			if line.startswith("ROOT"):
				# root becomes the hip joint 
				if rootNode:
					curr_parent = node(str(rootNode), None)
				else:
					# update parenthood
					curr_parent = node(line[5:].rstrip(), curr_parent)
		
			if "MOTION" in line:
				# read all the frames at the end of the file if it's the case
				motion_frame = True		

			# treat each possible case
			if "JOINT" in line:
				new_joint = line.split(" ") # create list iterator of this JOINT line
				# parent of joint is the old curr_parent 
				# rstrip() gets rid of the spaces
				curr_parent = node(new_joint[-1].rstrip(), curr_parent)

			if "End" in line: # End Site
				close_flag = True # close ignoring "}"
				
		
			#if "{" in line:
			#	continue # nothing to do here
				
			if "}" in line:
				# close the section if flag is true
				print close_flag
				if close_flag:
					close_flag = False
					continue # go back to beginning of the for loop
					
				# update parent (one level higher)
				if curr_parent is not None:
					# go one level above
					curr_parent = curr_parent.parent
					if curr_parent is not None: # if still not None select it
						mc.select(curr_parent.objPath())
				
			if "CHANNELS" in line:
				curr_chan = line.strip().split(" ")

				# for each channel indice
				# curr_chan[0] is "CHANNELS", curr_chan[1] is the channel number
				chan_nb_param = int(curr_chan[1]) # curr_chan[1] is int containing nb of rotate or tanslate param
				#print channel_lst
				for nb in range(chan_nb_param):
					channel_lst.append("%s.%s" % (curr_parent.objPath(), maya_notation(curr_chan[nb+2]))) # create full name, used later 
					
			if "OFFSET" in line:
				curr_offset = line.strip().split(" ")
				joint_name = str(curr_parent)
				if close_flag:
					joint_name = joint_name + "_end" # add _end for debug

				objpath = curr_parent.objPath()
				# create object by specifying fullpath using pymel module (pm)
				# cant do that in maya in maya we have to create mfnikjoint
				# exists in python also but pm is simpler
				if mc.objExists(str(curr_parent.objPath())):
					# already exists, don't create it just translate it
					curr_joint = pm.PyNode(objpath)
					curr_joint.rotateOrder.set(rotOrder)
					curr_joint.translate.set([float(curr_offset[1]), float(curr_offset[2]), float(curr_offset[3])])
					continue
				# create the joint
				curr_joint = pm.joint(name=joint_name, p=(0,0,0))
				curr_joint.translate.set([float(curr_offset[1]), float(curr_offset[2]), float(curr_offset[3])])
				curr_joint.rotateOrder.set(rotOrder)
		else:
			# motion here, parse the keyframes
			if "Frame" not in line: # ignore the first two lines containing info about the frame Frame and Frames:nb_of_frame
						data = line.split(" ")
						if len(data) > 0:
							if data[0] == "":
								data.pop(0) # rm empty char
						
						# Set the values to channels
						for x in range(0, len(data) - 1 ): # for all keyframes
							mc.setKeyframe(channel_lst[x], time=frame, value=float(data[x])) # add keyframe, easy in maya, need animcurve in c++
						frame = frame + 1