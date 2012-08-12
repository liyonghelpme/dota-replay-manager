#include "stdafx.h"
#include "image.h"
#include "utils.h"

extern "C"
{
  #include "jpeg/jpeglib.h"
  #include "jpeg/jerror.h"
}
#pragma comment (lib, "jpeg\\jpeg.lib")
#include <setjmp.h>

struct BLPHeader
{
  uint32 sig;
  uint32 compression;
  uint32 flags;
  uint32 width;
  uint32 height;
  uint32 type;
  uint32 subType;
  uint32 mipOffs[16];
  uint32 mipSize[16];
};

struct my_error_mgr
{
  jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};
static void initSource (j_decompress_ptr)
{
}
static boolean fillInputBuffer (j_decompress_ptr)
{
  return TRUE;
}
static void skipInputData (j_decompress_ptr cinfo, long count)
{
  jpeg_source_mgr* src = cinfo->src;
  if (count > 0)
  {
    src->bytes_in_buffer -= count;
    src->next_input_byte += count;
  }
}
static void termSource (j_decompress_ptr)
{
}
static void myErrorExit (j_common_ptr cinfo)
{
  my_error_mgr* myerr = (my_error_mgr*) cinfo->err;
  longjmp (myerr->setjmp_buffer, 1);
}
BLPImage* LoadJPEG (uint8* data, uint32 size)
{
  jpeg_decompress_struct cinfo;
  my_error_mgr jerr;

  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = myErrorExit;

  if (setjmp (jerr.setjmp_buffer))
  {
    jpeg_destroy_decompress (&cinfo);
    return NULL;
  }

  jpeg_create_decompress (&cinfo);
  jpeg_source_mgr jsrc;

  jsrc.bytes_in_buffer = size;
  jsrc.next_input_byte = (JOCTET*) data;
  jsrc.init_source = initSource;
  jsrc.fill_input_buffer = fillInputBuffer;
  jsrc.skip_input_data = skipInputData;
  jsrc.resync_to_restart = jpeg_resync_to_restart;
  jsrc.term_source = termSource;
  cinfo.src = &jsrc;

  jpeg_read_header (&cinfo, TRUE);
  jpeg_calc_output_dimensions (&cinfo);
  jpeg_start_decompress (&cinfo);

  BLPImage* img = new BLPImage;
  img->width = cinfo.output_width;
  img->height = cinfo.output_height;
  img->data = new uint32[img->width * img->height];
  int rowSpan = cinfo.image_width * cinfo.num_components;
  bool isGreyScale = (cinfo.jpeg_color_space == JCS_GRAYSCALE);
  uint32 palette[256];

  if (cinfo.quantize_colors)
  {
    int shift = 8 - cinfo.data_precision;
    if (cinfo.jpeg_color_space != JCS_GRAYSCALE)
    {
      for (int i = 0; i < cinfo.actual_number_of_colors; i++)
        palette[i] = (cinfo.colormap [0][i] << (shift + 16)) + (cinfo.colormap [1][i] << (shift + 8)) + (cinfo.colormap [2][i] << shift);
    }
    else
    {
      for (int i = 0; i < cinfo.actual_number_of_colors; i++)
        palette[i] = (cinfo.colormap [0][i] << (shift + 16)) + (cinfo.colormap [0][i] << (shift + 8)) + (cinfo.colormap [0][i] << shift);
    }
  }

  JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, rowSpan, 1);

  int y = 0;
  while (cinfo.output_scanline < cinfo.output_height)
  {
    jpeg_read_scanlines (&cinfo, buffer, 1);
    DWORD* lineBuf = &img->data[img->width * y];
    if (cinfo.output_components == 1)
    {
      if (cinfo.quantize_colors)
      {
        for (int i = 0; i < img->width; i++)
          lineBuf[i] = palette[buffer[0][i]];
      }
      else
      {
        for (int i = 0; i < img->width; i++)
          lineBuf [i] = (buffer[0][i] << 16) + (buffer[0][i] << 8) + buffer[0][i];
      }
    }
    else
    {
      unsigned char* ptr = buffer[0];
      for (int i = 0; i < img->width; i++, ptr += 3)
      {
        lineBuf[i] = (ptr[0] << 16) + (ptr[1] << 8) + ptr[2];
        if (cinfo.num_components > 3)
        {
          lineBuf[i] |= (ptr[3] << 24);
          ptr++;
        }
        else
          lineBuf[i] |= 0xFF000000;
      }
    }
    y++;
  }

  jpeg_finish_decompress (&cinfo);
  jpeg_destroy_decompress (&cinfo);

  return img;
}

BLPImage* LoadBLP (MPQFILE blp)
{
  BLPHeader hdr;
  MPQFileRead (blp, sizeof hdr, &hdr);
  if (hdr.sig != flipInt ('BLP1'))
    return NULL;
  if (hdr.compression == 0)
  {
    uint32 hsize;
    MPQFileRead (blp, sizeof hsize, &hsize);
    uint8* data = new uint8[hsize + hdr.mipSize[0]];
    MPQFileRead (blp, hsize, data);
    MPQFileSeek (blp, hdr.mipOffs[0], MPQSEEK_SET);
    MPQFileRead (blp, hdr.mipSize[0], data + hsize);
    BLPImage* res = LoadJPEG (data, hsize + hdr.mipSize[0]);
    delete[] data;
    return res;
  }
  else
  {
    BLPImage* img = new BLPImage;
    img->width = hdr.width;
    img->height = hdr.height;
    img->data = new uint32[img->width * img->height];
    uint32 pal[256];
    MPQFileRead (blp, sizeof pal, pal);
    MPQFileSeek (blp, hdr.mipOffs[0], SEEK_SET);
    //for (int i = 0; i < 256; i++)
    //{
    //  int tmp = (pal[i] & 0xFF);
    //  pal[i] = (pal[i] & 0xFFFFFF00) | ((pal[i] >> 16) & 0x000000FF);
    //  pal[i] = (pal[i] & 0xFF00FFFF) | ((tmp << 16) & 0x00FF0000);
    //}
    if (hdr.type == 5)
    {
      for (int y = 0; y < img->height; y++)
      {
        for (int x = 0; x < img->width; x++)
        {
          uint8 i = (uint8) MPQFileGetc (blp);
          img->data[y * img->width + x] = (pal[i] & 0x00FFFFFF) | ((255 - (pal[i] >> 24)) << 24);
        }
      }
    }
    else
    {
      for (int y = 0; y < img->height; y++)
      {
        for (int x = 0; x < img->width; x++)
        {
          uint8 i = (uint8) MPQFileGetc (blp);
          img->data[y * img->width + x] = (pal[i] & 0x00FFFFFF);
        }
      }
      for (int y = 0; y < img->height; y++)
      {
        for (int x = 0; x < img->width; x++)
        {
          uint8 a = (uint8) MPQFileGetc (blp);
          img->data[y * img->width + x] |= (a << 24);
        }
      }
    }
    return img;
  }
}
