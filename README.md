# CalculaThreeDS
A scientific calculator homebrew written in C++17 for the 3DS

![Interace](/screenshot.png?raw=true)

## Interface
The interface is split between 4 parts:
- Memory on the top screen, up to 12 calculations can be remembered and copied back to the working board
- Input area on the bottom screen, where what you type will appear, using a custom pretty-printing system (freely sized fractions, exponents, and square roots)
- Keyboard panel selector on a black background
- Touch keyboard with all the inputs you will ever need!

## Controls
Intuitive touch keyboard with 3 panels:
- basic input, for operations and digits
- functions (don't forget to put parentheses after!)
- user variables (10 of them) and user functions (3, TODO) with an assignment (arrow) sign that can appear anywhere in a calculation

The D-PAD and touchscreen let you move the cursor after you typed things out  
The B button or delete key will remove content  
And finally, the A button or equals key will launch the calculation  

## Building

Install the devkitPro 3DS toolchain with libctru, Citro3D, Citro2D,
`makerom`, and `bannertool`, then run one of these targets:

```sh
make          # out/CalculaThreeDS.3dsx and .smdh
make cia      # also builds out/CalculaThreeDS.cia
make package  # builds the complete release package
```

The `.3dsx` is for the Homebrew Launcher. The `.cia` is an installable package
for custom-firmware systems and uses title ID `000400000CA1C000`.

## Credits
LiquidFenrir for writing the application  
devkitPro for devkitARM and libctru/libcitro3d/libcitro2d which allowed me to make this run  
Wikipedia article on the Shunting-Yard algorithm/Reverse Polish Notation for interpreting the calculations
