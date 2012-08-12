#ifndef __MPQ_COMPRESS_H__
#define __MPQ_COMPRESS_H__

#define ID_WAVE         0x46464952

uint32 zlib_compress (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 zlib_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 bzip2_compress (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 bzip2_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 pkzip_compress (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 pkzip_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 wave_compress_mono (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 wave_decompress_mono (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 wave_compress_stereo (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 wave_decompress_stereo (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 huff_compress (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 huff_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size);

uint32 mpq_compress (uint8* in, uint32 in_size, uint8* out, uint32* out_size, uint8* temp, uint32 methods);
uint32 mpq_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size);
uint32 mpq_part_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size);

#endif // __MPQ_COMPRESS_H__
