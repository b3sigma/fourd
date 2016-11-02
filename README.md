Four dimensional VR minecraft thing!
OpenGL, Vive/Oculus and C++11ish.
Actually maybe I should advertise as 5 dimensional as it's 4 space + 1 time!

![Project picture](data/textures/poster.jpg?raw=true)

Getting:
* Make sure you use --recursive on your clone command as this project uses submodules.
* If you already didn't do that, do this sequence:
** cd fourd
** git submodule init
** git submodule update
* If you don't need a submodule because you already have something installed, you can do them manually. You may avoid the submodulepocalype this way.

Running on Windows:
* open up the .sln in under ./project/, build, run

Running on Linux (commands assumed from fourd dir):
* sudo aptitude install cmake libglew-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxmu-dev
* Next we need to build glfw. (Needless detail note: as there wasn't a package available, I had to build from source. As of this writing there is still no package for my distro, but if you would rather use your distro I think it was only a fullscreen bug that is probably already fixed in the mainline.
* cd glfw && mkdir build && cd build && cmake ..
* For any issues cmake reports, aptitude install the package or get it from source
* make && sudo make install
* That should have installed glfw, so now we should be able to finish compiling fourd, so
* cd ../..
* make
* ./fourd

Controls UI:
* w/s = forward/backward
* a/d = strafe left/right
* q/e = strafe up/down
* r/f = inward/outward
* t/g = roll left/right
* y/h = rotate between inside right
* u/j = rotate between inside up
* i/k = animated pi/2 rotation between inside right (seems like miegakure button)
* o/l = animated pi/2 rotation between inside up

VR:
* V = enable VR (vive/rift only)
* F = fullscreen

Rendering UI:
* 0-9&* load different base shapes and different levels. (Default is quaxols in 4d.)
* !-^) load different shaders and alpha/depth options
* % loads settings for a nearly flat sliced w-projection
* x/c Decrease/increase near w-plane
* v/b Decrease/increase far w-plane
* n/m Decrease/increase w-plane far/near ratio (alternative to FOV)
* ] toggles w-ortho and w-projection (w-ortho can also be set by far/near ratio=1)

Action UI (vague due to being in flux):
* z add quaxol
* Z remove quaxol
* X add bunch of quaxols in a line
* ` lock mouse to window
* Esc quit

Minimal TODOs:
* Better fix for solid/blendy interaction alignment

Game TODOs:
* Add VR UI input?
* Make separate mouse ui/game modes that steal key input
* more visualization attempts
** Multi-texture render target?
*** Depthy to 0, alpha to 1
*** would still need compose passt
** Single pass conditional depth writes?
** sub-surface w mode?
** Alpha test cleaner clipping?
* cleanup rift positioning
* refactor keyboard input
* auto shape gen
* quaxol bitpacked rendering
* quaxol chunking
* quaxol streaming
* refactor input keys
* add console
* Load compressed textures with mipmaps directly
* Cleanup quaxol size/worldscale
* Separate physics into modular system
* Improve physics response loop
* Iterate on multipass shader in VR
* Improve multipass fillrate performance (overdraw is overboard)
** Lower resolution blendy?
** Try blend vs compose for VR final pass
* Finish the portal system
* Add a map
* Add the flipped signs.

QM/Physics TODOs:
* Integrate the python branch
* Finish the PIMC
* Add spin system
* lorentz transformation rendering?
* gr rendering?

General TODOs:
* Cleanup matrix math
* Add text window with command entry
** Done in python branch
* Fix portability, CMake
** This is necessary to integrate the python branch.

Using a Makefile from Michael Crawford. Thanks!
Using a Todd-Coxeter solver from Fritz Obermeyer. Thanks!
Using an arg parser from Ian Clarkson. Thanks!
Using a signals/slots impl from Patrick Hogan. Thanks!
Using stb libs from Sean Barrett. Thanks!
