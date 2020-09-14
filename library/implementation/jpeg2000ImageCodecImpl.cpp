/*
Copyright 2005 - 2017 by Paolo Brandoli/Binarno s.p.

Imebra is available for free under the GNU General Public License.

The full text of the license is available in the file license.rst
 in the project root folder.

If you do not want to be bound by the GPL terms (such as the requirement 
 that your application must also be GPL), you may purchase a commercial 
 license for Imebra from the Imebra’s website (http://imebra.com).
*/

/*! \file jpeg2000ImageCodec.cpp
    \brief Implementation of the class jpeg2000ImageCodec.

*/

#ifdef JPEG2000

#include "exceptionImpl.h"
#include "streamReaderImpl.h"
#include "streamWriterImpl.h"
#include "memoryStreamImpl.h"
#include "huffmanTableImpl.h"
#include "jpeg2000ImageCodecImpl.h"
#include "dataSetImpl.h"
#include "imageImpl.h"
#include "dataHandlerNumericImpl.h"
#include "codecFactoryImpl.h"
#include "../include/imebra/exceptions.h"
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cstdio>
#include <assert.h>

extern "C"
{
#include <openjpeg.h>
}

namespace imebra {

    namespace implementation {

        namespace codecs {

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
//
//
// jpegCodec
//
//
//
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////


#ifdef JPEG2000_V2


#define gdcmErrorMacro(msg) {}
#define  gdcmWarningMacro(msg) {}
#define  gdcmDebugMacro(msg){}


            class SwapperNoOp {
            public:
                template<typename T>
                static T Swap(T val) { return val; }

                template<typename T>
                static void SwapArray(T *, size_t) {}
            };

            class SwapperDoOp {
            public:
                template<typename T>
                static T Swap(T val);

                template<typename T>
                static void SwapArray(T *array, size_t n) {
                    // TODO: need to unroll loop:
                    for (size_t i = 0; i < n; ++i) {
                        array[i] = Swap<T>(array[i]);
                    }
                }
            };

            typedef enum {
                FF30 = 0xFF30,
                FF31 = 0xFF31,
                FF32 = 0xFF32,
                FF33 = 0xFF33,
                FF34 = 0xFF34,
                FF35 = 0xFF35,
                FF36 = 0xFF36,
                FF37 = 0xFF37,
                FF38 = 0xFF38,
                FF39 = 0xFF39,
                FF3A = 0xFF3A,
                FF3B = 0xFF3B,
                FF3C = 0xFF3C,
                FF3D = 0xFF3D,
                FF3E = 0xFF3E,
                FF3F = 0xFF3F,
                SOC = 0xFF4F,
                CAP = 0xFF50,
                SIZ = 0xFF51,
                COD = 0xFF52,
                COC = 0xFF53,
                TLM = 0xFF55,
                PLM = 0XFF57,
                PLT = 0XFF58,
                QCD = 0xFF5C,
                QCC = 0xFF5D,
                RGN = 0xFF5E,
                POC = 0xFF5F,
                PPM = 0XFF60,
                PPT = 0XFF61,
                CRG = 0xFF63,
                COM = 0xFF64,
                SOT = 0xFF90,
                SOP = 0xFF91,
                EPH = 0XFF92,
                SOD = 0xFF93,
                EOC = 0XFFD9  /* EOI in old jpeg */
            } MarkerType;

            typedef enum {
                JP = 0x6a502020,
                FTYP = 0x66747970,
                JP2H = 0x6a703268,
                JP2C = 0x6a703263,
                JP2 = 0x6a703220,
                IHDR = 0x69686472,
                COLR = 0x636f6c72,
                XML = 0x786d6c20,
                CDEF = 0x63646566,
                CMAP = 0x636D6170,
                PCLR = 0x70636c72,
                RES = 0x72657320
            } OtherType;

            static inline bool hasnolength(uint_fast16_t marker) {
                switch (marker) {
                    case FF30:
                    case FF31:
                    case FF32:
                    case FF33:
                    case FF34:
                    case FF35:
                    case FF36:
                    case FF37:
                    case FF38:
                    case FF39:
                    case FF3A:
                    case FF3B:
                    case FF3C:
                    case FF3D:
                    case FF3E:
                    case FF3F:
                    case SOC:
                    case SOD:
                    case EOC:
                    case EPH:
                        return true;
                }
                return false;
            }


            static inline bool read16(const char **input, size_t *len, uint16_t *ret) {
                if (*len >= 2) {
                    union {
                        uint16_t v;
                        char bytes[2];
                    } u;
                    memcpy(u.bytes, *input, 2);
                    *ret = SwapperDoOp::Swap(u.v);
                    *input += 2;
                    *len -= 2;
                    return true;
                }
                return false;
            }


            static inline bool read32(const char **input, size_t *len, uint32_t *ret) {
                if (*len >= 4) {
                    union {
                        uint32_t v;
                        char bytes[4];
                    } u;
                    memcpy(u.bytes, *input, 4);
                    *ret = SwapperDoOp::Swap(u.v);
                    *input += 4;
                    *len -= 4;
                    return true;
                }
                return false;
            }

            static inline bool read64(const char **input, size_t *len, uint64_t *ret) {
                if (*len >= 8) {
                    union {
                        uint64_t v;
                        char bytes[8];
                    } u;
                    memcpy(u.bytes, *input, 8);
                    *ret = SwapperDoOp::Swap(u.v);
                    *input += 8;
                    *len -= 8;
                    return true;
                }
                return false;
            }


/**
sample error callback expecting a FILE* client object
*/
            void error_callback(const char *msg, void *) {
                (void) msg;
                gdcmErrorMacro("Error in gdcmopenjpeg" << msg);
            }

/**
sample warning callback expecting a FILE* client object
*/
            void warning_callback(const char *msg, void *) {
                (void) msg;
                gdcmWarningMacro("Warning in gdcmopenjpeg" << msg);
            }

/**
sample debug callback expecting no client object
*/
            void info_callback(const char *msg, void *) {
                (void) msg;
                gdcmDebugMacro("Info in gdcmopenjpeg" << msg);
            }

#define J2K_CFMT 0
#define JP2_CFMT 1
#define JPT_CFMT 2

#define PXM_DFMT 10
#define PGX_DFMT 11
#define BMP_DFMT 12
#define YUV_DFMT 13
#define TIF_DFMT 14
#define RAW_DFMT 15
#define TGA_DFMT 16
#define PNG_DFMT 17
#define CODEC_JP2 OPJ_CODEC_JP2
#define CODEC_J2K OPJ_CODEC_J2K
#define CLRSPC_GRAY OPJ_CLRSPC_GRAY
#define CLRSPC_SRGB OPJ_CLRSPC_SRGB

            struct myfile {
                char *mem;
                char *cur;
                size_t len;
            };

            void gdcm_error_callback(const char *msg, void *) {
                fprintf(stderr, "%s", msg);
            }


            OPJ_SIZE_T opj_read_from_memory(void *p_buffer, OPJ_SIZE_T p_nb_bytes, myfile *p_file) {
                //OPJ_UINT32 l_nb_read = fread(p_buffer,1,p_nb_bytes,p_file);
                OPJ_SIZE_T l_nb_read;
                if (p_file->cur + p_nb_bytes <= p_file->mem + p_file->len) {
                    l_nb_read = 1 * p_nb_bytes;
                } else {
                    l_nb_read = (OPJ_SIZE_T) (p_file->mem + p_file->len - p_file->cur);
                    assert(l_nb_read < p_nb_bytes);
                }
                memcpy(p_buffer, p_file->cur, l_nb_read);
                p_file->cur += l_nb_read;
                assert(p_file->cur <= p_file->mem + p_file->len);
                //std::cout << "l_nb_read: " << l_nb_read << std::endl;
                return l_nb_read ? l_nb_read : ((OPJ_SIZE_T) -1);
            }

            OPJ_SIZE_T opj_write_from_memory(void *p_buffer, OPJ_SIZE_T p_nb_bytes, myfile *p_file) {
                //return fwrite(p_buffer,1,p_nb_bytes,p_file);
                OPJ_SIZE_T l_nb_write;
                //if( p_file->cur + p_nb_bytes < p_file->mem + p_file->len )
                //  {
                l_nb_write = 1 * p_nb_bytes;
                //  }
                //else
                //  {
                //  l_nb_write = p_file->mem + p_file->len - p_file->cur;
                //  assert( l_nb_write < p_nb_bytes );
                //  }
                memcpy(p_file->cur, p_buffer, l_nb_write);
                p_file->cur += l_nb_write;
                p_file->len += l_nb_write;
                //assert( p_file->cur < p_file->mem + p_file->len );
                return l_nb_write;
                //return p_nb_bytes;
            }

            OPJ_OFF_T opj_skip_from_memory(OPJ_OFF_T p_nb_bytes, myfile *p_file) {
                //if (fseek(p_user_data,p_nb_bytes,SEEK_CUR))
                //  {
                //  return -1;
                //  }
                if (p_file->cur + p_nb_bytes <= p_file->mem + p_file->len) {
                    p_file->cur += p_nb_bytes;
                    return p_nb_bytes;
                }

                p_file->cur = p_file->mem + p_file->len;
                return -1;
            }

            OPJ_BOOL opj_seek_from_memory(OPJ_OFF_T p_nb_bytes, myfile *p_file) {
                //if (fseek(p_user_data,p_nb_bytes,SEEK_SET))
                //  {
                //  return false;
                //  }
                //return true;
                assert(p_nb_bytes >= 0);
                if ((size_t) p_nb_bytes <= p_file->len) {
                    p_file->cur = p_file->mem + p_nb_bytes;
                    return OPJ_TRUE;
                }

                p_file->cur = p_file->mem + p_file->len;
                return OPJ_FALSE;
            }

            opj_stream_t *
            OPJ_CALLCONV opj_stream_create_memory_stream(myfile *p_mem, OPJ_SIZE_T p_size, bool p_is_read_stream) {
                opj_stream_t *l_stream = nullptr;
                if
                        (!p_mem) {
                    return nullptr;
                }
                l_stream = opj_stream_create(p_size, p_is_read_stream);
                if
                        (!l_stream) {
                    return nullptr;
                }
                opj_stream_set_user_data(l_stream, p_mem, nullptr);
                opj_stream_set_read_function(l_stream, (opj_stream_read_fn) opj_read_from_memory);
                opj_stream_set_write_function(l_stream, (opj_stream_write_fn) opj_write_from_memory);
                opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn) opj_skip_from_memory);
                opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn) opj_seek_from_memory);
                opj_stream_set_user_data_length(l_stream, p_mem->len /* p_size*/); /* important to avoid an assert() */
                return l_stream;
            }


/*
 * Divide an integer by a power of 2 and round upwards.
 *
 * a divided by 2^b
 */
            inline int int_ceildivpow2(int a, int b) {
                return (a + (1 << b) - 1) >> b;
            }


// Represents a Jpeg2000 memory stream information
            typedef struct {
                OPJ_UINT8 *pData; //Our data.
                OPJ_SIZE_T dataSize; //How big is our data.
                OPJ_SIZE_T offset; //Where are we currently in our data.
            } opj_memory_stream;


// Read from memory stream
            static OPJ_SIZE_T opj_memory_stream_read(void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data) {
                opj_memory_stream *l_memory_stream = (opj_memory_stream *) p_user_data;//Our data.
                OPJ_SIZE_T l_nb_bytes_read = p_nb_bytes;//Amount to move to buffer.

                //Check if the current offset is outside our data buffer.
                if (l_memory_stream->offset >= l_memory_stream->dataSize) return (OPJ_SIZE_T) -1;

                //Check if we are reading more than we have.
                if (p_nb_bytes > (l_memory_stream->dataSize - l_memory_stream->offset)) {
                    l_nb_bytes_read = l_memory_stream->dataSize - l_memory_stream->offset;//Read all we have.
                }

                //Copy the data to the internal buffer.
                memcpy(p_buffer, &(l_memory_stream->pData[l_memory_stream->offset]), l_nb_bytes_read);
                l_memory_stream->offset += l_nb_bytes_read;//Update the pointer to the new location.
                return l_nb_bytes_read;
            }

//This will write from the buffer to our memory.
            static OPJ_SIZE_T opj_memory_stream_write(void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data) {
                opj_memory_stream *l_memory_stream = (opj_memory_stream *) p_user_data;//Our data.
                OPJ_SIZE_T l_nb_bytes_write = p_nb_bytes;//Amount to move to buffer.

                //Check if the current offset is outside our data buffer.
                if (l_memory_stream->offset >= l_memory_stream->dataSize) return (OPJ_SIZE_T) -1;

                //Check if we are write more than we have space for.
                if (p_nb_bytes > (l_memory_stream->dataSize - l_memory_stream->offset)) {
                    l_nb_bytes_write = l_memory_stream->dataSize - l_memory_stream->offset;//Write the remaining space.
                }

                //Copy the data from the internal buffer.
                memcpy(&(l_memory_stream->pData[l_memory_stream->offset]), p_buffer, l_nb_bytes_write);
                l_memory_stream->offset += l_nb_bytes_write;//Update the pointer to the new location.
                return l_nb_bytes_write;
            }

//Moves the pointer forward, but never more than we have.
            static OPJ_OFF_T opj_memory_stream_skip(OPJ_OFF_T p_nb_bytes, void *p_user_data) {
                opj_memory_stream *l_memory_stream = (opj_memory_stream *) p_user_data;
                OPJ_SIZE_T l_nb_bytes;

                if (p_nb_bytes < 0) return -1;//No skipping backwards.
                {
                    l_nb_bytes = (OPJ_SIZE_T) p_nb_bytes;//Allowed because it is positive.
                }

                // Do not allow jumping past the end.
                if (l_nb_bytes > l_memory_stream->dataSize - l_memory_stream->offset) {
                    l_nb_bytes = l_memory_stream->dataSize - l_memory_stream->offset;//Jump the max.
                }

                //Make the jump.
                l_memory_stream->offset += l_nb_bytes;

                //Returm how far we jumped.
                return l_nb_bytes;
            }

//Sets the pointer to anywhere in the memory.
            static OPJ_BOOL opj_memory_stream_seek(OPJ_OFF_T p_nb_bytes, void *p_user_data) {
                opj_memory_stream *l_memory_stream = (opj_memory_stream *) p_user_data;

                if (p_nb_bytes < 0) {
                    return OPJ_FALSE;
                }

                if (p_nb_bytes > (OPJ_OFF_T) l_memory_stream->dataSize) {
                    return OPJ_FALSE;
                }

                l_memory_stream->offset = (OPJ_SIZE_T) p_nb_bytes;//Move to new position.
                return OPJ_TRUE;
            }

//The system needs a routine to do when finished, the name tells you what I want it to do.
            static void opj_memory_stream_do_nothing(void * /* p_user_data */) {
            }

//Create a stream to use memory as the input or output.
            opj_stream_t *
            opj_stream_create_default_memory_stream(opj_memory_stream *p_memoryStream, OPJ_BOOL p_is_read_stream) {
                opj_stream_t *l_stream;

                if (!(l_stream = opj_stream_default_create(p_is_read_stream))) {
                    return (NULL);
                }

                //Set how to work with the frame buffer.
                if (p_is_read_stream) {
                    opj_stream_set_read_function(l_stream, opj_memory_stream_read);
                } else {
                    opj_stream_set_write_function(l_stream, opj_memory_stream_write);
                }

                opj_stream_set_seek_function(l_stream, opj_memory_stream_seek);
                opj_stream_set_skip_function(l_stream, opj_memory_stream_skip);
                opj_stream_set_user_data(l_stream, p_memoryStream, opj_memory_stream_do_nothing);
                opj_stream_set_user_data_length(l_stream, p_memoryStream->dataSize);
                return l_stream;
            }


            template<typename T>
            void rawtoimage_fill2(const T *inputbuffer, int w, int h, int numcomps, opj_image_t *image, int pc,
                                  int bitsallocated, int bitsstored, int highbit, int sign) {
                uint16_t pmask = 0xffff;
                pmask = (uint16_t) (pmask >> (bitsallocated - bitsstored));

                const T *p = inputbuffer;
                if (sign) {
                    // smask : to check the 'sign' when BitsStored != BitsAllocated
                    uint16_t smask = 0x0001;
                    smask = (uint16_t) (
                            smask << (16 - (bitsallocated - bitsstored + 1)));
                    // nmask : to propagate sign bit on negative values
                    int16_t nmask = (int16_t) 0x8000;
                    nmask = (int16_t) (nmask >> (bitsallocated - bitsstored - 1));
                    if (pc) {
                        for (int compno = 0; compno < numcomps; compno++) {
                            for (int i = 0; i < w * h; i++) {
                                /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
                                uint16_t c = *p;
                                c = (uint16_t) (c >> (bitsstored - highbit - 1));
                                if (c & smask) {
                                    c = (uint16_t) (c | nmask);
                                } else {
                                    c = c & pmask;
                                }
                                int16_t fix;
                                memcpy(&fix, &c, sizeof fix);
                                image->comps[compno].data[i] = fix;
                                ++p;
                            }
                        }
                    } else {
                        for (int i = 0; i < w * h; i++) {
                            for (int compno = 0; compno < numcomps; compno++) {
                                /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
                                uint16_t c = *p;
                                c = (uint16_t) (c >> (bitsstored - highbit - 1));
                                if (c & smask) {
                                    c = (uint16_t) (c | nmask);
                                } else {
                                    c = c & pmask;
                                }
                                int16_t fix;
                                memcpy(&fix, &c, sizeof fix);
                                image->comps[compno].data[i] = fix;
                                ++p;
                            }
                        }
                    }
                } else {
                    if (pc) {
                        for (int compno = 0; compno < numcomps; compno++) {
                            for (int i = 0; i < w * h; i++) {
                                /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
                                uint16_t c = *p;
                                c = (uint16_t) (
                                        (c >> (bitsstored - highbit - 1)) & pmask);
                                image->comps[compno].data[i] = c;
                                ++p;
                            }
                        }
                    } else {
                        for (int i = 0; i < w * h; i++) {
                            for (int compno = 0; compno < numcomps; compno++) {
                                /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
                                uint16_t c = *p;
                                c = (uint16_t) (
                                        (c >> (bitsstored - highbit - 1)) & pmask);
                                image->comps[compno].data[i] = c;
                                ++p;
                            }
                        }
                    }
                }
            }

            template<typename T>
            void rawtoimage_fill(const T *inputbuffer, int w, int h, int numcomps, opj_image_t *image, int pc) {
                const T *p = inputbuffer;
                if (pc) {
                    for (int compno = 0; compno < numcomps; compno++) {
                        for (int i = 0; i < w * h; i++) {
                            /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
                            image->comps[compno].data[i] = *p;
                            ++p;
                        }
                    }
                } else {
                    for (int i = 0; i < w * h; i++) {
                        for (int compno = 0; compno < numcomps; compno++) {
                            /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
                            image->comps[compno].data[i] = *p;
                            ++p;
                        }
                    }
                }
            }

            opj_image_t *rawtoimage(const char *inputbuffer8, opj_cparameters_t *parameters,
                                    size_t fragment_size, int image_width, int image_height, int sample_pixel,
                                    int bitsallocated, int bitsstored, int highbit, int sign, int quality, int pc) {
                (void) quality;
                (void) fragment_size;
                int w, h;
                int numcomps;
                OPJ_COLOR_SPACE color_space;
                opj_image_cmptparm_t cmptparm[3]; /* maximum of 3 components */
                opj_image_t *image = nullptr;
                const void *inputbuffer = inputbuffer8;

                assert(sample_pixel == 1 || sample_pixel == 3);
                if (sample_pixel == 1) {
                    numcomps = 1;
                    color_space = CLRSPC_GRAY;
                } else // sample_pixel == 3
                {
                    numcomps = 3;
                    color_space = CLRSPC_SRGB;
                    /* Does OpenJPEg support: CLRSPC_SYCC ?? */
                }
                if (bitsallocated % 8 != 0) {
                    gdcmDebugMacro("BitsAllocated is not % 8");
                    return nullptr;
                }
                assert(bitsallocated % 8 == 0);
                // eg. fragment_size == 63532 and 181 * 117 * 3 * 8 == 63531 ...
                assert(((fragment_size + 1) / 2) * 2 ==
                       (((size_t) image_height * image_width * numcomps * (bitsallocated / 8) + 1) / 2) * 2);
                int subsampling_dx = parameters->subsampling_dx;
                int subsampling_dy = parameters->subsampling_dy;

                // FIXME
                w = image_width;
                h = image_height;

                /* initialize image components */
                memset(&cmptparm[0], 0, 3 * sizeof(opj_image_cmptparm_t));
                //assert( bitsallocated == 8 );
                for (int i = 0; i < numcomps; i++) {
                    cmptparm[i].prec = bitsstored;
                    cmptparm[i].prec = bitsallocated; // FIXME
                    cmptparm[i].bpp = bitsallocated;
                    cmptparm[i].sgnd = sign;
                    cmptparm[i].dx = subsampling_dx;
                    cmptparm[i].dy = subsampling_dy;
                    cmptparm[i].w = w;
                    cmptparm[i].h = h;
                }

                /* create the image */
                image = opj_image_create(numcomps, &cmptparm[0], color_space);
                if (!image) {
                    return nullptr;
                }
                /* set image offset and reference grid */
                image->x0 = parameters->image_offset_x0;
                image->y0 = parameters->image_offset_y0;
                image->x1 = parameters->image_offset_x0 + (w - 1) * subsampling_dx + 1;
                image->y1 = parameters->image_offset_y0 + (h - 1) * subsampling_dy + 1;

                /* set image data */

                //assert( fragment_size == numcomps*w*h*(bitsallocated/8) );
                if (bitsallocated <= 8) {
                    if (sign) {
                        rawtoimage_fill<int8_t>((const int8_t *) inputbuffer, w, h, numcomps, image, pc);
                    } else {
                        rawtoimage_fill<uint8_t>((const uint8_t *) inputbuffer, w, h, numcomps, image, pc);
                    }
                } else if (bitsallocated <= 16) {
                    if (bitsallocated != bitsstored) {
                        if (sign) {
                            rawtoimage_fill2<int16_t>((const int16_t *) inputbuffer, w, h, numcomps, image, pc,
                                                      bitsallocated, bitsstored, highbit, sign);
                        } else {
                            rawtoimage_fill2<uint16_t>((const uint16_t *) inputbuffer, w, h, numcomps, image, pc,
                                                       bitsallocated, bitsstored, highbit, sign);
                        }
                    } else {
                        if (sign) {
                            rawtoimage_fill<int16_t>((const int16_t *) inputbuffer, w, h, numcomps, image, pc);
                        } else {
                            rawtoimage_fill<uint16_t>((const uint16_t *) inputbuffer, w, h, numcomps, image, pc);
                        }
                    }
                } else if (bitsallocated <= 32) {
                    if (sign) {
                        rawtoimage_fill<int32_t>((const int32_t *) inputbuffer, w, h, numcomps, image, pc);
                    } else {
                        rawtoimage_fill<uint32_t>((const uint32_t *) inputbuffer, w, h, numcomps, image, pc);
                    }
                } else // dead branch ?
                {
                    opj_image_destroy(image);
                    return nullptr;
                }

                return image;
            }


#endif

            class JPEG2000Internals {
            public:
                JPEG2000Internals()
                        : nNumberOfThreadsForDecompression(-1) {
                    memset(&coder_param, 0, sizeof(coder_param));
                    opj_set_default_encoder_parameters(&coder_param);
                }

                opj_cparameters coder_param;
                int nNumberOfThreadsForDecompression;
            };

            jpeg2000ImageCodec::jpeg2000ImageCodec() {
                Internals = new JPEG2000Internals;
            }

            jpeg2000ImageCodec::~jpeg2000ImageCodec() {
                delete Internals;
            }

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//
//
// Returns true if the codec can handle the specified transfer
//  syntax
//
//
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
            bool jpeg2000ImageCodec::canHandleTransferSyntax(const std::string &transferSyntax) const {
                IMEBRA_FUNCTION_START() ;

                    return (
                            transferSyntax == "1.2.840.10008.1.2.4.90" ||
                            transferSyntax == "1.2.840.10008.1.2.4.91");

                IMEBRA_FUNCTION_END();
            }


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//
//
// Returns true if the transfer syntax has to be
//  encapsulated
//
//
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
            bool jpeg2000ImageCodec::encapsulated(const std::string &transferSyntax) const {
                IMEBRA_FUNCTION_START() ;

                    if (!canHandleTransferSyntax(transferSyntax)) {
                        IMEBRA_THROW(CodecWrongTransferSyntaxError, "Cannot handle the transfer syntax");
                    }
                    return true;

                IMEBRA_FUNCTION_END();
            }

            bool jpeg2000ImageCodec::defaultInterleaved() const {
                return false;
            }


            std::shared_ptr<image> jpeg2000ImageCodec::getImage(const std::string &transferSyntax,
                                                                const std::string &colorSpace,
                                                                std::uint32_t channelsNumber,
                                                                std::uint32_t imageWidth,
                                                                std::uint32_t imageHeight,
                                                                bool bSubsampledX,
                                                                bool bSubsampledY,
                                                                bool bInterleaved,
                                                                bool b2Complement,
                                                                std::uint8_t allocatedBits,
                                                                std::uint8_t storedBits,
                                                                std::uint8_t highBit,
                                                                std::shared_ptr<streamReader> pSourceStream) const {
                IMEBRA_FUNCTION_START() ;

#ifdef JPEG2000_V1
                    CODEC_FORMAT format = (CODEC_FORMAT)(CODEC_J2K);
#else
                    CODEC_FORMAT format = (CODEC_FORMAT) (OPJ_CODEC_J2K);
#endif

                    bitDepth_t depth;
                    if(b2Complement)
                    {
                        if(highBit >= 16)
                        {
                            depth = bitDepth_t::depthS32;
                        }
                        else if(highBit >= 8)
                        {
                            depth = bitDepth_t::depthS16;
                        }
                        else
                        {
                            depth = bitDepth_t::depthS8;
                        }
                    }
                    else
                    {
                        if(highBit >= 16)
                        {
                            depth = bitDepth_t::depthU32;
                        }
                        else if(highBit >= 8)
                        {
                            depth = bitDepth_t::depthU16;
                        }
                        else
                        {
                            depth = bitDepth_t::depthU8;
                        }
                    }

                    std::shared_ptr<memory> jpegMemory(std::make_shared<memory>());
                    {
                        std::shared_ptr<memoryStreamOutput> memoryStream(
                                std::make_shared<memoryStreamOutput>(jpegMemory));
                        streamWriter memoryStreamWriter(memoryStream);
                        unsigned char tempBuffer[4096];
                        while (!pSourceStream->endReached()) {
                            size_t readLength = pSourceStream->readSome(tempBuffer, sizeof(tempBuffer));
                            memoryStreamWriter.write(tempBuffer, readLength);
                        }
                    }

                    opj_dparameters_t parameters;
                    opj_set_default_decoder_parameters(&parameters);
#ifdef JPEG2000_V1
                    opj_dinfo_t *dinfo;
#else
                    opj_codec_t *dinfo;
#endif

                    dinfo = opj_create_decompress(format);
                    opj_setup_decoder(dinfo, &parameters);
                    opj_image_t *jp2image(0);

#ifdef JPEG2000_V1
                    opj_cio_t* cio = opj_cio_open((opj_common_ptr)dinfo, jpegMemory->data(), (int)jpegMemory->size());
       if(cio != 0)
       {
           jp2image = opj_decode(dinfo, cio);
           opj_cio_close(cio);
       }
#else
                    opj_memory_stream memoryStream;
                    memoryStream.pData = jpegMemory->data();
                    memoryStream.dataSize = jpegMemory->size();
                    memoryStream.offset = 0;

                    opj_stream_t *pJpeg2000Stream = opj_stream_create_default_memory_stream(&memoryStream, true);
                    if (pJpeg2000Stream != 0) {
                        if (opj_read_header(pJpeg2000Stream, dinfo, &jp2image)) {
                            if (!opj_decode(dinfo, pJpeg2000Stream, jp2image)) {
                                opj_image_destroy(jp2image);
                                jp2image = 0;
                            }
                        }
                        opj_stream_destroy(pJpeg2000Stream);
                    }
                    opj_destroy_codec(dinfo);
#endif
                    if (jp2image == 0) {
                        IMEBRA_THROW(CodecCorruptedFileError, "Could not decode the jpeg2000 image");
                    }




                    std::shared_ptr<image> returnImage(
                            std::make_shared<image>(
                                    imageWidth,
                                    imageHeight,
                                    depth,
                                    colorSpace,
                                    highBit

                            ));

                    std::shared_ptr<handlers::writingDataHandlerNumericBase> imageHandler(
                            returnImage->getWritingDataHandler());

                    ::memset(imageHandler->getMemory()->data(), 0, imageHandler->getSize());

                    // Copy channels
                    for (unsigned int channelNumber(0); channelNumber != jp2image->numcomps; ++channelNumber) {
                        const opj_image_comp_t &channelData = jp2image->comps[channelNumber];

                        imageHandler->copyFromInt32Interleaved(channelData.data,
                                                               channelData.dx,
                                                               channelData.dy,
                                                               channelData.x0,
                                                               channelData.y0,
                                                               channelData.x0 + channelData.w * channelData.dx,
                                                               channelData.y0 + channelData.h * channelData.dy,
                                                               channelNumber,
                                                               imageWidth,
                                                               imageHeight,
                                                               jp2image->numcomps);
                    }

                    opj_image_destroy(jp2image);

                    return returnImage;


                IMEBRA_FUNCTION_END();

            }

/////////////////////////////
            bool jpeg2000ImageCodec::CodeFrameIntoBuffer(char *outdata, size_t outlen, size_t &complen,
                                                         const char *inputdata,
                                                         size_t inputlength,
                                                         uint32_t image_width,
                                                         uint32_t image_height,

                                                         uint32_t sample_pixel,
                                                         uint32_t bitsallocated,
                                                         uint32_t bitsstored,
                                                         uint32_t highbit,
                                                         uint32_t pixelRepresentation,
                                                         const std::shared_ptr<palette> pc
            ) const {
                complen = 0; // default init
#if 0
                if( NeedOverlayCleanup )
    {
    gdcmErrorMacro( "TODO" );
    return false;
    }
#endif
                //               const unsigned int *dims = this->GetDimensions();
//                int image_width = dims[0];
//                int image_height = dims[1];
                int numZ = 0; //dims[2];

//                int sample_pixel =  pf.GetSamplesPerPixel();
//                int bitsallocated =bitsallocated;
//                int bitsstored = bitsstored;
//                int highbit = highbit;
                int sign = pixelRepresentation;
                int quality = 100;

                //// input_buffer is ONE image
                //// fragment_size is the size of this image (fragment)
                (void) numZ;
                bool bSuccess;
                //bool delete_comment = true;
                opj_cparameters_t parameters;  /* compression parameters */
                opj_image_t *image = nullptr;
                //quality = 100;

                /* set encoding parameters to default values */
                //memset(&parameters, 0, sizeof(parameters));
                //opj_set_default_encoder_parameters(&parameters);

                memcpy(&parameters, &(Internals->coder_param), sizeof(parameters));

                if ((parameters.cp_disto_alloc || parameters.cp_fixed_alloc || parameters.cp_fixed_quality)
                    && (!(parameters.cp_disto_alloc ^ parameters.cp_fixed_alloc ^ parameters.cp_fixed_quality))) {

                    return false;
                }        /* mod fixed_quality */

                /* if no rate entered, lossless by default */
                if (parameters.tcp_numlayers == 0) {
                    parameters.tcp_rates[0] = 0;
                    parameters.tcp_numlayers = 1;
                    parameters.cp_disto_alloc = 1;
                }

                if (parameters.cp_comment == nullptr) {
                    const char comment[] = "Created by GDCM/OpenJPEG version %s";
                    const char *vers = opj_version();
                    parameters.cp_comment = (char *) malloc(strlen(comment) + 10);
                    snprintf(parameters.cp_comment, strlen(comment) + 10, comment, vers);
                    /* no need to delete parameters.cp_comment on exit */
                    //delete_comment = false;
                }

                // Compute the proper number of resolutions to use.
                // This is mostly done for images smaller than 64 pixels
                // along any dimension.
                unsigned int numberOfResolutions = 0;

                unsigned int tw = image_width >> 1;
                unsigned int th = image_height >> 1;

                while (tw && th) {
                    numberOfResolutions++;
                    tw >>= 1;
                    th >>= 1;
                }

                // Clamp the number of resolutions to 6.
                if (numberOfResolutions > 6) {
                    numberOfResolutions = 6;
                }

                parameters.numresolution = numberOfResolutions;


                /* decode the source image */
                /* ----------------------- */

                int pcx = 0;
                if (pc) {
                    pcx = 1;
                }
                image = rawtoimage((const char *) inputdata, &parameters,
                                   inputlength,
                                   image_width, image_height,
                                   sample_pixel, bitsallocated, bitsstored, highbit, sign, quality,
                                   pcx);
                if (!image) {
                    return false;
                }

                /* encode the destination image */
                /* ---------------------------- */
                parameters.cod_format = J2K_CFMT; /* J2K format output */
                size_t codestream_length;
                opj_codec_t *cinfo = nullptr;
                opj_stream_t *cio = nullptr;

                /* get a J2K compressor handle */
                cinfo = opj_create_compress(CODEC_J2K);


                /* setup the encoder parameters using the current image and using user parameters */
                opj_setup_encoder(cinfo, &parameters, image);

                myfile mysrc;
                myfile *fsrc = &mysrc;
                char *buffer_j2k = new char[inputlength]; // overallocated
                fsrc->mem = fsrc->cur = buffer_j2k;
                fsrc->len = 0; //inputlength;

                /* open a byte stream for writing */
                /* allocate memory for all tiles */
                cio = opj_stream_create_memory_stream(fsrc, OPJ_J2K_STREAM_CHUNK_SIZE, false);
                if (!cio) {
                    delete [] buffer_j2k;
                    return false;
                }
                /* encode the image */
                /*if (*indexfilename)          // If need to extract codestream information
                  bSuccess = opj_encode_with_info(cinfo, cio, image, &cstr_info);
                  else*/
                bSuccess = opj_start_compress(cinfo, image, cio) ? true : false;
                bSuccess = bSuccess && opj_encode(cinfo, cio);
                bSuccess = bSuccess && opj_end_compress(cinfo, cio);

                if (!bSuccess) {
                    delete [] buffer_j2k;
                    opj_stream_destroy(cio);
                    return false;
                }
                codestream_length = mysrc.len;

                /* write the buffer to disk */
                //f = fopen(parameters.outfile, "wb");
                //if (!f) {
                //  fprintf(stderr, "failed to open %s for writing\n", parameters.outfile);
                //  return 1;
                //}
                //fwrite(cio->buffer, 1, codestream_length, f);
                //#define MDEBUG
#ifdef MDEBUG
                static int c = 0;
  std::ostringstream os;
  os << "/tmp/debug";
  os << c;
  c++;
  os << ".j2k";
  std::ofstream debug(os.str().c_str(), std::ios::binary);
  debug.write((char*)(cio->buffer), codestream_length);
  debug.close();
#endif

                bool success = false;
                if (codestream_length <= outlen) {
                    success = true;
                    memcpy(outdata, (char *) (mysrc.mem), codestream_length);
                }
                delete[] buffer_j2k;

                /* close and free the byte stream */
                opj_stream_destroy(cio);

                /* free remaining compression structures */
                opj_destroy_codec(cinfo);
                complen = codestream_length;

                /* free user parameters structure */
                if (parameters.cp_comment) free(parameters.cp_comment);
                if (parameters.cp_matrice) free(parameters.cp_matrice);

                /* free image data */
                opj_image_destroy(image);

                return success;
            }

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//
//
// Return the suggested allocated bits
//
//
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
            std::uint32_t
            jpeg2000ImageCodec::suggestAllocatedBits(const std::string &transferSyntax, std::uint32_t highBit) const {
                IMEBRA_FUNCTION_START() ;

                const int  hb = highBit +1 ;
                    if (transferSyntax == uidJPEG2000ImageCompressionLosslessOnly_1_2_840_10008_1_2_4_90 ||
                        transferSyntax == uidJPEG2000ImageCompression_1_2_840_10008_1_2_4_91
                            ) {
                        if (hb <= 8) {
                            return 8;
                        } else if (hb <= 16) {
                            return 16;
                        } else if (hb <= 32) {
                            return 32;
                        } else {
                            IMEBRA_THROW(DataSetUnknownTransferSyntaxError, "highBit  value out of range ");
                        }

                    } else {
                        IMEBRA_THROW(DataSetUnknownTransferSyntaxError,
                                     "Only Support 1.2.840.10008.1.2.4.90  and 1.2.840.10008.1.2.4.91");

                    }
//        if( comp->prec <= 8 )
//            PF.SetBitsAllocated( 8 );
//        else if( comp->prec <= 16 )
//            PF.SetBitsAllocated( 16 );
//        else if( comp->prec <= 32 )
//            PF.SetBitsAllocated( 32 );
//        PF.SetBitsStored( (unsigned short)comp->prec );
//        PF.SetHighBit( (unsigned short)(comp->prec - 1) ); // ??

                IMEBRA_FUNCTION_END();
            }


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
//
//
// Write an image into the dataset
//
//
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
            void jpeg2000ImageCodec::setImage(
                    std::shared_ptr<streamWriter> pDestStream,
                    std::shared_ptr<const image> pSourceImage,
                    const std::string &transferSyntax,
                    imageQuality_t imageQuality,
                    std::uint32_t allocatedBits,
                    bool bSubSampledX,
                    bool bSubSampledY,
                    bool bInterleaved,
                    bool b2Complement) const {
                IMEBRA_FUNCTION_START() ;
                    if (!canHandleTransferSyntax(transferSyntax)) {
                        IMEBRA_THROW(DataSetUnknownTransferSyntaxError,
                                     " setImage  unsupport the specified transfer syntax JPEG2000");
                    }


                    bitDepth_t depth;
                    uint32_t highBit = pSourceImage->getHighBit();
                    if(b2Complement)
                    {
                        if(highBit >= 16)
                        {
                            depth = bitDepth_t::depthS32;
                        }
                        else if(highBit >= 8)
                        {
                            depth = bitDepth_t::depthS16;
                        }
                        else
                        {
                            depth = bitDepth_t::depthS8;
                        }
                    }
                    else
                    {
                        if(highBit >= 16)
                        {
                            depth = bitDepth_t::depthU32;
                        }
                        else if(highBit >= 8)
                        {
                            depth = bitDepth_t::depthU16;
                        }
                        else
                        {
                            depth = bitDepth_t::depthU8;
                        }
                    }

                    if(depth !=  pSourceImage->getDepth() ){
                        IMEBRA_THROW(DataSetUnknownTransferSyntaxError,
                                     " change to  JPEG2000 bitDepth_t  is not equal !");

                    }

                    std::shared_ptr<handlers::readingDataHandlerNumericBase> dataReader = pSourceImage->getReadingDataHandler();

                    size_t sz = dataReader->getMemorySize();
                    const std::uint8_t *buffer = dataReader->getMemoryBuffer();
                    uint32_t image_width = 0;
                    uint32_t image_height = 0 ;
                    pSourceImage->getSize(&image_width, &image_height);

                    uint32_t sample_pixel = pSourceImage->getChannelsNumber() ;
                    uint32_t bitsallocated =  allocatedBits;// pSourceImage->getBitsallocated();
                    uint32_t bitsstored = highBit + 1 ;// this ->suggestAllocatedBits(transferSyntax, highBit) ;
                    uint32_t pixelRepresentation =  b2Complement  ?  1 : 0 ;
                    std::shared_ptr<palette> pc = pSourceImage->getPalette();
                    std::vector<char> rgbyteCompressed;
                    rgbyteCompressed.resize(image_width * image_height * 4);
                    size_t cbyteCompressed;
                    bool b = CodeFrameIntoBuffer((char *) &rgbyteCompressed[0], rgbyteCompressed.size(),
                                                 cbyteCompressed, reinterpret_cast<const char *>(buffer), sz,
                                                 image_width,
                                                 image_height,
                                                 sample_pixel,
                                                 bitsallocated,
                                                 bitsstored,
                                                 highBit,
                                                 pixelRepresentation,
                                                 pc
                    );
                    if (!b) {
                        IMEBRA_THROW(DataSetUnknownTransferSyntaxError,
                                     " change to  JPEG2000 failed !");
                    }
                    pDestStream->write(reinterpret_cast<const uint8_t *>(&rgbyteCompressed[0]),
                                       (uint32_t) cbyteCompressed);



                IMEBRA_FUNCTION_END();
            }


        } // namespace codecs

    } // namespace implementation

} // namespace imebra

#endif //JPEG2000
