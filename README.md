# lattice

## Overview
Lattice is a programmable video mixer capable of performing real-time processing of audio or video input.  Fundamental to lattice is the concept of a video cube, a 3D data structure representing the two spatial dimensions of the screen and the temporal dimension.  User programs have access to the video cube of each input, with a time-depth defined as an arbitrary but finite length, allowing delay effects that allow for filters that operate on brightness in the time domain to create various motion blur effects and edge highlighting.  Programs send output to the video plane, the 2D surface represented on the screen.  Parameters such as cutoff frequency may be controlled via OSC messages. 

## Design
![alt text](https://raw.githubusercontent.com/jmisciagno/lattice/master/flowchart.jpg "")  
  
  
## Screenshots
![alt text](https://raw.githubusercontent.com/jmisciagno/lattice/master/screenshot1.jpg "")  
![alt text](https://raw.githubusercontent.com/jmisciagno/lattice/master/screenshot2.jpg "")  
![alt text](https://raw.githubusercontent.com/jmisciagno/lattice/master/screenshot3.jpg "")  
![alt text](https://raw.githubusercontent.com/jmisciagno/lattice/master/screenshot4.jpg "")  

## Future Development
In the future, user programs will be enabled to perform batch operations on data in the cube, allowing for implementations of the Fourier transform, and therefore operations in the frequency domain and various spectral effects with video.  Conversions between Cartesian, polar, cylindrical and spherical coordinate systems will be added to the library and various metrics for measuring the distance between points in these spaces.  Analogous operations will also be defined for color space, so that users may measure similarities of video inputs and manipulate the color palette more expressively.  Using these conversions, a library of for constructing continuous deformations on video space and color space will be made available.
