# MCSchematicToPng
Converts MineCraft schematic files to PNG. Can work either in batch mode, processing local files, or as a network daemon processing requests from other computers.

# Usage - network daemon
To run as a network daemon, pass the `-jsonnet <portnumber>` commandline parameter. The program will start listening for incoming connections on the specified port. Each connection will allow remote computers to make conversions.

## JSON Protocol
The protocol is simple, each side streams JSON objects, delimited by a 0x17 character (ETB). Once the delimiter is received, the JSON is parsed, the command specified in it executed and a reply sent. The server starts the communication by sending the version information JSON: `{"MCSchematicToPng": 2}`. The client can send JSON commands, each command must have at least a `Cmd` member, specifying the action to perform. An action `RenderSchematic` is used to render an embedded .schematic data into a PNG image. The program reads further parameters from the JSON message (for a list, see below), and also remembers any `CmdID` value in the command. Finally, it replies with a JSON that has its `Status` member set to `ok` or `error`, and the `CmdID` member repeated from the incoming command. If successful, the reply also contains a `PngData` member, containing the Base64-ed PNG image data.

Parameters read for the `RenderSchematic` command:

Parameter | Default value | Notes
----------|---------------|------
BlockData | (compulsory) | Base64-ed .schematic data to be rendered
StartX | 0 | Optional crop on the X axis from the minus-side
EndX | (width) | Optional crop on the X axis from the plus-side
StartY | 0 | Optional crop on the Y axis from the minus side
EndY | (height) | Optional crop on the Y axis from the plus-side
StartZ | 0 | Optional crop on the Z axis from the minus-side
EndZ | (length) | Optional crop on the Z axis from the plus-side
NumCWRotations | 0 | Number of CW rotations to apply before rendering
HorzSize | 4 | Horizontal size of the drawn cubes' faces
VertSize | 5 | Vertical size of the drawn cubes' faces
Markers | none | Vector markers to draw in the image. Array of marker objects (see below)

Each marker has these parameters:

Parameter | Default value | Notes
----------|---------------|-------
X | (compulsory) | The X coord of the marker
Y | (compulsory) | The Y coord of the marker
Z | (compulsory) | The Z coord of the marker
Shape | (compulsory) | The name of the shape of the marker
Color | 000000 | The color for the shape, expressed as a hexadecimal RRGGBB value

Another action to perform is the `SetName` command, which simply sets the "name" of the connection, used when logging things. The `Name` member is used as the connection name. No confirmation of this command is given.

## Typical protocol exchange
Client connects, server sends the version message:
```json
{"MCSchematicToPng": 2}
```
Client sends a request to name the connection:
```json
{"Cmd": "SetName", "Name": "GalExport"}
```
Client sends a request to render a schematic:
```json
{
  "Cmd": "RenderSchematic",
	"CmdID": {"AreaID": 1, "NumRotations": 2, "LastEditTick": 345},
	"BlockData": "<base64-ed data>",
	"Markers":
	[
		{ "X": 0, "Y": 0, "Z": 0, "Shape": "ArrowXM", "Color": "ff0000" },
	],
}
```
Server renders the schematic and returns the PNG image:
```json
{
	"Status": "ok",
	"CmdID": {"AreaID": 1, "NumRotations": 2, "LastEditTick": 345},
	"PngData": "<base64-ed image data>",
}
```

# Usage - local files
The program uses listfiles - instead of specifying the files to be converted and the conversion params on the commandline, you write them into a text file and give that file to the convertor. 

```
MCSchematicToPng -threads 8 listfile.txt
```
converts files listed in listfile.txt  using 8 threads

```
MCSchematicToPng --
```
reads stdin as the listfile and converts using 4 threads (default)

Listfile is a simple text file that lists the .schematic files to be converted, and the properties for each export. If a line starts with non-whitespace, it is considered a filename to convert. If a line starts with a whitespace (tab, space etc) it is considered a property for the last file. Properties can specify different output filename, cropping, size of the isometric tile and rotation. Additional (vector-based) markers can be output at any valid block position
Example:
```
file1.schematic
  outfile: large file1.png
  horzsize: 16
  vertsize: 20
  marker: 0, 0, 0, BottomArrowXM
  marker: 5, 8, 3, Cube

file1.schematic
  outfile: medium file1.png
  horzsize: 8
  vertsize: 10

file1.schematic
  outfile: small file1.png

file2.schematic

file3.schematic
  outfile: slice1.png
  startx: 0
  endx: 0

file3.schematic
  outfile: slice2.png
  startx: 1
  endx: 1

file3.schematic
  outfile: slice3.png
  startx: 2
  endx: 2
```
This converts file1.schematic into three PNG files according to the properties specified, file2.schematic into a PNG file with the default properties, and three slices of file3.schematic into three separate PNG files. The `large file1.png` additionally gets two markers.
