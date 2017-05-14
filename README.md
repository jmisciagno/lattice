# lattice

* Overview
Lattice is a programmable video mixer capable of performing real-time processing of audio or video input, with optional OSC messages for controlling user-defined parameters.  Fundamental to lattice is the concept of a video cube, a 3D data structure representing the two spatial dimensions of the screen and the temporal dimension.  User programs have access to the video cube of each input, with a time-depth defined as an arbitrary but finite length, allowing delay effects that allow for filters that operate on brightness in the time domain to create various motion blur effects and edge highlighting.  Programs send output to the video plane, the 2D surface represented on the screen.  

## Design
![alt text](https://raw.githubusercontent.com/jmisciagno/lattice/master/flowchart.jpg "")  

## Screenshots
![alt text](https://raw.githubusercontent.com/jmisciagno/lattice/master/pic1.jpg "")  
![alt text](https://raw.githubusercontent.com/jmisciagno/lattice/master/pic2.jpg "")  
![alt text](https://raw.githubusercontent.com/jmisciagno/lattice/master/pic3.jpg "")  
