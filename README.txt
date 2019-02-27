GROUPE : ahmed mouhamaa, virgile langlet, jean-baptiste gaeng
CONTAINS : python script and c++ bvh reader that were presented during the MAYA presentation (soutenance)
== BVH READER ==
script_python fold : contains python script for loading bvh (maya)
lepTranslator : contains project and c++ code for extension that loads bvh file (drag and drop), was not renamed bvhtranslator but it does the job!
must reconfigure correctly the ide visual studio (2017) before compiling properly.


NOTE : for bvh translator extension, it uses currently an animcurve_tab of size 96 (maximum channel number),
increase this size (replace 96 by more) if you need more than 96 channels. It could easily be changed using std::vector and parsing  the number of channel by reading a frame line.
However it is faster this way (but that doesnt matter much).


ALSO CONTAINS:
== mtb parsing into bvh (not finished for computing frames properly) (also presented during maya soutenance) ==
script python for parsing extracted txt mtb data (must be put at the right path)
The script creates a bvh files from the 18 sensor captures txt files (with the FRAMES still global, need to modify them).
 
mtb files we used for it (mtb files for full body from the groupe of Odillon).
NOTE :We had conducted our own captures with sensors (a few) however we did not keep track of the proper corresponding sensors with joints, therefore we did not use them. 

IS NOT IMPLEMENTED (lack of time, may be implemented before soutenance of SIA) : correct computations of relative angle (formula R^(-1)(parent)*prod_Rcurrent)