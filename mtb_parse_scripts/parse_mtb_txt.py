capteurs_lst = [["00B43DEC", "FOOTr"],
["00B43DED", "LLEGr"],
["00B43DEE", "ULEGl"],
["00B43DEF", "LLEGl"],
["00B43DF0", "FARMr"],
["00B43DF1", "SHOUr"],
["00B43DF2", "HEAD"],
["00B43DF3", "PELV"],
["00B43DF4", "STERN"],
["00B43DF5", "SHOUl"],
["00B43DF6", "HANDr"],
["00B43DF7", "UARMl"],
["00B43DF8", "FOOTl"],
["00B43DFA", "FARMl"],
["00B43DFB", "UARMr"],
["00B43DFC", "ULEGr"],
["00B43DFD", "HANDl"]] # ["00B43E23", "PROP1"]

# translation dictionnary
dico_capteur_to_joint = {
"00B43DEC": "FOOTr",
"00B43DED": "LLEGr",
"00B43DEE": "ULEGl",
"00B43DEF": "LLEGl",
"00B43DF0": "FARMr",
"00B43DF1": "SHOUr",
"00B43DF2": "HEAD",
"00B43DF3": "PELV",
"00B43DF4": "STERN",
"00B43DF5": "SHOUl",
"00B43DF6": "HANDr",
"00B43DF7": "UARMl",
"00B43DF8": "FOOTl",
"00B43DFA": "FARMl",
"00B43DFB": "UARMr",
"00B43DFC": "ULEGr",
"00B43DFD": "HANDl"} # "00B43E23": "PROP1"

dico_joint_to_capteur = dict()
keys = [capteurs_lst[i][0] for i in range(17)]
for i in range(len(keys)):
        dico_joint_to_capteur[capteurs_lst[i][1]] = capteurs_lst[i][0]


# global variables
FRAME_CHARLST = [] # contains all frames in strings, used to store them then write to bvh
CURR_LINE_FRAME = [] # current list containing computed frames values (relative rotations), GLOBAL TEMPORARY VARIABLE, update by functions
channel_lst = []
dico_frame_sensor = dict() # dictionnary : capteur_name -> frame_vector

capteur_nb = len(capteurs_lst) # number of sensors
VIDEO_ID = 4 # t-pose



class Dagnode:
        def __init__(self, parent=None, children=[], rel_rotation_value=[0.0, 0.0, 0.0], rot_sum = [0.0, 0.0, 0.0],
                      translation_value=[0.0, 0.0, 0.0], name="", channel_id=0):
                self.parent = parent
                self.children = children
                self.rel_rotation_value = rel_rotation_value
                self.rot_sum = rot_sum
                self.translation_value = translation_value
                self.name = name
                self.channel_id = channel_id

        def compute_rotation(self, glob_rot):
                if self.parent != None:
                        for i in range(3):
                                self.rel_rotation_value[i] = glob_rot[i] - self.parent.rot_sum[i]
                                self.rot_sum[i] = glob_rot[i] #+ self.parent.total_sum_rotation[i]#self.rel_rotation_value[i] + 
                else: # true if we are at the root node
                        for i in range(3):
                                self.rel_rotation_value[i] = glob_rot[i]
                                self.rot_sum[i] = glob_rot[i]
                                self.translation_value[i] = 0.0



f = open("skeleton_base.bvh")
lines = f.readlines()
curr_parent = None
close_flag = False
channel_id = 0
for line in lines:
        curr_line = []
        if line.startswith("HIERARCHY"):
                continue
        if line.startswith("ROOT"):
                curr_line = line.rstrip().split(" ")
                rootNode = Dagnode(children=[], name=curr_line[1], parent=None,
                                 rel_rotation_value=[0.0,0.0,0.0], rot_sum=[0.0, 0.0, 0.0],
                                 channel_id=channel_id, translation_value=[0.0, 0.0, 0.0])
                channel_lst = channel_lst + [ rootNode.name for i in range(6) ]
                channel_id += 6
                curr_parent = rootNode
        elif "JOINT" in line:
                curr_line = line.rstrip().split(" ")
                new_joint = Dagnode(parent=curr_parent, name=curr_line[1], children=[], channel_id=channel_id, rel_rotation_value=[0.0,0.0,0.0], rot_sum=[0.0, 0.0, 0.0])
                channel_id += 3
                channel_lst = channel_lst + [ new_joint.name for i in range(3) ]
                new_joint.parent.children.append(new_joint)
                curr_parent = new_joint
        elif "End" in line:
                close_flag = True
        elif "}" in line:
                if close_flag == True:
                        close_flag = False
                        continue
                else:
                        if curr_parent.parent is not None:
                                curr_parent = curr_parent.parent
f.close()
depth = 0

def iterate_through(node):
        childrens = node.children
        print(node.name + " rel " + str(node.rel_rotation_value) + " total " + str(node.rot_sum) + " channel_id " + str(node.channel_id))
        if node.parent:
                print ("== parent is " + node.parent.name)
        global depth
        depth += 1
        for child in childrens:
                iterate_through(child)


CURR_LINE_FRAME = []
def print_vec(tab):
        for el in tab:
                print(el)
##############################
# make the dico_frame_sensor
# in order to get the keyFrames
##############################
for i in range(capteur_nb):
        path_to_mtb_ascii ="C:\\Users\\ensimag\\Downloads\\sia2\\GPGPU_TP\\plugin_python\\test_bvh_stuff\\export_mtb\\export_mtb\\mtb_export_0" + str(VIDEO_ID) +"\\MT_0120083A-00" + str(VIDEO_ID) + "-000_" +  capteurs_lst[i][0]  + ".txt"
        #path_to_mtb_ascii = "/home/vivi/Desktop/sia2/GPGPU_TP/plugin_python/test_bvh_stuff/export_mtb/mtb_export_00/MT_0120083A-000-000_" + capteurs_lst[i][0] + ".txt"
        with open(path_to_mtb_ascii) as f:
                tmp = []
                lines = f.readlines()
                lines = lines[6:] # remove headers lines
                for line in lines: # keyframes of current sensor
                        curr_line = line.strip().split('\t')
                        tmp.append(curr_line)
                        # acc_x,y,z are indices 2,3,4
                        # roll, pitch, yaw rotations are indices 5,6,7
                dico_frame_sensor[capteurs_lst[i][0]] = tmp # capteurs_lst[i][0] is name of ith capteurs


def get_frame_from_channel_id(channel_id, frame_id): # channel_id is the first id of channel for the considered object
        joint_name = channel_lst[channel_id]
        global dico_frame_sensor
        global dico_joint_to_capteur
        sensor_frame_vector = dico_frame_sensor[dico_joint_to_capteur[joint_name]]
        return sensor_frame_vector[frame_id][5:] # the 3 rotation values



def compute_rotations_at_frame(currNode=None, frame_id=0):
        childrens = currNode.children
        global CURR_LINE_FRAME
        if currNode.channel_id == 0:
                # translations
                for i in range(3):
                        CURR_LINE_FRAME.append(currNode.translation_value[i])

        glob_rotation = get_frame_from_channel_id(currNode.channel_id, frame_id)
        currNode.compute_rotation([float(glob_rotation[i]) for i in range(3)])

        for i in range(3):
                CURR_LINE_FRAME.append(currNode.rel_rotation_value[i]) # order x y z

        # propagate to childrens
        for child in childrens:
                compute_rotations_at_frame(child, frame_id)


def make_frame_line_string(frame_vector):
        ret = ""
        for i in range(len(frame_vector) - 1):
                ret = ret + str(frame_vector[i]) + " "
        ret = ret + str(frame_vector[len(frame_vector) - 1])
        return ret


# update frame line
def update_line_frame(frame_id=0):
        global FRAME_CHARLST
        global CURR_LINE_FRAME
        CURR_LINE_FRAME = []
        compute_rotations_at_frame(rootNode, frame_id)
        FRAME_CHARLST.append(make_frame_line_string(CURR_LINE_FRAME))

#compute_rotations_at_frame(rootNode, 0)

i=0
for key in keys:
        print(capteurs_lst[i][1])
        print(dico_frame_sensor[key][0])
        i += 1

nb_frame = len(dico_frame_sensor[keys[0]])
for key in keys:
        if nb_frame != len(dico_frame_sensor[key]):
                print(" == ERROR ONE SENSOR HAS MORE FRAMES")

CURR_LINE_FRAME=[]
FRAME_CHARLST=[]
for frame_id in range(nb_frame):
                print(str(frame_id) + " id ")
                compute_rotations_at_frame(rootNode, frame_id)
                update_line_frame(frame_id)
                
def write_frames_to_bvh(file_obj):
        file_obj.write("\n")
        for line in FRAME_CHARLST:
                file_obj.write(line + "\n")

out_frame_bvh_file = open("skeleton_base.bvh", "a")
write_frames_to_bvh(out_frame_bvh_file)
out_frame_bvh_file.close()
