//-
// ==========================================================================
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+

//
//  This example plugin demonstrates how to implement a Maya File Translator.
//  In this example, the user can create one or more nurbsSpheres, nurbsCylinders or nurbsCones.
//  The user can translate them around.
//
//  The LEP files can be referenced by Maya files.
//  
//  It is to be noted that this example was made to be simple.  Hence, there are limitations.
//  For example, every geometry saved will have its values reset to default,
//  except their translation if the option "Show Position" has been turned on.  To find what 
//  geometries we can export, we search by name, hence, if a polygon cube contains in its 
//  named the string "nurbsSphere", it will be written out as a nurbs sphere.
//

#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MString.h>
#include <maya/MVector.h>
#include <maya/MStringArray.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>
#include <maya/MIOStream.h>
#include <maya/MFStream.h>
#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MFnIkJoint.h> // to make the joint nodes
#include <maya/MFnAnimCurve.h> // for keyframes?
#include <maya/MNamespace.h>
#include <maya/MTime.h>
#include <string.h>
#include <list>

//This is the backbone for creating a MPxFileTranslator
class LepTranslator : public MPxFileTranslator {
public:

    //Constructor
    LepTranslator () {};
    //Destructor
    virtual            ~LepTranslator () {};

    //This tells maya that the translator can read files.
    //Basically, you can import or load with your translator.
    bool haveReadMethod() const { return true; }

    //This tells maya that the translator can write files.
    //Basically, you can export or save with your translator.
    bool haveWriteMethod() const { return true; }

    //If this method returns true, and the lep file is referenced in a scene, the write method will be
    //called when a write operation is performed on the parent file.  This use is for users who wish
    //to implement a custom file referencing system.
    //For this example, we will return false as we will use Maya's file referencing system.
    bool haveReferenceMethod() const { return false; }

    //If this method returns true, it means we support namespaces.
    bool haveNamespaceSupport()    const { return true; }

    //This method is used by Maya to create instances of the translator.
    static void* creator();
    
    //This returns the default extension ".lep" in this case.
    MString defaultExtension () const;

    //If this method returns true it means that the translator can handle opening files 
    //as well as importing them.
    //If the method returns false then only imports are handled. The difference between 
    //an open and an import is that the scene is cleared(e.g. 'file -new') prior to an 
    //open, which may affect the behaviour of the translator.
    bool canBeOpened() const { return true; }

    //Maya will call this method to determine if our translator
    //is capable of handling this file.
    MFileKind identifyFile (    const MFileObject& fileName,
                                                const char* buffer,
                                                short size) const;

    //This function is called by maya when import or open is called.
    MStatus reader ( const MFileObject& file,
                                        const MString& optionsString,
                            MPxFileTranslator::FileAccessMode mode);

    //This function is called by maya when export or save is called.
    MStatus writer ( const MFileObject& file,
                                        const MString& optionsString,
                             MPxFileTranslator::FileAccessMode mode);

private:
    //The magic string to verify it's a BVH file
    //simply "HIERARCHY"
    static MString const magic;
};

//Creates one instance of the LepTranslator
void* LepTranslator::creator()
{
    return new LepTranslator();
}

// Initialize our magic string
MString const LepTranslator::magic("HIERARCHY");


bool contains_mstring(MStringArray &m_array, std::string str) {
	for (unsigned int i = 0; i < m_array.length(); i++) {
		if (m_array[i] == str.c_str()) {
			return true; // return true at first occurence
		}
	}
	return false;
}

/* 
* Helper function translates bvh notation
* into maya notation
*/
MString maya_notation(MString str_) {
	if (str_ == "Xposition") {
		return "translateX";
	}
	else if (str_ == "Yposition") {
		return "translateY";
	}
	else if (str_ == "Zposition") {
		return "translationZ";
	}
	else if (str_ == "Xrotation") {
		return "rotateX";
	}
	else if (str_ == "Yrotation") {
		return "rotateY";
	}
	else if (str_ == "Zrotation") {
		return "rotateZ";
	}
	return "";
}




MStatus make_joint(MObject &parent, MObject &new_obj, MString name)
{
	MStatus ret;
	MFnIkJoint mfn_joint;
	mfn_joint.create(parent, &ret);
	mfn_joint.setName(name);
	new_obj = mfn_joint.object();
	return ret;
}

MStringArray rstripArray(const MStringArray& array)
{
	MString tmp_string;
	const char *curr_chaine = NULL;
	unsigned int curr_length = 0;
	MStringArray ret(array.length(), MString(""));

	for (unsigned int i = 0; i < array.length(); i++) {

		std::string curr_string(array[i].asChar());
		std::string final;
		for (int j = 0; j < curr_string.length(); j++) {
			if ((curr_string[j] != ' ') && (curr_string[j] != '\n') &&
			   (curr_string[j] != '\0') && (curr_string[j] != '\t')) {
				final += curr_string[j];
			}
		}
		ret[i] = MString(final.c_str());
	}
	return ret;
}


MStringArray lst_to_array(std::list<MString> &channel_lst)
{
	MStringArray ret((int)channel_lst.size(), MString(""));
	std::list<MString>::iterator it;
	int p = 0;
	for (it = channel_lst.begin(); it != channel_lst.end(); ++it) {
		ret[p] = *it;
		p++;
	}
	return ret;
}

MStringArray array_from_string(MString mstring)
{
	std::string tmp(mstring.asChar());
	std::string curr_string("");
	int curr_index = 0;
	bool making_word = false;
	unsigned int tmp_length = tmp.length();
	MStringArray ret(96, MString(""));
	for (unsigned int i = 0; i < tmp_length; i++) {
		if ((tmp[i] != ' ') && making_word) {
			// continue current word
			curr_string = curr_string + tmp[i];
		}
		else if ((tmp[i] == ' ') && making_word) { // end of a word
			making_word = false;
			ret[curr_index] = MString(curr_string.c_str());
			curr_string = "";
			curr_index += 1;
		}
		else if ((tmp[i] != ' ') && (!making_word)) {
			// beginning of a new word
			making_word = true;
			curr_string = curr_string + tmp[i];
		}
	}
	//last word
	ret[curr_index] = MString(curr_string.c_str());
	return ret;
}
MString replace_tab_by_spaces(MString &str_wtab)
{
	std::string tmp_string(str_wtab.asChar());
	std::string ret("");
	//MString ret("");
	for (int i = 0; i < tmp_string.length(); i++) {
		if ((tmp_string[i] != '\t') &&
		((tmp_string[i] == '0') ||
			(tmp_string[i] == '1') ||
			(tmp_string[i] == '2') ||
			(tmp_string[i] == '3') ||
			(tmp_string[i] == '4') ||
			(tmp_string[i] == '5') ||
			(tmp_string[i] == '6') ||
			(tmp_string[i] == '7') ||
			(tmp_string[i] == '8') ||
			(tmp_string[i] == '9') ||
			(tmp_string[i] == '-') ||
			(tmp_string[i] == '.'))
			) {
			ret = ret + tmp_string[i];
		}
		else {
			ret = ret + ' ';
		}
	}
	MString res(ret.c_str());
	return res;
}


// An LEP file is an ascii whose first line contains the string <LEP>.
// The read does not support comments, and assumes that the each
// subsequent line of the file contains a valid MEL command that can
// be executed via the "executeCommand" method of the MGlobal class.
//
MStatus LepTranslator::reader ( const MFileObject& file,
                                const MString& options,
                                MPxFileTranslator::FileAccessMode mode)
{    
    const MString fname = file.fullName();

    MStatus rval(MS::kSuccess);
    const int maxLineSize = 1024;
    char buf[maxLineSize];

    ifstream inputfile(fname.asChar(), ios::in);
    if (!inputfile) {
        // open failed
        cerr << fname << ": could not be opened for reading\n";
        return MS::kFailure;
    }

    if (!inputfile.getline (buf, maxLineSize)) {
        cout << "file " << fname << " contained no lines ... aborting\n";
        return MS::kFailure;
    }

    if (0 != strncmp(buf, magic.asChar(), magic.length())) {
        cerr << "first line of file " << fname;
        cerr << " did not contain " << magic.asChar() << " ... aborting\n";
        return MS::kFailure;
    }


	/*
	*  make use  of MFnIkJoint MFnAnimCurve OpenMayaAnimlib
	*/
	bool close_flag = false;
	bool motion_frame = false;
	MObject rootNode = MObject::kNullObj;
	MObject curr_parent = MObject::kNullObj; //current parent
	MObject new_parent = MObject::kNullObj;

	std::list<MString> channel_lst; // list of channels in use

	/* main loop */
	MSelectionList select_lst;
	MGlobal::getActiveSelectionList(select_lst); // get activeSelectionList

	MStatus ret;
	MFnIkJoint mfn_joint;
	MFnIkJoint mfn_debug;
	MFnIkJoint mfn_util;
	int time_frame = 0;

	MStringArray channel_array;
	//mfn_joint.create(MObject::kNullObj, &ret);
	//mfn_joint.setName(MString("ROOT"));
	//rootNode = mfn_joint.object();
	bool change_to_array = true;


	//int[] tab_int;
	//MObject *obj_tab[96];
	MFnAnimCurve animcurve_tab[96]; // one anim curve per channel
    while (inputfile.getline (buf, maxLineSize)) {
        MString cmdString;
        cmdString.set(buf);

		MStringArray curr_line_array;
		cmdString.split(' ', curr_line_array);
		curr_line_array = rstripArray(curr_line_array); // remove whitespaces

		if (!motion_frame) {
			// debug 
			if (curr_parent != MObject::kNullObj) {
				mfn_debug.setObject(curr_parent);
				cerr << "name of object is" << mfn_debug.name() << endl;
				cerr << "fullpathname is" << mfn_debug.fullPathName() << endl;
			}
			// if startswith root
			if (curr_line_array[0] == "ROOT") {
				// supports only one root
				mfn_joint.create(MObject::kNullObj);
				mfn_joint.setName(curr_line_array[1]);
				curr_parent = mfn_joint.object();
				rootNode = mfn_joint.object();
				/*
				// root becomes the hip joint
				if (rootNode != MObject::kNullObj) {
					mfn_joint.create(rootNode, &ret);
					mfn_joint.setName(MString("Hip"));
					curr_parent = mfn_joint.object();
				}
				else {
					// update the parenthood
					mfn_joint.create(curr_parent, &ret);
					mfn_joint.setName(MString(curr_line_array[1]));
					curr_parent = mfn_joint.object();
				}*/
			}
			else if (contains_mstring(curr_line_array, "MOTION")) {
				motion_frame = true; // starts reading the keyframes
			}
			else if (contains_mstring(curr_line_array, "JOINT")) {
				mfn_joint.create(curr_parent, &ret);
				mfn_joint.setName(curr_line_array[curr_line_array.length() - 1]);
				curr_parent = mfn_joint.object();
			} else if (contains_mstring(curr_line_array, "End")) { // if contains "End"
				close_flag = true;
			}
			else if (contains_mstring(curr_line_array, "}")) {
				if (close_flag) {
					close_flag = false;
						continue;
				}

				if (curr_parent != MObject::kNullObj) { // if not none
					// we go one level above
					MFnIkJoint joint_handle(curr_parent);
					curr_parent = joint_handle.parent(joint_handle.parentCount() - 1); // this assumes only one parent
				}

			} else if (contains_mstring(curr_line_array, "CHANNELS")) {
				// curr_line_array should be curr_line.strip().split(" ")
				int chan_nb_param = curr_line_array[1].asInt(); // rstrip() needed
				for (int i = 0; i<chan_nb_param; i++) {
					mfn_util.setObject(curr_parent);
					MString tmp = mfn_util.name(); /*fullPathName() + MString(".") + MString(maya_notation(curr_line_array[i + 2]))*/
					channel_lst.push_back(tmp);
				}

			} else if (contains_mstring(curr_line_array, "OFFSET")) {

				// curr_offset should be curr_line.strip().split(" ")
				mfn_joint.setObject(curr_parent);
				MString joint_name = mfn_joint.name();
				if (close_flag) {
					joint_name = joint_name + MString("_tip");
				}
				if ( !curr_parent.isNull()) {
					mfn_joint.setRotationOrder(MTransformationMatrix::RotationOrder::kXYZ, false); // reorder = false?
					/*if (!mfn_joint.parent(0).isNull()) {
						mfn_util.setObject(mfn_joint.parent(0));
						mfn_joint.setTranslation(mfn_util.getTranslation(MSpace::kTransform), MSpace::kTransform);
					}*/
					//if (!mfn_joint.parent(0).isNull()) {
						//mfn_util.setObject(mfn_joint.parent(0));
					mfn_joint.setTranslation(MVector(atof(curr_line_array[1].asChar()), atof(curr_line_array[2].asChar()), atof(curr_line_array[3].asChar())), MSpace::kTransform);
					//}
				}
			}
		}
		else { 
			
			if (change_to_array) {
				channel_array = lst_to_array(channel_lst);
				change_to_array = false;
			}

			// skip first two lines of motion section
			if (contains_mstring(curr_line_array, "Frames:")) {
				continue;
			}
			if (contains_mstring(curr_line_array, "Frame")) {
				continue;
			}
 			
			cerr << "line : " << cmdString << endl;
			cerr << "channel " << channel_array << endl;
			MString cmd_string_wo_tab = replace_tab_by_spaces(cmdString);
			curr_line_array = array_from_string(cmd_string_wo_tab);
			cerr << "curr_line_array is " << curr_line_array << endl;
			curr_line_array = rstripArray(curr_line_array); // remove whitespaces
			MTime maya_time((double) (time_frame), MTime::kFilm);

			MSelectionList curr_selection; // used to store object we select using their names
			MObject curr_attribute; // store attribute for the current object considered (ie translationX)
			MObject curr_attribute_rotate;
			for (unsigned int i = 0; i < curr_line_array.length(); i++) { // for each frame at time_frame $time_frame
				MSelectionList curr_selection;
				//cerr << "selecting by name object " << channel_array[i] << endl;
				MGlobal::getSelectionListByName(channel_array[i], curr_selection); // find by name (not fullPathName?
				MItSelectionList iter(curr_selection, MFn::kJoint, &ret);
				MFnIkJoint mfn_test;
				MObject curr_obj;
				int itercount = 0;
				//cerr << "curr value of i is : " << i << endl;
				//cerr << "curr_line_array length" << curr_line_array.length() << endl;
				for (; !iter.isDone(); iter.next())
				{
					iter.getDependNode(curr_obj); // get the MObject node (result in curr_obj) that correspond to current iterator value
					mfn_test.setObject(curr_obj); // bind MFnIkJoint function set on curr_obj, used to retrieve the attribute by their names

					if (i < 3) { // because root node has 6 channels, the 3 first being translations, the first 3 are translations
						if ((i % 3) == 0) {
						
							if (time_frame == 0) {

								curr_attribute = mfn_test.attribute("translateX", &ret);
								if (ret != MStatus::kSuccess) {
									cerr << "FAILED TO RETRIEVE ATTRIBUTE TRANSLATEX OF HIP" << endl;
								}

								// if first time we do the keyframe processing (ie time_frame = 0)
								// then we have not yet created the animcurves
								animcurve_tab[i].create(curr_obj, curr_attribute, NULL, &ret);
								if (ret != MStatus::kSuccess) {
									cerr << "FAILED TO CREATE ATTRIBUTE TRANSLATEX OF HIP" << endl;
								}
							}

							animcurve_tab[i].addKeyframe(maya_time, atof(curr_line_array[i].asChar()));
						}
						else if ((i % 3) == 1) {

							if (time_frame == 0) {
								curr_attribute = mfn_test.attribute("translateY", &ret);
								animcurve_tab[i].create(curr_obj, curr_attribute, NULL, &ret);
							}

							animcurve_tab[i].addKeyframe(maya_time, atof(curr_line_array[i].asChar()));
						}
						else if ((i % 3) == 2) {
							
							if (time_frame == 0) {
								curr_attribute = mfn_test.attribute("translateZ", &ret);
								animcurve_tab[i].create(curr_obj, curr_attribute, NULL, &ret);
							}

							MStatus test_status = animcurve_tab[i].addKeyframe(maya_time, atof(curr_line_array[i].asChar()));
							if (test_status != MStatus::kSuccess) {
								cerr << "ERROR SETTING TZ KEYFRAME" << endl;
							}
						}
					}
					else {
						if ((i % 3) == 0) {

							if (time_frame == 0) {
								curr_attribute_rotate = mfn_test.attribute("rotateZ", &ret);
								if (ret != MStatus::kSuccess) {
									cerr << "FAILED TO RETRIEVE ATTRIBUTE ROTATEZ" << endl;
								}

								animcurve_tab[i].create(curr_obj, curr_attribute_rotate, NULL, &ret);
							}

							MStatus rz_status = animcurve_tab[i].addKeyframe(maya_time, atof(curr_line_array[i].asChar())*(3.1415 / 180));
							/*cerr << "trying to add keyframe added for RZ " << curr_line_array[i] << endl;
							cerr << "atof is " << atof(curr_line_array[i].asChar()) << endl;
							cerr << "time_frame is " << time_frame << endl;*/
							if (rz_status != MStatus::kSuccess) {
								cerr << "ERROR SETTING RZ KEYFRAME" << endl;

							}
						}
						else if ((i % 3) == 1) {

							if (time_frame == 0) {
								curr_attribute_rotate = mfn_test.attribute("rotateY", &ret);
								animcurve_tab[i].create(curr_obj, curr_attribute_rotate, NULL, &ret);
							}

							animcurve_tab[i].addKeyframe(maya_time, atof(curr_line_array[i].asChar())*(3.1415/180));
						}
						else if ((i % 3) == 2) {

							if (time_frame == 0) {
								curr_attribute_rotate = mfn_test.attribute("rotateX", &ret);
								animcurve_tab[i].create(curr_obj, curr_attribute_rotate, NULL, &ret);
							}

							animcurve_tab[i].addKeyframe(maya_time, atof(curr_line_array[i].asChar())*( 3.1415 / 180));
						}
					}
					itercount += 1;
				}
				if (itercount > 1) {
					// for our code to work, we for now suppose each joint
					// has a different name since we do not use the
					// fullPathName to retrieve them, but only their
					// .name() (for example "l_hip"
					cerr << "==== ERROR TWO OBJECTS HAVE SAME NAME " << endl;
				}
			}
			time_frame += 1; // increase time
			
		}
    }
    inputfile.close();

    return rval;
}

// The currently recognised primitives.
const char* primitiveStrings[] = {
    "nurbsSphere",
    "nurbsCone",
    "nurbsCylinder",
};
const unsigned numPrimitives = sizeof(primitiveStrings) / sizeof(char*);

// Corresponding commands to create the primitives
const char* primitiveCommands[] = {
    "sphere",
    "cone",
    "cylinder",
};

//The writer simply goes gathers all objects from the scene.
//We will check if the object has a transform, if so, we will check
//if it's either a nurbsSphere, nurbsCone or nurbsCylinder.  If so,
//we will write it out.
// We won't be implementing a bvh writer for now
MStatus LepTranslator::writer ( const MFileObject& file,
                                const MString& options,
                                MPxFileTranslator::FileAccessMode mode)
{
    MStatus status;
	bool showPositions = false;
    unsigned int  i;
    const MString fname = file.fullName();

    ofstream newf(fname.asChar(), ios::out);
    if (!newf) {
        // open failed
        cerr << fname << ": could not be opened for reading\n";
        return MS::kFailure;
    }
    newf.setf(ios::unitbuf);

    if (options.length() > 0) {
        // Start parsing.
        MStringArray optionList;
        MStringArray theOption;
        options.split(';', optionList);    // break out all the options.
        
        for( i = 0; i < optionList.length(); ++i ){
            theOption.clear();
            optionList[i].split( '=', theOption );
            if( theOption[0] == MString("showPositions") &&
                                                    theOption.length() > 1 ) {
                if( theOption[1].asInt() > 0 ){
                    showPositions = true;
                }else{
                    showPositions = false;
                }
            }
        }
    }

    // output our magic number
    newf << "<LEP>\n";

    MItDag dagIterator( MItDag::kBreadthFirst, MFn::kInvalid, &status);

    if ( !status) {
        status.perror ("Failure in DAG iterator setup");
        return MS::kFailure;
    }

    MSelectionList selection;
    MGlobal::getActiveSelectionList (selection);
    MItSelectionList selIterator (selection, MFn::kDagNode);

    bool done = false;
    while (true) 
    {
        MObject currentNode;
        switch (mode)
        {
            case MPxFileTranslator::kSaveAccessMode:
            case MPxFileTranslator::kExportAccessMode:
                if (dagIterator.isDone ())
                    done = true;
                else {
                    currentNode = dagIterator.item ();
                    dagIterator.next ();
                }
                break;
            case MPxFileTranslator::kExportActiveAccessMode:
                if (selIterator.isDone ())
                    done = true;
                else {
                    selIterator.getDependNode (currentNode);
                    selIterator.next ();
                }
                break;
            default:
                cerr << "Unrecognized write mode: " << mode << endl;
                break;
        }
        if (done)
            break;

        //We only care about nodes that are transforms
        MFnTransform dagNode(currentNode, &status);
        if ( status == MS::kSuccess ) 
        {
            MString nodeNameNoNamespace=MNamespace::stripNamespaceFromName(dagNode.name());
            for (i = 0; i < numPrimitives; ++i) {                
                if(nodeNameNoNamespace.indexW(primitiveStrings[i]) >= 0){
                    // This is a node we support
                    newf << primitiveCommands[i] << " -n " << nodeNameNoNamespace << endl;
                    if (showPositions) {
                        MVector pos;
                        pos = dagNode.getTranslation(MSpace::kObject);
                        newf << "move " << pos.x << " " << pos.y << " " << pos.z << endl;
                    }
                }
            }
        }//if (status == MS::kSuccess)
    }//while loop

    newf.close();
    return MS::kSuccess;
}

// Whenever Maya needs to know the preferred extension of this file format,
// it calls this method. For example, if the user tries to save a file called
// "test" using the Save As dialog, Maya will call this method and actually
// save it as "test.lep". Note that the period should *not* be included in
// the extension.
MString LepTranslator::defaultExtension () const
{
    return "bvh";
}


//This method is pretty simple, maya will call this function
//to make sure it is really a file from our translator.
//To make sure, we have a little magic number and we verify against it.
MPxFileTranslator::MFileKind LepTranslator::identifyFile (
                                        const MFileObject& fileName,
                                        const char* buffer,
                                        short size) const
{
    // Check the buffer for the "LEP" magic number, the
    // string "<LEP>\n"

    MFileKind rval = kNotMyFileType;

    if ((size >= (short)magic.length()) &&
        (0 == strncmp(buffer, magic.asChar(), magic.length()))) 
    {
        rval = kIsMyFileType;
    }
    return rval;
}

MStatus initializePlugin( MObject obj )
{
    MStatus   status;
    MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

    // Register the translator with the system
    // The last boolean in this method is very important.
    // It should be set to true if the reader method in the derived class
    // intends to issue MEL commands via the MGlobal::executeCommand 
    // method.  Setting this to true will slow down the creation of
    // new objects, but allows MEL commands other than those that are
    // part of the Maya Ascii file format to function correctly.
    status =  plugin.registerFileTranslator( "Lep",
                                        "lepTranslator.rgb",
                                        LepTranslator::creator,
                                        "lepTranslatorOpts",
                                        "showPositions=1",
                                        true );
    if (!status) 
    {
        status.perror("registerFileTranslator");
        return status;
    }

    return status;
}

MStatus uninitializePlugin( MObject obj )
{
    MStatus   status;
    MFnPlugin plugin( obj );

    status =  plugin.deregisterFileTranslator( "Lep" );
    if (!status)
    {
        status.perror("deregisterFileTranslator");
        return status;
    }

    return status;
}

