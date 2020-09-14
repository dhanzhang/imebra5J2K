/*
Copyright 2005 - 2017 by Paolo Brandoli/Binarno s.p.

Imebra is available for free under the GNU General Public License.

The full text of the license is available in the file license.rst
 in the project root folder.

If you do not want to be bound by the GPL terms (such as the requirement
 that your application must also be GPL), you may purchase a commercial
 license for Imebra from the Imebra’s website (http://imebra.com).
*/

/*! \file image.h
    \brief Declaration of the class image.

*/

#if !defined(imebraImage_A807A3CA_FA04_44f4_85D2_C7AA2FE103C4__INCLUDED_)
#define imebraImage_A807A3CA_FA04_44f4_85D2_C7AA2FE103C4__INCLUDED_


#include <memory>
#include "dataHandlerNumericImpl.h"
#include "../include/imebra/definitions.h"
#include "assert.h"

namespace imebra {

    namespace implementation {

        namespace handlers {
            class dataHandlerNumericBase;
        }

        class palette;

        class buffer;

/// \addtogroup group_image Image data
/// \brief The class image contains the data of one DICOM image.
///
/// The image's data includes:
/// - the image's size, in pixels
/// - the image's size, in millimeters
/// - the bit depth (bytes per color channel) and high
///   bit
/// - the color palette (if available)
/// - the pixels' data
///
/// An image can be obtained from a dataSet object by
///  calling dataSet::getImage(), or it can be initialized
///  with image::create().
///
/// Images can also be allocated by the transforms
///  by calling
///  transforms::transform::allocateOutputImage().
///
/// @{


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
/// \brief Represents a single image of a dicom data set.
///
/// Images are embedded into the dicom structures
///  (represented by the dataSet class), stored in
///  a compressed format.
///
/// The class image represents a decompressed raw image,
///  extracted from a dicom structure using
///  dataSet::getImage().
///
/// image objects can also be created by the
///  application and stored into a dicom structure using
///  the function dataSet::setImage().
///
/// The image and its buffer share a common lock object:
///  this means that a lock to the image object will also
///  locks the image's buffer and viceversa.
///
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
        class image {
        public:

            // Constructor
            ///////////////////////////////////////////////////////////
            image(
                    std::uint32_t width,
                    std::uint32_t height,
                    bitDepth_t depth,
                    const std::string &colorSpace,
                    std::uint32_t highBit);

            ~image();


            /// \brief Create the image.
            ///
            /// An image with the specified size (in pixels), bit depth
            ///  and color space is allocated.
            /// The number of channels to allocate is automatically
            ///  calculated using the colorSpace parameter.
            ///
            /// @param width    the image's width, in pixels.
            /// @param height    the image's height, in pixels.
            /// @param depth    the size of a single color's component.
            /// @param colorSpace The color space as defined by the
            ///                 DICOM standard.
            ///                 Valid colorspace are:
            ///                 - "RGB"
            ///                 - "YBR_FULL"
            ///                 - "YBR_PARTIAL"
            ///                 - "YBR_RCT" (Not yet supported)
            ///                 - "YBR_ICT" (Not yet supported)
            ///                 - "PALETTE COLOR"
            ///                 - "MONOCHROME2"
            ///                 - "MONOCHROME1"
            /// @param highBit  the highest bit used for integer
            ///                  values.
            /// @return         the data handler containing the image's
            ///                  data
            ///
            ///////////////////////////////////////////////////////////
            std::shared_ptr<handlers::writingDataHandlerNumericBase> create(
                    const std::uint32_t width,
                    const std::uint32_t height,
                    const bitDepth_t depth,
                    const std::string &colorSpace,
                    const std::uint32_t highBit);

            /// \brief Set the palette for the image
            ///
            /// @param imagePalette  the palette used in the image
            ///
            ///////////////////////////////////////////////////////////
            void setPalette(std::shared_ptr<palette> imagePalette);

            /// \brief Get the image's size, in pixels.
            ///
            /// @param pWidth a pointer to the variable to fill with
            ///               the image's width (in pixels).
            /// @param pHeight a pointer to the variable to fill with
            ///               the image's height (in pixels).
            ///
            ///////////////////////////////////////////////////////////
            void getSize(std::uint32_t *pWidth, std::uint32_t *pHeight) const;

            /// \brief Retrieve a data handler for managing the
            ///        image's buffer
            ///
            /// The retrieved data handler gives access to the image's
            ///  buffer.
            /// The image's buffer stores the data in the following
            ///  format:
            /// - when multiple channels are present, then the channels
            ///   are ALWAYS interleaved
            /// - the channels are NEVER subsampled or oversampled.
            ///   The subsampling/oversampling is done by the codecs
            ///   when the image is stored or loaded from the dicom
            ///   structure.
            /// - the first stored value represent the first channel of
            ///   the top/left pixel.
            /// - each row is stored countiguously, from the top to the
            ///   bottom.
            ///
            /// @param bWrite   true if the application wants to write
            ///                 into the buffer, false otherwise.
            /// @param pRowSize the function will fill the variable
            ///                 pointed by this parameter with
            ///                 the size of a single row, in bytes.
            /// @param pChannelPixelSize the function will fill the
            ///                 variable pointed by this parameter with
            ///                 the size of a single pixel,
            ///                 in bytes.
            /// @param pChannelsNumber  the function will fill the
            ///                 variable pointed by this parameter with
            ///                 the number of channels per pixel.
            /// @return a pointer to the data handler for the image's
            ///         buffer.
            ///
            ///////////////////////////////////////////////////////////
            std::shared_ptr<handlers::readingDataHandlerNumericBase> getReadingDataHandler() const;

            std::shared_ptr<handlers::writingDataHandlerNumericBase> getWritingDataHandler();

            /// \brief Get the image's color space (DICOM standard)
            ///
            /// @return a string with the image's color space
            ///
            ///////////////////////////////////////////////////////////
            std::string getColorSpace() const;




            /// \brief Get the number of allocated channels.
            ///
            /// @return the number of color channels in the image
            ///
            ///////////////////////////////////////////////////////////
            std::uint32_t getChannelsNumber() const;

            /// \brief Get the image's bit depth.
            ///
            /// The bit depth indicates the number of bits used to
            ///  store every single value.
            ///
            /// @return the bit depth.
            ///
            ///////////////////////////////////////////////////////////
            bitDepth_t getDepth() const;

            /// \brief Return true if the image's pixels contain signed
            ///        values.
            ///
            /// @return true if the image's pixels contain signed
            ///         values
            ///
            ///////////////////////////////////////////////////////////
            bool isSigned() const;

            /// \brief Get the high bit.
            ///
            /// @return the image's high bit
            ///
            ///////////////////////////////////////////////////////////
            std::uint32_t getHighBit() const;

            std::shared_ptr<palette> getPalette() const;





//            void setPlanarConfiguration(const std::string& ts, unsigned int pc){
//
//
//                    // precondition
//                    assert( pc == 0 || pc == 1 );
//                     m_planarConfiguration = pc;
//                    if( pc )
//                    {
//                        // LEADTOOLS_FLOWERS-8-MONO2-Uncompressed.dcm
//                        if(  m_sample_pixel != 3 ) // Please set PixelFormat first
//                        {
//                            // gdcmWarningMacro( "Cant have Planar Configuration in non RGB input. Discarding" );
//                            m_planarConfiguration = 0;
//                        }
//
////                        1.2.840.10008.1.2.4.50
////                        1.2.840.10008.1.2.4.51
////                        1.2.840.10008.1.2.4.52
////                        1.2.840.10008.1.2.4.53
//
////                        1.2.840.10008.1.2.4.55
////                        1.2.840.10008.1.2.4.57
////                        1.2.840.10008.1.2.4.70
////                        1.2.840.10008.1.2.4.80
//
////                        1.2.840.10008.1.2.4.81
////                        1.2.840.10008.1.2.4.90
////                        1.2.840.10008.1.2.4.91
////                        1.2.840.10008.1.2.4.94
////                        1.2.840.10008.1.2.5
//
//                        if(     ts == uidJPEGBaselineProcess1_1_2_840_10008_1_2_4_50   //   JPEGBaselineProcess1
//                             || ts ==uidJPEGExtendedProcess2_4_1_2_840_10008_1_2_4_51  //  TransferSyntax::JPEGExtendedProcess2_4
//                             || ts ==uidJPEGExtendedProcess3_5_1_2_840_10008_1_2_4_52  // TransferSyntax::JPEGExtendedProcess3_5
//                             || ts ==uidJPEGSpectralSelectionNonHierarchicalProcess6_8_1_2_840_10008_1_2_4_53 // TransferSyntax::JPEGSpectralSelectionProcess6_8
//                             || ts ==uidJPEGFullProgressionNonHierarchicalProcess10_12_1_2_840_10008_1_2_4_55 // TransferSyntax::JPEGFullProgressionProcess10_12
//                             || ts ==uidJPEGLosslessNonHierarchicalProcess14_1_2_840_10008_1_2_4_57 // TransferSyntax::JPEGLosslessProcess14
//                             || ts ==uidJPEGLosslessNonHierarchicalFirstOrderPredictionProcess14SelectionValue1_1_2_840_10008_1_2_4_70   // TransferSyntax::JPEGLosslessProcess14_1
//                             || ts == uidJPEGLSLosslessImageCompression_1_2_840_10008_1_2_4_80
//                             || ts ==uidJPEGLSLossyNearLosslessImageCompression_1_2_840_10008_1_2_4_81   //  TransferSyntax::JPEGLSNearLossless
//                             || ts == uidJPEG2000ImageCompressionLosslessOnly_1_2_840_10008_1_2_4_90  // TransferSyntax::JPEG2000Lossless
//                             || ts ==uidJPEG2000ImageCompression_1_2_840_10008_1_2_4_91   //  TransferSyntax::JPEG2000
//                             || ts == uidJPIPReferenced_1_2_840_10008_1_2_4_94           // TransferSyntax::JPIPReferenced
//                              || ts ==uidRLELossless_1_2_840_10008_1_2_5                 // TransferSyntax::RLELossless // FIXME internally GDCM produce per-pixel output
//                        )
//                        {
//                            // PS 3.6 - 2011 8.2.4 JPEG 2000 IMAGE COMPRESSION
//                            // The value of Planar Configuration (0028,0006) is irrelevant since the
//                            // manner of encoding components is specified in the JPEG 2000 standard,
//                            // hence it shall be set to 0.
//                            // By extension, this behavior has been applied also to JPEG and JPEG-LS
//
//                            m_planarConfiguration = 0;
//                        }
//                    }
//                    // \postcondition
//                    assert( m_planarConfiguration == 0 || m_planarConfiguration == 1 );
//            }
//
//            uint32_t   getPlanarConfiguration() const {
//                return  m_planarConfiguration;
//            }






        protected:
            // Image's buffer
            ///////////////////////////////////////////////////////////
            std::shared_ptr<buffer> m_buffer;

            tagVR_t m_bufferDataType;

            // Number of channels
            ///////////////////////////////////////////////////////////
            std::uint32_t m_channelsNumber;

            // Color space
            ///////////////////////////////////////////////////////////
            std::string m_colorSpace;

            // Depth (enum)
            ///////////////////////////////////////////////////////////
            bitDepth_t m_imageDepth;

            // High bit (not valid in float mode)
            ///////////////////////////////////////////////////////////
            std::uint32_t m_highBit;

            // Image's size in pixels
            ///////////////////////////////////////////////////////////
            std::uint32_t m_width;
            std::uint32_t m_height;

            // Image's lut (only if the colorspace is PALETTECOLOR
            ///////////////////////////////////////////////////////////
            std::shared_ptr<palette> m_palette;

        };


/// @}

    } // namespace implementation

} // namespace imebra

#endif // !defined(imebraImage_A807A3CA_FA04_44f4_85D2_C7AA2FE103C4__INCLUDED_)
