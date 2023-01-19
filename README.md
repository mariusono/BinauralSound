# BinauralSound
An audio plugin for binaural sound. Moves a mono sound source around a 1 meter sphere around the listener's head.

Implements a combination of the papers: "An efficient HRTF model for 3-D sound" and "A Structural Model for Binaural Sound Synthesis" both written by Brown and Duda. 

User can change the location of the sound source by changing the azimuth and elevation sliders. Azimuth controls the angle of the sound source with the median plane and the elevation controls the angle wrt. the horizontal plane. 

The .vst3 is at BinauralSound/Builds/MacOSX/build/Release/BinauralSound.vst3 . Download it and put it in your default VST3 plugin folder ( for mac it usually is /Users/YOUR_USER/Library/Audio/Plug-Ins/VST3 . Before you open your DAW, right click on the .vst3 and click "open with". Choose a random program, it will not work anyway -- I open it with Adobe Acrobat Reader, and when promted with the "developer not recognized" window, click "Open". You'll probably get an error but it doesn't matter. Now you can open your DAW and hopefully when you rescan for plugins you will find it there. 
