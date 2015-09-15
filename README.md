# MCSchematicToPng
Converts MineCraft schematic files to PNG

# Usage
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
