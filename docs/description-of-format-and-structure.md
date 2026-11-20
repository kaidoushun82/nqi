# What does NQI aim?

NQI as the full name suggests, aims to achieve a clean-room level codec with similar performance or better than other image codecs like Quram's Qmage.

# How are images in the NQI container stored?

An NQI image consists of a basic header, along with "containers" or metadata chunks for which information of the image or frames can be stored.

## Header

| Item                 | Type              | Description                       |
| -------------------- | ----------------- | --------------------------------- |
| Header (NQII/NQIA)   | 4-bytes u8/u32 le | Identifies the image format. The parser checks the image type according to header, along with additional checks |
| Major version (1)    | 2-bytes u8/u16 le | Identifies the spec version of encoded image |
| Minor version (0)    | 2-bytes u8/u16 le | Identifies the spec version of encoded image |

* le: little-endian

## Containers/Meta chunks

In its' basic form, there are two meta chunks: a whole image information meta and image data meta:

### WIM (Whole Image Information Meta)

| Item                 | Type              | Description                       |
| -------------------- | ----------------- | --------------------------------- |
| Total frames         | 2-bytes u8/u16 le | Only useful for NQIA files, the parser checks if it is strictly 1 for NQII files |
| Total width          | Variable bytes le | Full width of image, stored in VLQ form, preceded by a sign bit |
| Total height         | Variable bytes le | Full height of image, stored in VLQ form, preceded by a sign bit |

### IDM (Image Data Meta)

| Item                 | Type              | Description                       |
| -------------------- | ----------------- | --------------------------------- |
| Index                | 2-bytes u8/u16 le | Only useful for NQIA files, only one image data meta exists in NQII files |
| Image width          | Variable bytes le | Full width of image, stored in VLQ form, preceded by a sign bit |
| Image height         | Variable bytes le | Full height of image, stored in VLQ form, preceded by a sign bit |
| X Offset             | Variable bytes le | X Offset of frame since last frame offset (differential), stored in VLQ form, preceded by a sign bit |
| Y Offset             | Variable bytes le | Y Offset of frame since last frame offset (differential), stored in VLQ form, preceded by a sign bit |
| Attributes           | 2-bytes u8/u16 le | See attributes sub-section |
| Image data           | Variable bytes    | Compressed data, with 4-byte u8/u32 le size preceding it |

### IDM Attributes

| Item                 | Type              | Description                       |
| -------------------- | ----------------- | --------------------------------- |
| Alpha image          | 1 bit             | Image contains a compressed alpha channel |
| Image type           | 1 bit             | 0 - RGB888, 1 - RGB565 |
| Key image            | 1 bit             | Image is an animation/key frame |
| Common offs multip.  | 7 bits            | 1 if not used, else use this multiplier to save on X/Y offset bits for the block decompression |
| Reserved             | 6 bits            | No use under current spec, must be zeroes |
