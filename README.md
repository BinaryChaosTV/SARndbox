# ARSandbox
This is a forked Repo from the UC Davis' Augmented Reality Sandbox created by Oliver Kreylos. With his permission, and under the GPLv2 license, I am sharing my own changes (albeit minor) to the Sandbox as I set it up at my school. I am mostly maintaining this for myself if it were to ever break or become corrupted, but also to hopefully make it easier for anyone else wanting to create their own ARSandbox. Last but not least, I am also sharing these instructions in the event they were ever lost. At the time of writing, the dedicated website for the ARSandbox is now inaccessible. This made my job slightly more difficult as I had some specific needs for my project.

## Software Required
These instructions are up-to-date with the following software versions:

- Linux Mint 20.3 ("Una") with MATE desktop environment, 64-bit version. Currently supported until April 2023. [Download Here](https://linuxmint.com/edition.php?id=293)
- Version 8.0-002 of the Vrui VR Development Toolkit (automatically selected by Vrui installation script). [Download Here](https://web.cs.ucdavis.edu/~okreylos/ResDev/Vrui/index.html)
- Version 3.10 of the Kinect 3D Video Package. [Download Here](https://web.cs.ucdavis.edu/~okreylos/ResDev/Kinect/index.html)
- Version 2.8 of the Augmented Reality Sandbox. [Download Here](https://web.cs.ucdavis.edu/~okreylos/ResDev/SARndbox/)

Additionally, I used some scripts from [RiverWeyTrust/ARSandbox-Adds](https://github.com/RiverWeyTrust/ARSandbox-Adds) to add weather effects such as lava and snow to our ARSandbox. They are also included in the files above, but it's necessary to give them a shoutout for their work.

## Hardware Required
Find Hardware Instructions [Here](https://web.cs.ucdavis.edu/~okreylos/ResDev/SARndbox/).

An AR Sandbox requires the following hardware components:

- A computer with a good graphics card, running any version of Linux. The AR Sandbox software, in principle, also runs on Mac OS X, but we advise against it. We strongly recommend using desktop computers over laptop computers. For one, laptop computers powerful enough to run an AR Sandbox are typically more expensive than desktop computers of comparable power; second, laptop computers with high-end Nvidia graphics cards often contain so-called Nvidia Optimus technology to reduce system power usage by dynamically switching between the high-performance discrete graphics processing unit (GPU) and the main CPU's low-performance integrated graphics processor (IGP). Optimus is not fully supported on Linux, and may prevent access to the discrete graphics card entirely. Our current PC uses an Nvidia GTX 1060.
- The AR Sandbox does not run under Windows, and neither does it run from within a virtual machine. It either requires a PC exclusively running Linux, or a dual-boot setup with Linux and Windows.
- A Microsoft Kinect 3D camera. The AR Sandbox software, or rather the underlying Kinect 3D Video Package as of version 2.8, supports all three models of the first-generation Kinect (Kinect-for-Xbox 1414 and 1473 and Kinect for Windows). All three are functionally identical, so get the cheapest model you can find. Note: The second-generation Kinect (Kinect for Xbox One or Kinect for Windows v2) is experimentally supported by the AR Sandbox software as of version 2.4, when using version 3.4 of the Kinect 3D Video Package.
- A digital data projector with a digital video interface, such as HDMI, DVI, or DisplayPort.
- A sandbox with a way to mount the Kinect camera and the projector above the sandbox.
- Sand. Kinetic Sand is recommended as it's highly maleable.

## Installation

Taken from the following page: [Software Installation](https://web.cs.ucdavis.edu/~okreylos/ResDev/SARndbox/index.html)

This page holds complete, step-by-step installation and set-up instructions for the AR Sandbox software, starting from a blank desktop PC with an Nvidia GeForce graphics processing unit (GPU). These instructions are based on the current version of the Linux Mint operating system. Due to increasing weirdness in recent versions of Mac OS X (starting with version 10.7), we can no longer recommend Mac computers and Mac OS X to drive AR Sandbox installations.

Installation steps 1 to 5 install Linux and the AR Sandbox software, including its underlying Vrui and Kinect packages.

#### Step 1: Install Linux

Install the 64-bit version of Linux Mint 20.3 ("Una") with MATE desktop environment on a blank desktop computer. This needs to be a real computer; the AR Sandbox does not work from inside a virtual machine.

If you plan to run the AR Sandbox as a computational appliance, i.e., a closed system with no Internet connection, keyboard, mouse, or monitor besides the projector, where the AR Sandbox application starts automatically when you power on the PC, you should prepare for this early on during installation of Linux. One of the installation steps is to create a user account on the new operating system. At that point, check the option to log into that account automatically, and do not assign a password. Then, after the installation is done, follow optional Step 16 and Step 17.

#### Step 2: Install Nvidia Driver

Install vendor-supplied drivers for your Nvidia graphics card. Open the Control Center, select "Driver Manager," wait for the panel to show up and the list of available drivers to be populated, and then select the recommended Nvidia binary driver and press "Apply Changes." Then wait until the change is complete (might take a while), and reboot your computer when prompted. This process is described in detail in the illustrated step-by-step guide to installing Linux Mint 20 ("Una").

After installing the driver and rebooting the computer, check that the driver is working correctly by opening a terminal window and entering precisely the following command:

	glxinfo | grep vendor

The terminal should reply with the following:

	server glx vendor string: NVIDIA Corporation
	client glx vendor string: NVIDIA Corporation
	OpenGL vendor string: NVIDIA Corporation

If the reply is different, specifically, if it does not print "NVIDIA Corporation" in all three lines, something went wrong with driver installation. This issue needs to be corrected before continuing.

#### Step 3: Install the Vrui VR Development Toolkit

Open a terminal window and enter precisely the following commands (if in doubt, copy & paste directly from this web page, one line at a time). ~ is shorthand for the user's home directory:

	cd ~
	wget http://web.cs.ucdavis.edu/~okreylos/ResDev/Vrui/Build-Ubuntu.sh
	bash Build-Ubuntu.sh

The script run in the last command will ask you for your user's password to install prerequisite libraries, and then build the Vrui VR Toolkit. That will take a little while and produce lots of output, and at the end you'll be greeted by a spinning globe in a window.

This is a good opportunity to familiarize yourself with Vrui's basic user interface. Several utilities used in subsequent installation steps, and even the AR Sandbox itself, are Vrui applications. The UI is described in some detail in Vrui's online documentation, specifically the "Using Vrui Applications" page, but here are the most important bits:

- **Main Menu:** Press (and hold) the right mouse button to pop up the main application menu. Then move the mouse to the "Rotate Earth" menu entry, and release the right mouse button. The globe should stop spinning.
- **Rotate:** While pressing and holding the left mouse button, moving the mouse will rotate the 3D view in the window around the window center.
- **Pan:** To "pan," i.e., to move the globe left/right/up/down in the window, press and hold the "z" key on the keyboard, and move the mouse (without pressing any mouse buttons) left/right/up/down. Using keyboard keys as extra mouse buttons in this way is a common approach in Vrui applications.
- **Scale:** To "scale," i.e., to grow or shrink the globe in the window, press and hold both the "z" key and the left mouse button, and move the mouse up/down.
- **Recenter:** Should you ever get lost, i.e., nothing shows up in the window and you don't know how to get the contents back, you can reset the view to the default by either (1) selecting "Reset View" from the "View" sub-menu of the "Vrui System" sub-menu, or (2)  by pressing "Win+Home," i.e., holding the "Windows" key, and then pressing the "Home" key.
- **Tool Assignment:** Now press and hold some keyboard key, let's say "1." There is no function currently assigned to that key, thus, Vrui will pop up the tool selection menu, showing many different functions that can be dynamically assigned to it. The root (top-level) tool menu contains the most important application-specific tools such as the "Extract Planes" tool used in Step 8 as well as several sub-menus bundling tools with common purposes. Move the mouse (without pressing any other keys or buttons) to open the "Utility" sub-menu, then move to the "Curve Editor" entry, and let go of the "1" key. This will assign a Curve Editor tool to the key, and pop open the Curve Editor's control dialog. You can move the dialog by grabbing its blue title bar with the left mouse button, and dragging it with the mouse. You can also use the left mouse button to interact with the dialog, e.g., drag the "Line Width" slider, or select different colors. To draw curves, move the mouse back into the main window, press and hold "1," and move the mouse. Dynamically assigning functions to mouse buttons or keyboard keys is a very common operation in Vrui applications. To delete the Curve Editor tool, move the mouse into the "tool trash can," the red rectangle in the lower-left window corner, and press and release the "1" key.

When you are done exploring, close the globe window and go back to the same terminal window as before to continue.

Once the spinning globe appeared, you can delete the Vrui installation script by running in the same terminal window:

	rm ~/Build-Ubuntu.sh

#### Step 3a: Adjusting the Screen Size

While inside the spinning globe application that was started at the end of Step 3, open the application's main menu by pressing and holding the right mouse button, and check the text size of the menu items. Is the menu readable? Is the text tiny or gigantic?

A potential issue is that the Vrui toolkit tries to create a calibrated display, meaning that if an application draws a 5" line, that line should appear on the screen exactly 5" long. This feature relies on a connected display reporting its correct size to the operating system. While that works for most computer monitors, it does not work for projectors  -- a projector does not know its display size, as that depends on throw distance and zoom factor. As a result, projectors often report no or wildly inaccurate display sizes, and Vrui messes up by taking those at face value. In short, Vrui's automatic screen size feature might have to be disabled to work in an AR Sandbox context.

The best way to disable this feature for all Vrui applications is to edit Vrui's central configuration file, `/usr/local/etc/Vrui-8.0/Vrui.cfg`, with the xed text editor. In a terminal window, run:

	sudo xed /usr/local/etc/Vrui-8.0/Vrui.cfg

and enter your password if asked to do so. Then find the `autoScreenSize true` setting inside section Window inside section Desktop. In Vrui-8.0-002's configuration file, that is in line 153. Then change the value of `autoScreenSize` from `true` to `false`.

With `autoScreenSize` turned off, Vrui will fall back to the screen size configured in `section Screen`, in lines 104-111 in the same configuration file. The default values in there, defining a 20" 4:3 aspect ratio monitor, should work well enough. If your projector has a different aspect ratio, say 16:9, you might want to change the size values to reflect that, otherwise the image will appear squashed. First, change the `width` and `height` settings to appropriate values, and then adjust the origin setting such that the center of the screen is at position `(0.0, 0.0, 0.0)`. For example, if you set `width` to `20.92` and `height` to `11.77`, for a 24" diagonal 16:9 monitor, you would set `origin` to `(-10.46, 0.0, -5.885)`. When you are done with your changes, save the file and quit the text editor.

If you have already adjusted the screen size, but still want to fine-tune the size of displayed texts, you can do so by directly changing the font size in the same configuration file. Towards the top of the file, underneath `section Desktop`, find the `uiFontTextHeight` settings (line 58 for Vrui-8.0-002), and change the value as desired. Text height is defined in the same unit of measurement as every other position or size in the configuration; by default, that is in inches. When done, save the file and quit the text editor.

#### Step 4: Install the Kinect 3D Video Package

To install the Kinect 3D Video Package, run in the same terminal window:

	cd ~/src
	wget http://web.cs.ucdavis.edu/~okreylos/ResDev/Kinect/Kinect-3.10.tar.gz
	tar xfz Kinect-3.10.tar.gz
	cd Kinect-3.10
	make
	sudo make install
	sudo make installudevrules
	ls /usr/local/bin

The sixth command might ask you to enter your password one more time; if so, enter it to continue. Check that the list of names printed by the last command contains `KinectUtil` and `RawKinectViewer`.

#### Step 5: Install the Augmented Reality Sandbox

To install the Augmented Reality Sandbox software itself, run in the same terminal window:

	cd ~/src
	wget http://web.cs.ucdavis.edu/~okreylos/ResDev/SARndbox/SARndbox-2.8.tar.gz
	tar xfz SARndbox-2.8.tar.gz
	cd SARndbox-2.8
	make
	ls ./bin

Check that the list of names printed by the last command contains `CalibrateProjector`, `SARndbox`, and `SARndboxClient`. The AR Sandbox calibration utility and main application are now in `~/src/SARndbox-2.8/bin`.

### System Integration, Configuration, and Calibration

Installation steps 6 to 11 connect the additional system components, i.e., the 3D camera (Kinect or other camera) and the digital projector, physically align them with the sandbox for optimal image quality, and calibrate the camera with respect to the projector so that real and projected features in the sandbox line up precisely.

#### Step 6: Connect and Configure the 3D Camera

Plug in your 3D camera and download intrinsic calibration parameters directly from its firmware. In a terminal window, run:

	sudo /usr/local/bin/KinectUtil getCalib 0

This might ask you for your password again; if so, enter it to continue.

#### Step 6a (Optional): Calculate Per-pixel Depth Correction

First-generation Kinect cameras (Kinect-for-Xbox-360) have a certain amount of non-linear depth distortion. If you point such a camera at a flat surface, the 3D reconstruction will not be flat, but slightly bowl-shaped. This distortion can prevent getting a good alignment between the physical sand surface and the topographic map projection in Step 10.

`RawKinectViewer` has a built-in calibration utility to correct for this distortion. This step is not necessary for operation of the AR Sandbox, but it usually improves alignment quality. Again, this issue only applies to first-generation Kinect cameras (models 1414, 1473, and 1517). It is easiest to perform this calibration step with the Kinect camera removed from the AR Sandbox, so if your Kinect is difficult to remove after assembly, it might be a good idea to do this step before putting it in place.

To perform per-pixel depth correction, place your Kinect camera such that it faces a large flat non-shiny surface, like an empty wall. Then start `RawKinectViewer` as administrator, as it will need to write a calibration file to the system location `/usr/local/etc/Vrui-8.0/Kinect-3.10`. In a terminal window, run:

	sudo /usr/local/bin/RawKinectViewer -compress 0

and enter your password if/when asked.

To begin calibration, move your camera towards the flat surface until black blotches start appearing in the depth image stream on the left side of `RawKinectViewer`'s display window, and then pull back until there are only small and isolated black spots. It is best to have the camera face the flat surface head-on; rotate the camera left/right and up/down until the displayed depth color is uniform between the left, right, top, and bottom edges.

Next, assign a calibration tool to two arbitrary keys. Press the first key, say "A", and move the mouse to highlight **Calibrate Depth Lens** in the tool selection menu that pops up, then release the key. This will close the tool selection menu and open a **Creating Calibrate Depth Lens Tool...** dialog prompting you to press another key for tool function **Calibrate**. Pick a key, say "S", press and release it. This will close the dialog, and finish tool assignment.

To capture a calibration tie point, keep the camera very still and press the "A" key once. This will show a **Capturing average depth frame...** message. Do not touch or move the camera while this message is being displayed. After it disappears a few seconds later, move the camera a little distance further away from the flat surface, and press "A" again to capture the next tie point.

Repeat this process a few times to collect anywhere between five and ten tie points, from distances between about 0.5m and 1.5m. Once done, press the "S" key. This will calculate per-pixel depth correction factors and write them to a calibration file in the `/usr/local/etc/Vrui-8.0/Kinect-3.10` directory. `RawKinectViewer` will print `Writing depth correction file /usr/local/etc/Vrui-8.0/Kinect-3.10/DepthCorrection-<camera serial number>.dat` when it is finished. If any error messages appear at this point, close `RawKinectViewer` and redo the entire process. Otherwise, close `RawKinectViewer`, install the camera in the AR Sandbox, and continue with the next installation step.

#### Step 7: Align the 3D Camera

Align your camera so that its field of view covers the interior of your sandbox. Use `RawKinectViewer` to guide you during alignment. To start it, run in a terminal window:

	cd ~/src/SARndbox-2.8
	RawKinectViewer -compress 0

Ignore the color video stream on the right side of `RawKinectViewer`'s display window and focus on the depth image stream on the left (the AR Sandbox does not use the color video stream). Move and/or rotate the camera until it can see the entire interior of your sandbox.

#### Step 8: Measure Sandbox's Base Plane Equation

Calculate your sandbox's base plane, by following the instructions in the [AR Sandbox calibration video](https://www.youtube.com/watch?v=EW2PtRsQQr0). You can use the already-running instance of `RawKinectViewer` from Step 7.

To recap, you need to bind an **Extract Planes** tool from the root tool menu to some unused mouse button or keyboard key (see the note on **Tool Assignment** in Step 3 for details), then select **Average Frames** from `RawKinectViewer`'s main menu, wait for the depth image display to freeze, and then draw a rectangle on the depth image by pressing and holding the button/key to which you assigned the **Extract Planes** tool. This will print the plane equation best fitting the selected rectangle to the terminal window from which you ran `RawKinectViewer`. After extracting the base plane, you should turn **Average Frames** back off in `RawKinectViewer`'s main menu.

You need to enter the base plane equation (and the 3D sand surface extents in the next step) into the BoxLayout.txt file in `etc/SARndbox-2.8` inside the SARndbox source directory. To edit the file using xed, run in a terminal window:

	cd ~/src/SARndbox-2.8
	xed etc/SARndbox-2.8/BoxLayout.txt &

`RawKinectViewer` prints two plane equations when a depth plane is extracted: the first in depth space, the second in camera space. The AR Sandbox needs the second, camera-space, plane equation. After copying it, the equation has to be reformatted slightly. RawKinectViewer will print:

	Camera-space plane equation: x * <some vector> = <some offset>

where `<some vector>` is a three-component direction vector defining the "up" direction in camera coordinates, typically close to `(0.0, 0.0, 1.0)`, and `<some offset>` is the vertical position of the measured plane underneath the camera, which is in centimeters and negative. `BoxLayout.txt` needs that plane equation in the following format:

	<some vector>, <some offset>

where the direction vector and the offset are separated by a comma.

**Note:** For some reason, the depth plane equation reported by second-generation Kinect cameras (Kinect-for-Xbox-One) may be inverted. Before continuing, check that the fourth component (offset) of the camera-space plane equation is in fact negative. If it is not, flip the signs of all four components of the plane equation in BoxLayout.txt, e.g., `(-0.01, 0.04, -0.9991), 105.3` becomes `(0.01, -0.04, 0.9991), -105.3`.

#### Step 9: Measure Sandbox's 3D Box Corner Positions

Measure the 3D extents of the sand surface. As of version 3.2 of the Kinect package, this can be done inside `RawKinectViewer` as well by following the instructions in the AR Sandbox calibration video, starting at 4:10. Make sure to measure the box corners in the order **lower-left, lower-right, upper-left, upper-right**.

To recap, you need to bind a **Measure 3D Positions** tool from the root tool menu to some unused mouse button or keyboard key (see Step 3 for details), and then click on the corners of your sandbox in the depth image using the button/key to which you assigned the **Measure 3D Positions** tool. This will print a sequence of 3D positions to the terminal window from which you ran `RawKinectViewer`.

After you have copied the box corner positions into the text editor as described in the video, save the file (via the "File" menu or by pressing Ctrl-s), and quit from the text editor (via the "File" menu or by pressing Ctrl-q or by closing the window).

After Steps 8 and 9 have been completed, the contents of `BoxLayout.txt` should look like the following, with different numbers depending on your installation:

	(-0.0076185, 0.0271708, 0.999602), -98.0000
	(  -48.6846899089,   -36.4482382583,   -94.8705084084)
	(   48.3653058763,   -34.3990483954,   -89.3884158982)
	(   -50.674914634,    35.8072086558,   -97.4082571497)
	(   48.7936140481,    36.4780970044,     -91.74159795)

Ensure that this text starts in the first column of the first line, and that the file contains no other text at all.

#### Step 10: Align the Projector

Align your projector such that its image fills the interior of your sandbox. You can use the calibration grid drawn by Vrui's `XBackground` utility as a guide. In a terminal window, run:

	XBackground

After the window showing the calibration grid appears, press F11 to toggle it into full-screen mode. Ensure that the window really covers the entire screen, i.e., that there are no title bar, desktop panel, or other decorations left. Press the "Esc" key to close XBackground's window when you're done.

Ensure to disable all digital image distortion features of your projector before alignment, and only use optical features, i.e., optical focus adjustment and optical zoom. Specifically, disable any kind of digital keystone correction, and check that the projector maps the incoming video signal 1:1 to its display pixels. The best way to check for 1:1 matching is to look at the central horizontal bar in `XBackground`'s test image. It should be a clear pattern of alternating white and black one-pixel-wide vertical lines with no smearing or stair steps.

Slight overprojection outside of the sandbox, and any remaining keystone distortion of the projected image, will be taken care of by the following projector calibration step.

#### Step 11: Projector/Camera Calibration

Calibrate the Kinect camera and the projector with respect to each other by running the `CalibrateProjector` utility:

	cd ~/src/SARndbox-2.8
	./bin/CalibrateProjector -s <width> <height>

where `<width>` and `<height>` are the width and height of your projector's image in pixels, respectively. For example, for an XGA projector like the recommended BenQ model, the command would be:

	./bin/CalibrateProjector -s 1024 768

Ensure that the given image size exactly matches the size of the projector's input video signal.

**Very important:** Set `CalibrateProjector`'s window to full-screen mode by pressing F11 before proceeding. Then follow the instructions in the AR Sandbox calibration video, starting at 10:10.

To recap, you need to bind a "Capture" tool from the root tool menu to two unused mouse buttons or keyboard keys to capture tie points. For example, to bind a "Capture" tool to the "1" and "2" keys, first press and hold "1." This will pop up the tool selection menu. While holding "1," move the mouse to highlight the "Capture" menu entry. Then let go of the "1" key. This will close the tool selection menu, and open a "Creating Capture Tool..." dialog prompting you to press another key for tool function "Capture Background." Now press and release the "2" key. This will close the dialog, and finish tool assignment. Afterwards, press and release "1" to capture a tie point and advance calibration, and press and release "2" to re-capture the background sand surface after you have made any changes to it, such as digging a hole to capture a low tie point.

**Please read the preceding paragraph carefully, and follow its instructions precisely.** If you get a red screen in response to pressing a key/button during calibration, you are pressing the wrong key/button. The first assigned tool key ("1" in the instructions above) will capture a tie point and advance calibration, i.e., move the calibration crosshairs. The second assigned tool key ("2" in the instructions above) will re-capture the background, indicated by turning the display red for a few seconds, and not advance calibration.

The calibration program expects a disk of diameter 120mm (4.7"), which is a standard CD, CD-ROM, or DVD. The easiest way to create a calibration disk is to glue a sheet of white paper to the data side of a CD/DVD/..., carefully cut around the edge of the CD, and draw a cross onto the paper that intersects exactly in the center of the CD's hole.

**Note:** Do not worry if the projected calibration image (yellow background, yellow or green disk) does not line up with the physical sandbox. This calibration step will make the image line up after it's done.

#### Step 12: Running the Augmented Reality Sandbox

At this point, the Augmented Reality Sandbox is configured, calibrated, and ready to run. To run the main AR Sandbox application, run in a terminal window:

	cd ~/src/SARndbox-2.8
	./bin/SARndbox -uhm -fpv

The `-uhm` argument tells the `SARndbox` application to apply an elevation color map to the sand surface, and the `-fpv` argument tells it to use the projector/camera calibration collected in Step 11.

**Very important:** Set `SARndbox`'s window to full-screen mode by pressing F11, or the projector/camera calibration will not line up.

### Post-Installation Setup and Fine-Tuning

The instructions above get the AR Sandbox software installed and running, but there are still a lot of (optional) improvements to be made. The following steps automate full-screen display and tool bindings, configure multiple displays, etc.

#### Step 13: Create Per-application Configuration File Directory

The Vrui VR toolkit supports per-application configuration files, to set display or interaction options, or pre-bind regularly-used tools. These configuration files are stored in a directory `~/.config/Vrui-<version>/Applications`, and are given the same name as the application to which they belong, plus a .cfg extension. Create the configuration file directory by running in a terminal window:

	mkdir -p ~/.config/Vrui-8.0/Applications

#### Step 14: Create Configuration File for CalibrateProjector

Create a configuration file for the `CalibrateProjector` application using the xed text editor:

	xed ~/.config/Vrui-8.0/Applications/CalibrateProjector.cfg

Into that new file, paste exactly the following text:

	section Vrui
		section Desktop
			section Window
				# Force the application's window to full-screen mode:
				windowFullscreen true
			endsection

			section Tools
				section DefaultTools
					# Bind a tie point capture tool to the "1" and "2" keys:
					section CalibrationTool
						toolClass CaptureTool
						bindings ((Mouse, 1, 2))
					endsection
				endsection
			endsection
		endsection
	endsection

then save the new file and exit from the text editor.

If you now start `CalibrateProjector`, its window will cover the entire screen, with no title bars or panels remaining. If you press the "1" key, the program will capture a calibration tie point, and if you press the "2" key, it will re-capture the background, indicated by the screen turning red for two seconds.

You can replace the "1" and "2" key names in the "bindings" tag with any other keys you like.

#### Step 15: Create Configuration File for SARndbox

Create a configuration file for the SARndbox application using the xed text editor:

	xed ~/.config/Vrui-8.0/Applications/SARndbox.cfg

Into that new file, paste exactly the following text:

	section Vrui
		section Desktop
			# Disable the screen saver:
			inhibitScreenSaver true

			section MouseAdapter
				# Hide the mouse cursor after 5 seconds of inactivity:
				mouseIdleTimeout 5.0
			endsection

			section Window
				# Force the application's window to full-screen mode:
				windowFullscreen true
			endsection

			section Tools
				section DefaultTools
					# Bind a global rain/dry tool to the "1" and "2" keys:
					section WaterTool
						toolClass GlobalWaterTool
						bindings ((Mouse, 1, 2))
					endsection
				endsection
			endsection
		endsection
	endsection

then save the new file and exit from the text editor.

As in Step 14, this will force `SARndbox` to start in full-screen mode, ensuring that the calibration created using `CalibrateProjector` exactly matches the one used in the actual AR Sandbox. In addition, the `inhibitScreenSaver` setting will prevent the screen from blanking if no keys are pressed, and the `mouseIdleTimeout 5.0` setting will hide the mouse cursor after five seconds of inactivity. To get the cursor back (for menu interactions etc.), simply move the mouse. Finally, the `WaterTool` section binds a tool to add or remove water to/from the entire AR Sandbox, by pressing "1" to rain, and "2" to drain. As previously, set the binding to whatever keys you prefer.

#### Step 16: Create a Desktop Icon to Launch the AR Sandbox

Starting the AR Sandbox may require typing a lengthy command line into a terminal window, which might get tedious. To simplify things, you can create a desktop icon to launch the AR Sandbox by simply double-clicking. This is best done in two steps: first, create a shell script to launch the SARndbox application with all command line arguments; second, link that shell script to a desktop icon.

To create a shell script, run in a terminal window:

	xed ~/src/SARndbox-2.8/RunSARndbox.sh

This will open an editor window with an empty file. Paste exactly the following contents into that file:

	#!/bin/bash

	# Enter SARndbox directory:
	cd ~/src/SARndbox-2.8

	# Run SARndbox with proper command line arguments:
	./bin/SARndbox -uhm -fpv

Add any command line arguments you normally use to the last line. Then save the file, exit the text editor, and run in the same terminal:

	chmod a+x ~/src/SARndbox-2.8/RunSARndbox.sh

This will tell the operating system that the shell script can be executed.

To create a desktop icon, run in a terminal window:

	xed ~/Desktop/RunSARndbox.desktop

Into the empty text file, paste the following contents:

	#!/usr/bin/env xdg-open
	[Desktop Entry]
	Version=1.0
	Type=Application
	Terminal=false
	Icon=mate-panel-launcher
	Icon[en_US]=
	Name[en_US]=
	Exec=/home/<username>/src/SARndbox-2.8/RunSARndbox.sh
	Comment[en_US]=
	Name=Start the AR Sandbox
	Comment=

Replace `<username>` with your actual user name, to locate the shell script created in the previous step. Then save the file, exit the text editor, and make the file executable:

	chmod a+x ~/Desktop/RunSARndbox.desktop

Now, double-clicking the icon will start the AR Sandbox with all command line arguments.

#### Step 17: Launch the AR Sandbox on Login / Boot

To run the AR Sandbox as a computational appliance that starts automatically when the PC is powered on, you need to create an auto-login account during Linux installation in Step 1, and create a shell script to run the `SARndbox` application as described in Step 16. Then you add that shell script to your user account's start-up list. Select the **Startup Applications** applet in the MATE Control Center, and click the "Add" button next to the list of **Additional startup programs.** Into the dialog box that opens, enter a name (such as "Start AR Sandbox"), and into the Command field enter the full name of the shell script, i.e., `/home/<username>/src/SARndbox-2.8/RunSARndbox.sh`, where you replace `<username>` with your actual user name.

The next time you log into your account, or the next time the PC powers up (if auto-login is enabled), the AR Sandbox will start automatically. To exit from the application, for example to run a new projector calibration or do other maintenance tasks, simply press the "Esc" key.

#### Step 18: Use Multiple Screens

If you are running an AR Sandbox from a laptop, or if your desktop computer has a main monitor in addition to the sandbox projector, you can tell `CalibrateProjector` and `SARndbox` to use the projector, and leave the main monitor for other purposes, such as starting scripts or displaying a secondary view of the virtual topographic map from arbitrary points of view. First, determine to which video output port the sandbox projector is connected. In a terminal window, run:

	xrandr | grep connected

The `xrandr` utility will print a list of all video output ports that exist on the computer, and information about the monitors/projectors connected to those ports. An `xrandr` report might look like this:

	DVI-I-0 disconnected primary (normal left inverted right x axis y axis)
	DVI-I-1 connected 2560x1600+0+0 (normal left inverted right x axis y axis) 641mm x 401mm
	HDMI-0 disconnected (normal left inverted right x axis y axis)
	DP-0 disconnected (normal left inverted right x axis y axis)
	DVI-D-0 connected 1600x1200+2560+0 (normal left inverted right x axis y axis) 367mm x 275mm
	DP-1 disconnected (normal left inverted right x axis y axis)

This report shows two connected monitors: One with 2560x1600 pixels connected to port `DVI-I-1`, and a secondary with 1600x1200 pixels and origin +2560+0, i.e., positioned to the right of the main monitor, connected to port `DVI-D-0`.

From your `xrandr` report, determine the port name connected to your sandbox projector, for example by looking for the projector's pixel size, e.g., 1024x768. If your projector is connected via an HDMI cable, the port name will usually be something like HDMI-0. Then direct `CalibrateProjector` and `SARndbox` to open their display windows on that video output port by editing their respective configuration files:

	xed ~/.config/Vrui-8.0/Applications/CalibrateProjector.cfg

and afterwards

	xed ~/.config/Vrui-8.0/Applications/SARndbox.cfg

In both files, insert the following `outputName` setting into the existing `Window` section:

        section Window
            ...
            # Open the window on a specific video output port:
            outputName <port name>
            ...
        endsection

where `<port name>` is replaced with the name of your projector's actual video output port from your `xrandr` report, e.g., `HDMI-0`.

Afterwards, starting `CalibrateProjector` or `SARndbox` will send their display windows to the sandbox projector and full-screen them there. Check that there are no remaining window frames or panels etc.

The `xrandr` utility can also be used to turn connected monitors/projectors on or off. For example, let's say that your main monitor is connected to port `DVI-I-1`, and your projector to port `HDMI-0`. Then you can turn on the projector, and place it to the right of your main monitor, by running:

	xrandr --output DVI-I-1 --auto --primary --output HDMI-0 --auto --right-of DVI-I-1

To turn the projector off again, run:

	xrandr --output DVI-I-1 --auto --primary --output HDMI-0 --off

#### Step 19: Show a Secondary View of the AR Sandbox

If you have multiple displays connected to the PC running your AR Sandbox, and have done the multi-screen setup in Step 18, then you can show a second display window that does not just replicate the projected view shown in the sandbox itself, but that can show the captured 3D topography from arbitrary points of view, in full 3D, as explained in this video.

To create a secondary view, you first need to edit the `SARndbox` application's configuration file and instruct it to open a second window on a different display. Run in a terminal window:

	xed ~/.config/Vrui-8.0/Applications/SARndbox.cfg

At the beginning of the `Desktop` section, insert the following setting:

	section Vrui
		section Desktop
			# Open a second window:
			windowNames += (Window2)
			...
		endsection
	endsection

Then, after the existing `Window` section, insert a new section `Window2` with the following settings:

	section Vrui
		section Desktop
			...
			section Window
				...
			endsection

			section Window2
				viewerName Viewer
				screenName Screen
				windowType Mono

				# Open the window on a specific video output port:
				outputName <port name>

				# Open the window to full-screen mode:
				windowSize (800, 600)
				windowFullscreen true
			endsection
			...
		endsection
	endsection

where `<port name>` is replaced with the name of the actual video output port to which your non-sandbox display is connected, as gathered from your xrandr report in Step 18.

After the above changes, `SARndbox` will open a second window on the secondary display, but it will still show the same fixed projector view as the main projector, possibly squished due to a different aspect ratio on the secondary display. To allow an independent view on the secondary display, you have to modify `SARndbox`'s command line. The best way to do this is to edit the `RunSARndbox.sh` shell script you created in Step 16. In a terminal window, run:

	xed ~/src/SARndbox-2.8/RunSARndbox.sh

The simple command line entered in Step 16 only has two arguments:

	./bin/SARndbox -uhm -fpv

These instruct `SARndbox`, in order, to apply the default elevation color map, and to lock the display to the projector calibration matrix captured in Step 11. By default, `SARndbox` applies display options from the command line to all windows it opens, but this can be overridden by using a -wi <window index> argument, where `<window index>` is the zero-based index of a window. After seeing that argument, `SARndbox` will apply all following display options to all windows with the same or a larger index, and also reset the `-fpv` option. Thus, append the following to `SARndbox`'s command line:

	./bin/SARndbox -uhm -fpv -wi 1 -rws

The (optional) `-rws` argument will draw water as a three-dimensional surface instead of applying it to the topography as a texture, as is the default setting. This will create a better representation of water flow when seen from oblique angles.

After these changes, the AR Sandbox will show a navigable three-dimensional view of the 3D topography model, including water drawn as a real 3D surface, on the secondary display. The secondary display can be rotated, translated, and scaled as in many other 3D graphics software, using a combination of mouse buttons and keyboard keys. Most importantly, pressing the left mouse button will rotate the view around the center of the window, and pressing the "Z" key while moving the mouse will translate the view in the window's plane. Pressing the left mouse button and "Z" key together, while moving the mouse up and down, will scale the view (zoom in or out), as will rolling the mouse wheel. Finally, pressing the left mouse button, the "Z" key, and the left Shift key, while moving the mouse up or down, will "dolly" the view in or out of the window's plane, as will rolling the mouse wheel while pressing the left Shift key.

On start-up, `SARndbox` will show a default view of the 3D topography in its secondary window. If you would like to show a different view on start-up, you can use the navigation techniques described in the previous paragraph to create the perfect view that you want, and then save it. To do this, press and hold the right mouse button to show the main application menu. Move the mouse to the last entry, "Vrui System", and from there to the "View" sub-menu, then to the "Save View..." entry, and there release the right mouse button. This will open a file selection dialog where you can specify a file name and location for the view file about to be written. Leave the default file name (`SavedViewpoint0001.view`) and default location (the AR Sandbox's project directory) alone, and press the "OK" button. This will close the file selection dialog and save the current view to the selected file.

After exiting from SARndbox, change the command line in the `RunSARndbox.sh` shell script again, this time appending a command to load the view file you just saved. Run in a terminal:

	xed ~/src/SARndbox-2.8/RunSARndbox.sh

and change the command line to:

	./bin/SARndbox -uhm -fpv -wi 1 -rws -loadView SavedViewpoint0001.view

If you changed the name of the view file before saving it (or renamed it later), use the correct name here. Afterwards, the AR Sandbox will start up with your preferred view displayed in its secondary window.

