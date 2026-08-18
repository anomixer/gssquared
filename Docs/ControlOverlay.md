# Control Overlay

Since the graphics system of GSSquared operates like a video game, comprised of 60 frames per second and each frame being drawn from scratch, there are very interesting video game like possibilities for a control overlay.

Instead of trying to rely on traditional GUI menuing system, we'll build our own. It will be streamlined, simple.

Take your Apple II screen, whatever is going on on it. Hit a key (Fsomething) to open the control panel.

First, we will draw a white rectangle on the screen with a black border. It will be opaque, perhaps with 50% opactiy. Opaque enough to still have a sense of what is going on on the Apple II display remember, in video game world, we are layering things on top of each other in the view, from back to front. And, since we're drawing each frame this way, *everything on the apple ii screen will continue happily behind our semi-opaque control panel*! That will be uber-cool.

We can draw things with 0 opacity. (e.g., our control panel widgets). Then the background will not be visible behind them. We can use this to great effect.

But we won't just draw it and be done with it. We will animate it sliding in from the left side of the screen. The rectangle (and all the things we draw over it) will be in a texture, and when we render the texture to the window.

So, the first principle - in normal operation, you will not need separate windows to manage the virtual Apple. This means the controls will be readily available all the time, look and work exactly the same on any platform (mac, linux, windows). And, scale with the window size including up to full screen.

I AM contemplating a separate window for the *debugger*. But that is not something most users will use.

In the control panel, will be some buttons:
* power on / off.
* Reset.
* Save state.
* Load state.
* Screen capture. (as PNG, and as Apple II screen memory dump, e.g. hi-res image).
* Text copy/paste. 
* CPU Speed control: normal, GS, Apple IIc+, Ludicrous
* Display: color, mono green, mono amber.
* Enable / Disable sound recording. (i.e. storing and saving the event timestamps. This is for debugging.)
* Joystick emulation selection.

Now all or almost all of these will have keyboard shortcuts too. But allow doing them from here.

A really cool effect would be: when a disk drive is running, have it displayed on the screen too (not in the control panel). When the drive stops, have it fade out. Maybe do this with a smaller version of the disk drive image so it takes up less screen real estate. Have this be optional. We can also have the disk drive sounds playing. But the disk II image coming up with the drive light on, that's spiffaroonie. And, we have border area to put the images in, so it needn't cover up much of the Apple II display.

When the mouse hovers over a controllable item for a bit, display a tooltip.

These button controls should have obvious visual feedback to indicate their state.

Then, there will be an area that displays the current machine Profile. The Profile is:
* Apple II model (II, II+, //e, //c, IIgs)
* Slot card configuration.
* Memory configuration (i.e., in a IIgs, the memory size. Other models have memory cards as selectable plugins).
* Disk Configuration - by default what disk image is mounted in which drive.

And, the profile might have been loaded from a saved Profile file. So we also need: Save Profile, Load Profile controls. And, display of the current Profile file name. This lets us manage multiple profiles easily. Different virtual IIs will be needed to run all the different software. Various stuff from II+ era won't run on GS, etc.

Based on the slot configuration, we will further display the drives attached. We'll just assume every card gets two drives. (E.g, a Disk II gets two drives. A SmartPort gets two drives. A curiosity, does a SmartPort UniDisk 5.25 work the same way as a Disk II? was there some kind of passthrough for this? I don't have SmartPort really yet, but, you get the idea).

Show images of the drives.

Click on a drive to select and mount a disk image onto it. There will be some clickable widget on the drive to eject a disk. On the Disk II, the door opener. On a Unidisk 3.5, its eject button.

Further, display Drive 1 and Drive 2 labels in the upper left corner of Disk II. And in the appropriate place on the UniDisk. Those had little drive number stickers too.

The ProFile was a pretty big device, more than twice the width of a Disk II. I can have a smaller image for those.

Hover mouse over the drive to see the current disk image name. Or, display them somewhere on the drive image? In cases I've seen
that is hard to read.

Only have the control panel take up as much room on the screen as is needed. We want the user to be able to see what's going on underneath, remember. And the control panel should not interfere with the operation of the virtual II!

Things that cannot be changed while the machine is running, should be grayed out in some way. For example, the slot configurations. I could code the emulator to do that without crashing, but, Apple II software is not going to handle it without a reboot. Ah, hot hardware insertion will just not be an Apple II thing, ha ha.

When we close the control panel, either with a close button or with the same keypress that opened it, we will slide it back off the screen just like we slid it in.

## Event Loop function

The event loop will be modified as follows.

If the control panel is visible:

1. we need to include the control panel drawing function, and execute those routines before we do the Render. Unless we can do two renders one after the other. (That might be a thing).
1. if visible and stable - i.e. not opening or closing - then we need to check for mouse input and perform all the interactive stuff in the control panel.

## Architecture

A question of architecture: should each control panel widget be defined and implemented:

* in a separate module
* in the same module with the slot card it represents

I am thinking the former. There are a couple reasons for this.

First, I acknowledge that this will mean these display routines are going to have to have access to the internals of these virtual devices. However, I guess you can think of them as View components. So, the control panel will be tightly bound to the virtual device implementation.

But, having all this display stuff in separate module, well we're doing that now, by having all the slot init etc in the gs2 module, and the profile definition in their own modules, etc.

## Additional Overlay features

We could have other types of overlays. For example, the effective CPU speed. I think people probably don't care most of the time. But, toggle it on/off - ooh, we have a border area on the display. So we have room to put that kind of thing.

We could have a indicator if the machine is in an infinite loop. We already detect it. 

## Things I thought about when writing this

I think we'll want a RAMfast or Apple II High Speed SCSI Card virtual card to allow people to easily attach many volumes as partitions. Do we want to support a single file that has partitions inside it? Then we'd need a partition manager. (or could use the RAMFast firmware? Maybe that's why Antoine was asking about this).

I am hoping SDL3 will take care of the operating system file picker stuff in a separate thread or otherwise in a way that does not interfere with the operation of the event loop. This should be something to test really soon.

When we detect infinite loop, don't crash out of the emulator.  Infinite loop is defined as "an instruction that jumps precisely to itself". Anything else is waiting for an event, like an interrupt.

Instead of choosing slot as a list, we could drag and drop a slot card image onto a slot.? could be fun.

# Overlay Drawing


I see talk of pipelines. I think these are basically lists (arrays) of drawing commands to execute. Done as a data structure, so that you don't have to draw as a sequences of lines of code, but rather, executing a sort of virtual program.

So, let's think about the primitives we need.

1. Slot label
1. Drive Icon


So we want the Drive Icon to be in a tile that's a little bigger than the icon itself. And that tile background will be drawn in different colors depending on the cursor hover state. (Hover as determined by mouse position - should make it so this works during drag and drop).

Second, Disk Running vs Disk Stopped. There will be some state. First, that state needs to be read from the diskII routines. The DiskII should have a method to query the state, and update it in our Drive Icon. I could draw this by just modifying the light. But it's just as easy to draw the whole thing. The entire thing is drawn anyway.

Third, the drive number. Label Drive 1 and Drive 2 accordingly.

Then we can draw the drive. So there are two steps:
* draw background
* draw drive

There is additional state: the X and Y coordinates of the tile. The drive itself can be sized, and centered in the tile.

## Assets

An Asset is a texture, and a rectangle range. A simple asset is: one image, one rectangle.

But we can have an AssetAtlas that is: one image, multiple rectangles.

The abstraction here is that a simple asset is also an AssetAtlas, with only one rectangle.

The rects have IDs.

So an AssetAtlas has:
* Texture
* List of AtlasElements

An AtlasElement is:
* ID
* X, Y
* Width, Height

An Atlas will have these methods:
* get_rect(id) - return the rectangle for the AtlasElement with the given ID.

* draw(id, x, y) - render the AtlasElement out of the texture to the current display at coordinates x,y

So what is primary here? AssetAtlas. Then Asset_t refers to subsets of an AssetAtlas.


## Containers

We have a Container object. The Container holds:
* List of Tiles
* X, Y and W, H
* Background Color
* Layout direction (left to right / right to left; top to bottom / bottom to top).
* Padding between tiles.
* Visible - boolean - whether container is visible or not.
* Active - boolean - whether a container and the tiles inside it are active (should allow interaction) or not.

The purposes of a Container are:
* grouping similar types of tiles, in a grid pattern
* reducing the number of tiles that need to be checked for activity. Two levels: first the Container bounds, then the tile bounds.
* render a variety of complex things with a single call.

The Container will then lay out the elements inside it on a grid according to the layout direction. Get the largest W/H of any tile inside it. That controls the grid spacing. Then position the X,Y coords of each tile. Leaving the width and height separately. (Or, set by the child class).

Container data structures will be created/modified on:
* App startup
* when a profile is loaded
* a profile is edited
* A disk image is mounted or unmounted.

You will want to use Tiles of roughly similar size and shape inside a given container. 

Tiles in a Container are drawn in the order they were added to the container.

Containers are drawn in the order they were added to the Container list.

There will be starting out several containers. 
* slots
* Drives
* control buttons

When there are changes to:
* visibility of a Tile,
* what Tiles are in a Container

we need to re-run the Container layout.

### Container Dimensions and Coordinates

If we aren't careful, and we do a render with src and dst rects that are not the same size, we'll get a scaled image. This causes problems when we are checking for hover and click. So we need to make sure we are always rendering control panel elements using absolute, unscaled window coordinates. 

So we define our window as 560 * SCALE and 192 * SCALE. Scale may vary as we make the window larger or smaller. So we can't rely on the original size. We need to:
* read window size
* scale mouse coordinates to window coordinates based on difference from original size.
* Make sure containers etc are defined in terms of the "original size" coordinate system before scaling. 
(Scaling is just to get from Apple II pixels, to window pixels, and, it's irrelevant to our drawing of the control panel elements).

So the Window is 1120 + borders, by 768 plus borders, native pixels. That's our possible canvas for the Control Panel texture. Then we will ask this to draw into the window scaled by the current window scale. (Which is calculated whenever the window is resized.)

We want the CP texture to be 1120x768 plus borders also, so that its 1:1 with window, and there is -no coordinate offset-. This gets around the mouse coordinate offset problem.

We probably don't need to update the control panel with a very high frame rate. Right now is 60fps; can probably get by with 15-20fps, saving CPU.

Do measurements on how long we're spending doing the event processing, and display update, on the control panel.

## Tile

A base Tile object is
* Border color
* Border width
* X, Y
* Width, Height
* Background Color
* Hover State - Hovering, Not Hovering
* Hover Color
* Active / Inactive. Inactive = drawn grayed out and do not allow interaction.
* Visible / Invisible. Keep place in list, but, treat as if it wasn't there for purposes of layout.
* Click Callback

If a Tiles has a border, we need to make the tile that many pixels bigger than requested, and, create a dst FRect for rendering the content that is the requested size that is where the content goes. So the content is always what is requested, and it may be offset by the border width. Also, when drawing image, we may want to be able to control the opacity.


### Drive Tile

There is a Drive Tile Object. It is:
* Drive Type
* Drive State
* Drive number (1 or 2)
* Image Name (file name of disk image mounted on this drive)
* Write Protected

Drive Type is:
* DiskII
* UniDisk 3.5
* Profile
* SCSI Disk

Drive State is:
* Disk Running
* Disk Stopped


What if the Hover image was of a floppy disk with a label that is the image name? Then click on that to eject the disk?

### Buttons

Buttons are a derived class of Tile. They are simple actuators:

* Button Text
* Button Image (one or the other)
* Button Background Color
* Button State (active, inactive)
* Button Hover Color
* Button Group ID

The Button Group ID is like for radio buttons. When a button with a given ID is made active, all other buttons with the same ID are made inactive.

The Callback can be done as a function pointer; or, we can just define lots of different subclasses of Button and callback() is a method that's overridden in each subclass. tomato, tomahto?

### Slot Tile

* Slot number
* Slot Type (standard slot, Apple IIe memory slot, Apple IIgs memory slot, etc.)
* Slot text (text descriptor of what's in the slot)

### Image Tile

Just display an image.

### Text Tile

A very simple tile that simply displays some text.

* Pointer to string
* Font
* Font Color

## Container Sub-clases

Do I really need subclasses? Or just init different instances of Container? Try it as the latter first.

### Drives Container
### Slots Container
### Control Buttons Container
### Computer Type Display Container

### Modal Selection Container

Since we draw the Containers one at a time, and, we don't check for any overlap, a container later in the list will cover up a container earlier in the list. We can have the Modal container Visible flag change.

Thus, we need to check events BACKWARDS in the Containers list. Draw bottom up - check top down.

## Event handling

Each entry to our event loop handler, (each 1/60th second), check for:
* Mouse hover: is mouse in a container? If so, which tile is it in? Mark that tile as hover, and all others as not hover.
* Mouse click: is mouse in container? If so, find if it's in a tile. If so, call the click callback for that tile.

# Assets

Instead of having many different files we load, have a single file for display assets. This is called a "sprite sheet" or "texture atlas". 

This uses a single texture that is preloaded into GPU memory. Then we have an array of image coordinates that map to various parts of the texture. Instead of creating lots of separate image handles, we're just tracking rects. And then drawing them is straightforward: blit a rectangle from the texture atlas to the display.

For Disk Drives, we'll have for example variations of a Disk II:

Drive light: on and off (x 2 versions)
Drive sticker: 1 / 2 (x 2 versions)
Drive door: open (umounted) / closed (mounted) (2 versions)

That's four versions.
So no multiple drawing calls. We just pick the right image and blit that.
Same for UniDisk 3.5, and hard drive images.

This will also be handy for all of the various buttons we want.

Keep track of last state, and current state, and only redraw onto the cpTexture if the state has changed.
So then we could have these rects:
* drive light on and off
* drive sticker 1 and 2
* Disk II with drive open/closed.
* Diskette with blank label

8 images, instead of 8. We draw like this:

* select Disk II image
* Select sticker 1 / 2
* select drive light on / off
* if hovering, draw a diskette with a blank label.
* draw the filename on the diskette image

Again, only redraw if the state has changed.
