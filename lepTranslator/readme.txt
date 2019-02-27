
NOTE : for bvh translator extension, it uses currently an animcurve_tab of size 96 (maximum channel number),
increase this size (replace 96 by more) if you need more than 96 channels. It could easily be changed using std::vector and parsing  the number of channel by reading a frame line.
However it is faster this way (but that doesnt matter much).
