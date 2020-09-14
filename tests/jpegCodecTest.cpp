#include <imebra/imebra.h>
#include <gtest/gtest.h>
#include <thread>
#include "buildImageForTest.h"
#include <iostream>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <list>
//#include "gdcm-3.0/gdcmImageReader.h"
//#include "gdcm-3.0/gdcmReader.h"
//#include "gdcm-3.0/gdcmImage.h"
//#include "gdcm-3.0/gdcmFile.h"
namespace imebra {

    namespace tests {

// A buffer initialized to a default data type should use the data type OB
        TEST(jpegCodecTest, testBaseline) {
            for (int precision = 0; precision != 2; ++precision) {
                std::uint32_t bits = precision == 0 ? 7 : 11;
                std::cout << "Testing baseline jpeg (" << (bits + 1) << " bits)" << std::endl;

                std::string transferSyntax = precision == 0 ? "1.2.840.10008.1.2.4.50" : "1.2.840.10008.1.2.4.51";
                MutableDataSet dataset(transferSyntax);

                std::uint32_t width = 600;
                std::uint32_t height = 400;

                Image baselineImage = buildImageForTest(width, height,
                                                        precision == 0 ? bitDepth_t::depthU8 : bitDepth_t::depthU16,
                                                        bits, "RGB", 50);

                Transform transformToYBR = ColorTransformsFactory::getTransform("RGB", "YBR_FULL");
                MutableImage ybrImage = transformToYBR.allocateOutputImage(baselineImage, width, height);
                transformToYBR.runTransform(baselineImage, 0, 0, width, height, ybrImage, 0, 0);

                dataset.setImage(0, ybrImage, imageQuality_t::veryHigh);

                Image checkImage = dataset.getImage(0);

                std::uint32_t checkWidth(checkImage.getWidth()), checkHeight(checkImage.getHeight());
                Transform transformToRGB = ColorTransformsFactory::getTransform("YBR_FULL", "RGB");
                MutableImage rgbImage = transformToRGB.allocateOutputImage(checkImage, checkWidth, checkHeight);
                transformToRGB.runTransform(checkImage, 0, 0, checkWidth, checkHeight, rgbImage, 0, 0);

                // Compare the buffers. A little difference is allowed
                double differenceRGB = compareImages(baselineImage, rgbImage);
                double differenceYBR = compareImages(ybrImage, checkImage);
                ASSERT_LE(differenceRGB, 5);
                ASSERT_LE(differenceYBR, 1);

                // Save jpeg, reload jpeg and check
                MutableMemory memory;
                {
                    MemoryStreamOutput streamOutput(memory);
                    StreamWriter writer(streamOutput);
                    CodecFactory::save(dataset, writer, codecType_t::jpeg);
                }
                MemoryStreamInput streamInput(memory);
                StreamReader reader(streamInput);
                DataSet checkDataSet(CodecFactory::load(reader));
                Image checkJpegImage(checkDataSet.getImage(0));
                double differenceJPG = compareImages(ybrImage, checkJpegImage);
                ASSERT_LE(differenceJPG, 1);

            }
        }


        TEST(jpegCodecTest, testBaselineSubsampled) {
            for (int subsampledX = 0; subsampledX != 2; ++subsampledX) {
                for (int subsampledY = 0; subsampledY != 2; ++subsampledY) {
                    for (int interleaved = 0; interleaved != 2; ++interleaved) {
                        for (int prematureEoi(0); prematureEoi != 2; ++prematureEoi) {
                            std::uint32_t width = 300;
                            std::uint32_t height = 200;
                            Image baselineImage = buildSubsampledImage(width, height, bitDepth_t::depthU8, 7, "RGB");

                            Transform colorTransform = ColorTransformsFactory::getTransform("RGB", "YBR_FULL");
                            MutableImage ybrImage = colorTransform.allocateOutputImage(baselineImage, width, height);
                            colorTransform.runTransform(baselineImage, 0, 0, width, height, ybrImage, 0, 0);

                            MutableMemory savedJpeg;
                            {
                                MemoryStreamOutput saveStream(savedJpeg);
                                StreamWriter writer(saveStream);

                                CodecFactory::saveImage(writer, ybrImage, "1.2.840.10008.1.2.4.50",
                                                        imageQuality_t::veryHigh, 8, subsampledX != 0, subsampledY != 0,
                                                        interleaved != 0, false);
                            }
                            if (prematureEoi == 1) {
                                // Insert a premature EOI tag
                                /////////////////////////////
                                size_t dataSize;
                                std::uint8_t *pData = reinterpret_cast<std::uint8_t *>(savedJpeg.data(&dataSize));
                                pData[dataSize - 10] = 0xff;
                                pData[dataSize - 9] = 0xd9;
                            }

                            // Insert an unknown tag immediately after the jpeg signature
                            MutableMemory savedJpegUnknownTag(savedJpeg.size() + 128);
                            size_t dummy;
                            ::memcpy(savedJpegUnknownTag.data(&dummy), savedJpeg.data(&dummy), 2);
                            ::memcpy(savedJpegUnknownTag.data(&dummy) + 130, savedJpeg.data(&dummy) + 2,
                                     savedJpeg.size() - 2);
                            savedJpegUnknownTag.data(&dummy)[2] = static_cast<char>(0xff);
                            savedJpegUnknownTag.data(&dummy)[3] = 0x10;
                            savedJpegUnknownTag.data(&dummy)[4] = 0;
                            savedJpegUnknownTag.data(&dummy)[5] = 124;

                            MemoryStreamInput loadStream(savedJpegUnknownTag);
                            StreamReader reader(loadStream);

                            DataSet readDataSet = CodecFactory::load(reader, 0xffff);

                            try {
                                Image checkImage = readDataSet.getImage(0);
                                ASSERT_TRUE(prematureEoi == 0);

                                std::uint32_t checkWidth(checkImage.getWidth()), checkHeight(checkImage.getHeight());
                                Transform transformToRGB = ColorTransformsFactory::getTransform("YBR_FULL", "RGB");
                                MutableImage rgbImage = transformToRGB.allocateOutputImage(checkImage, checkWidth,
                                                                                           checkHeight);
                                transformToRGB.runTransform(checkImage, 0, 0, checkWidth, checkHeight, rgbImage, 0, 0);

                                // Compare the buffers. A little difference is allowed
                                double differenceRGB = compareImages(baselineImage, rgbImage);
                                double differenceYBR = compareImages(ybrImage, checkImage);
                                ASSERT_LE(differenceRGB, (1 + subsampledX) * (1 + subsampledY) * 50);
                                ASSERT_LE(differenceYBR, (1 + subsampledX) * (1 + subsampledY) * 25);
                            }
                            catch (const CodecCorruptedFileError &) {
                                ASSERT_TRUE(prematureEoi == 1);
                            }

                        }
                    }
                }
            }
        }


        TEST(jpegCodecTest, testLossless) {
            for (int interleaved = 0; interleaved != 2; ++interleaved) {
                for (std::uint32_t bits = 8; bits <= 16; bits += 8) {
                    for (int firstOrderPrediction = 0; firstOrderPrediction != 2; ++firstOrderPrediction) {
                        for (int b2Complement = 0; b2Complement != 2; ++b2Complement) {
                            for (int colorSpace(0); colorSpace != 2; ++colorSpace) {
                                std::cout <<
                                          "Testing lossless jpeg (" << (bits) <<
                                          " bits, interleaved=" << interleaved <<
                                          ", firstOrderPrediction=" << firstOrderPrediction <<
                                          ", 2complement=" << b2Complement <<
                                          ", colorSpace=" << (colorSpace == 0 ? "RGB" : "MONOCHROME2") <<
                                          ")" << std::endl;

                                std::string transferSyntax = (firstOrderPrediction == 0) ? "1.2.840.10008.1.2.4.57"
                                                                                         : "1.2.840.10008.1.2.4.70";

                                std::uint32_t width = 115;
                                std::uint32_t height = 400;

                                bitDepth_t depth;
                                if (bits <= 8) {
                                    depth = (b2Complement == 1) ? bitDepth_t::depthS8 : bitDepth_t::depthU8;
                                } else {
                                    depth = (b2Complement == 1) ? bitDepth_t::depthS16 : bitDepth_t::depthU16;
                                }

                                Image image = buildImageForTest(width, height, depth, bits - 1,
                                                                colorSpace == 0 ? "RGB" : "MONOCHROME2", 50);


                                MutableMemory savedJpeg;
                                {
                                    MutableDataSet dataSet(transferSyntax);
                                    dataSet.setImage(0, image, imageQuality_t::veryHigh);

                                    MemoryStreamOutput saveStream(savedJpeg);
                                    StreamWriter writer(saveStream);
                                    CodecFactory::save(dataSet, writer, codecType_t::dicom);
                                }

                                MemoryStreamInput loadStream(savedJpeg);
                                StreamReader reader(loadStream);

                                DataSet readDataSet = CodecFactory::load(reader, 0xffff);

                                Image checkImage = readDataSet.getImage(0);

                                // Compare the buffers
                                double difference = compareImages(image, checkImage);
                                ASSERT_DOUBLE_EQ(0.0, difference);
                            }
                        }
                    }
                }
            }
        }


        void feedJpegDataThread(PipeStream &source, DataSet &dataSet) {
            StreamWriter writer(source.getStreamOutput());

            CodecFactory::save(dataSet, writer, codecType_t::jpeg);

            source.close(5000);
        }

        void CopyGroups(const DataSet &source, MutableDataSet &destination) {
            tagsIds_t tags(source.getTags());

            for (tagsIds_t::const_iterator scanTags(tags.begin()), endTags(tags.end());
                 scanTags != endTags; ++scanTags) {
                if ((*scanTags).getGroupId() == 0x7fe0) {
                    continue;
                }
//                uint16_t   grpid =(*scanTags).getGroupId()  ;
//                uint16_t   tagid= (*scanTags).getTagId();
//                if( grpid  == 0x0028
//                &&  (   tagid  == 0x1101
//                     || tagid  == 0x1102  || tagid  == 0x1103 ||   tagid  == 0x1199
//                     || tagid ==  0x1201 ||    tagid ==0x1202  ||    tagid == 0x1203
//                )){
//                    continue;
//                }

//                        (0028,1101) [US] RedPaletteColorLookupTableDescriptor: 256\0\16
//                        (0028,1102) [US] GreenPaletteColorLookupTableDescriptor: 256\0\16
//                        (0028,1103) [US] BluePaletteColorLookupTableDescriptor: 256\0\16
//                        (0028,1199) [UI] PaletteColorLookupTableUID: 999.999.389972238
//
//                (0028,1201) [OW] RedPaletteColorLookupTableData: binary data
//                (0028,1202) [OW] GreenPaletteColorLookupTableData: binary data
//                (0028,1203) [OW] BluePaletteColorLookupTableData: binary data

                try {
                    destination.getTag(*scanTags);
                }
                catch (const MissingDataElementError &) {
                    Tag sourceTag = source.getTag(*scanTags);
                    MutableTag destTag = destination.getTagCreate(*scanTags, sourceTag.getDataType());

                    try {
                        for (size_t buffer(0);; ++buffer) {
                            try {
                                DataSet sequence = sourceTag.getSequenceItem(buffer);
                                MutableDataSet destSequence = destination.appendSequenceItem(*scanTags);
                                CopyGroups(sequence, destSequence);
                            }
                            catch (const MissingDataElementError &) {
                                ReadingDataHandler sourceHandler = sourceTag.getReadingDataHandler(buffer);
                                WritingDataHandler destHandler = destTag.getWritingDataHandler(buffer);
                                destHandler.setSize(sourceHandler.getSize());
                                for (size_t item(0); item != sourceHandler.getSize(); ++item) {
                                    destHandler.setUnicodeString(item, sourceHandler.getUnicodeString(item));
                                }
                            }
                        }
                    }
                    catch (const MissingDataElementError &) {

                    }
                }
            }
        }


        int readFileList(const char *basePath, std::list<std::string> &lst) {
            DIR *dir;
            struct dirent *ptr;
            char base[1000];

            if ((dir = opendir(basePath)) == NULL) {
                perror("Open dir error...");
                exit(1);
            }

            while ((ptr = readdir(dir)) != NULL) {
                if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)    ///current dir OR parrent dir
                    continue;
                else if (ptr->d_type == 8) {
                    ///file
                    char State1[1024] = {0};
                    sprintf(State1, "%s/%s", basePath, ptr->d_name);
                    lst.push_back(State1);
                } else if (ptr->d_type == 10) { ///link file
                    printf("d_name:%s/%s\n", basePath, ptr->d_name);
                } else if (ptr->d_type == 4)    ///dir
                {
                    memset(base, '\0', sizeof(base));
                    strcpy(base, basePath);
                    strcat(base, "/");
                    strcat(base, ptr->d_name);
                    readFileList(base, lst);
                }
            }
            closedir(dir);
            return 1;
        }


        TEST(jpegCodecTest, codecFactoryPipe) {
            MutableDataSet testDataSet("1.2.840.10008.1.2.4.50");

            const std::uint32_t width = 600;
            const std::uint32_t height = 400;

            Image baselineImage(buildImageForTest(width, height, bitDepth_t::depthU8, 7, "RGB", 50));

            Transform transformToYBR(ColorTransformsFactory::getTransform("RGB", "YBR_FULL"));
            MutableImage ybrImage(transformToYBR.allocateOutputImage(baselineImage, width, height));
            transformToYBR.runTransform(baselineImage, 0, 0, width, height, ybrImage, 0, 0);
            testDataSet.setImage(0, ybrImage, imageQuality_t::veryHigh);

            PipeStream source(1024);

            std::thread feedData(imebra::tests::feedJpegDataThread, std::ref(source), std::ref(testDataSet));

            StreamReader reader(source.getStreamInput());
            DataSet loadedDataSet(CodecFactory::load(reader));

            feedData.join();

            Image checkImage(loadedDataSet.getImage(0));

            std::uint32_t checkWidth(checkImage.getWidth()), checkHeight(checkImage.getHeight());
            Transform transformToRGB(ColorTransformsFactory::getTransform("YBR_FULL", "RGB"));
            MutableImage rgbImage(transformToRGB.allocateOutputImage(checkImage, checkWidth, checkHeight));
            transformToRGB.runTransform(checkImage, 0, 0, checkWidth, checkHeight, rgbImage, 0, 0);

            // Compare the buffers. A little difference is allowed
            double differenceRGB = compareImages(baselineImage, rgbImage);
            double differenceYBR = compareImages(ybrImage, checkImage);
            ASSERT_LE(differenceRGB, 5);
            ASSERT_LE(differenceYBR, 1);
        }

        TEST(jpeg2000CodecBuildTest, codecFactoryPipe) {


            const std::uint32_t width = 600;
            const std::uint32_t height = 400;
            Image baselineImage = buildImageForTest(width, height, bitDepth_t::depthU32, 31, "RGB", 50);
            Image j2kImage = buildImageForTest(width, height, bitDepth_t::depthU32, 31, "RGB", 50);
            ASSERT_EQ(baselineImage.getHeight(), j2kImage.getHeight());
            ASSERT_EQ(baselineImage.getWidth(), j2kImage.getWidth());
            ASSERT_EQ(baselineImage.getHighBit(), j2kImage.getHighBit());
            ASSERT_EQ(baselineImage.getChannelsNumber(), j2kImage.getChannelsNumber());
            ASSERT_EQ(baselineImage.getColorSpace(), j2kImage.getColorSpace());
            ASSERT_EQ(baselineImage.getDepth(), j2kImage.getDepth());
            imebra::ReadingDataHandlerNumeric br = baselineImage.getReadingDataHandler();
            imebra::ReadingDataHandlerNumeric kr = j2kImage.getReadingDataHandler();

            ASSERT_EQ(br.getSize(), kr.getSize());
            ASSERT_EQ(br.getMemory().size(), kr.getMemory().size());

            size_t k1, k2;
            const char *dp1 = br.data(&k1);
            const char *dp2 = kr.data(&k2);
            ASSERT_EQ(k1, k2);
            for (size_t i = 0; i < k1; i++) {
                ASSERT_EQ(dp1[i], dp2[i]);
            }
        }

        TEST(jpeg2000CodecDepthU8Test, codecFactoryPipe) {

            char name[] = "j2k_XXXXXX";
            int fd = mkstemp(name);
            printf("name:%s\n", name);
            close(fd);

            char outname[] = "j2k_XXXXXX";
            fd = mkstemp(outname);
            printf("outname:%s\n", outname);
            close(fd);

            const std::uint32_t width = 400;
            const std::uint32_t height = 400;
            Image baselineImage = buildImageForTest(width, height, bitDepth_t::depthU8, 7, "RGB", 50);

            MutableDataSet loadedDataSet(imebra::uidExplicitVRLittleEndian_1_2_840_10008_1_2_1);
            loadedDataSet.setImage(0, baselineImage, imageQuality_t::high);
            CodecFactory::save(loadedDataSet, name, codecType_t::dicom);

            // Now we create a new dataset and copy the tags and images from the loaded dataset
            MutableDataSet newDataSet(imebra::uidJPEG2000ImageCompressionLosslessOnly_1_2_840_10008_1_2_4_90);
            newDataSet.setImage(0, baselineImage, imageQuality_t::high);
            CodecFactory::save(newDataSet, outname, codecType_t::dicom);

            DataSet d1 = CodecFactory::load(name);
            DataSet d2 = CodecFactory::load(outname);
            Image a1 = d1.getImage(0);
            Image a2 = d2.getImage(0);
            ASSERT_EQ(a1.getHeight(), a2.getHeight());
            ASSERT_EQ(a1.getWidth(), a2.getWidth());
            ASSERT_EQ(a1.getHighBit(), a2.getHighBit());
            ASSERT_EQ(a1.getChannelsNumber(), a2.getChannelsNumber());
            ASSERT_EQ(a1.getColorSpace(), a2.getColorSpace());
            ASSERT_EQ(a1.getDepth(), a2.getDepth());

            ReadingDataHandlerNumeric r1 = a1.getReadingDataHandler();
            ReadingDataHandlerNumeric r2 = a2.getReadingDataHandler();
            size_t k1, k2;
            const char *p1 = r1.data(&k1);
            const char *p2 = r2.data(&k2);

            ASSERT_EQ(k1, k2);

            for (size_t ip = 0; ip < k1; ip++) {
                ASSERT_EQ(p1[ip], p2[ip]);
            }


        }

        TEST(jpeg2000CodecDepthU16Test, codecFactoryPipe) {

            char name[] = "j2k_XXXXXX";
            int fd = mkstemp(name);
            printf("name:%s\n", name);
            close(fd);

            char outname[] = "j2k_XXXXXX";
            fd = mkstemp(outname);
            printf("outname:%s\n", outname);
            close(fd);

            const std::uint32_t width = 600;
            const std::uint32_t height = 400;
            Image baselineImage = buildImageForTest(width, height, bitDepth_t::depthU16, 15, "RGB", 50);

            MutableDataSet loadedDataSet(imebra::uidExplicitVRLittleEndian_1_2_840_10008_1_2_1);
            loadedDataSet.setImage(0, baselineImage, imageQuality_t::high);
            CodecFactory::save(loadedDataSet, name, codecType_t::dicom);

            // Now we create a new dataset and copy the tags and images from the loaded dataset
            MutableDataSet newDataSet(imebra::uidJPEG2000ImageCompressionLosslessOnly_1_2_840_10008_1_2_4_90);
            newDataSet.setImage(0, baselineImage, imageQuality_t::high);
            CodecFactory::save(newDataSet, outname, codecType_t::dicom);

            DataSet d1 = CodecFactory::load(name);
            DataSet d2 = CodecFactory::load(outname);
            Image a1 = d1.getImage(0);
            Image a2 = d2.getImage(0);
            ASSERT_EQ(a1.getHeight(), a2.getHeight());
            ASSERT_EQ(a1.getWidth(), a2.getWidth());
            ASSERT_EQ(a1.getHighBit(), a2.getHighBit());
            ASSERT_EQ(a1.getChannelsNumber(), a2.getChannelsNumber());
            ASSERT_EQ(a1.getColorSpace(), a2.getColorSpace());
            ASSERT_EQ(a1.getDepth(), a2.getDepth());

            ReadingDataHandlerNumeric r1 = a1.getReadingDataHandler();
            ReadingDataHandlerNumeric r2 = a2.getReadingDataHandler();
            size_t k1, k2;
            const char *p1 = r1.data(&k1);
            const char *p2 = r2.data(&k2);

            ASSERT_EQ(k1, k2);

            for (size_t ip = 0; ip < k1; ip++) {
                ASSERT_EQ(p1[ip], p2[ip]);
            }


        }

        TEST(jpeg2000CodecDepthS16Test, codecFactoryPipe) {

            char name[] = "j2k_XXXXXX";
            int fd = mkstemp(name);
            printf("name:%s\n", name);
            close(fd);

            char outname[] = "j2k_XXXXXX";
            fd = mkstemp(outname);
            printf("outname:%s\n", outname);
            close(fd);

            const std::uint32_t width = 600;
            const std::uint32_t height = 400;
            Image baselineImage = buildImageForTest(width, height, bitDepth_t::depthS16, 15, "RGB", 50);

            MutableDataSet loadedDataSet(imebra::uidExplicitVRLittleEndian_1_2_840_10008_1_2_1);
            loadedDataSet.setImage(0, baselineImage, imageQuality_t::high);
            CodecFactory::save(loadedDataSet, name, codecType_t::dicom);

            // Now we create a new dataset and copy the tags and images from the loaded dataset
            MutableDataSet newDataSet(imebra::uidJPEG2000ImageCompressionLosslessOnly_1_2_840_10008_1_2_4_90);
            newDataSet.setImage(0, baselineImage, imageQuality_t::high);
            CodecFactory::save(newDataSet, outname, codecType_t::dicom);

            DataSet d1 = CodecFactory::load(name);
            DataSet d2 = CodecFactory::load(outname);
            Image a1 = d1.getImage(0);
            Image a2 = d2.getImage(0);
            ASSERT_EQ(a1.getHeight(), a2.getHeight());
            ASSERT_EQ(a1.getWidth(), a2.getWidth());
            ASSERT_EQ(a1.getHighBit(), a2.getHighBit());
            ASSERT_EQ(a1.getChannelsNumber(), a2.getChannelsNumber());
            ASSERT_EQ(a1.getColorSpace(), a2.getColorSpace());
            ASSERT_EQ(a1.getDepth(), a2.getDepth());

            ReadingDataHandlerNumeric r1 = a1.getReadingDataHandler();
            ReadingDataHandlerNumeric r2 = a2.getReadingDataHandler();
            size_t k1, k2;
            const char *p1 = r1.data(&k1);
            const char *p2 = r2.data(&k2);

            ASSERT_EQ(k1, k2);

            for (size_t ip = 0; ip < k1; ip++) {
                ASSERT_EQ(p1[ip], p2[ip]);
            }


        }

        TEST(jpeg2000CodecDepthS8Test, codecFactoryPipe) {

            char name[] = "j2k_XXXXXX";
            int fd = mkstemp(name);
            printf("name:%s\n", name);
            close(fd);

            char outname[] = "j2k_XXXXXX";
            fd = mkstemp(outname);
            printf("outname:%s\n", outname);
            close(fd);

            const std::uint32_t width = 600;
            const std::uint32_t height = 400;
            Image baselineImage = buildImageForTest(width, height, bitDepth_t::depthS8, 7, "RGB", 50);

            MutableDataSet loadedDataSet(imebra::uidExplicitVRLittleEndian_1_2_840_10008_1_2_1);
            loadedDataSet.setImage(0, baselineImage, imageQuality_t::high);
            CodecFactory::save(loadedDataSet, name, codecType_t::dicom);

            // Now we create a new dataset and copy the tags and images from the loaded dataset
            MutableDataSet newDataSet(imebra::uidJPEG2000ImageCompressionLosslessOnly_1_2_840_10008_1_2_4_90);
            newDataSet.setImage(0, baselineImage, imageQuality_t::high);
            CodecFactory::save(newDataSet, outname, codecType_t::dicom);

            DataSet d1 = CodecFactory::load(name);
            DataSet d2 = CodecFactory::load(outname);
            Image a1 = d1.getImage(0);
            Image a2 = d2.getImage(0);
            ASSERT_EQ(a1.getHeight(), a2.getHeight());
            ASSERT_EQ(a1.getWidth(), a2.getWidth());
            ASSERT_EQ(a1.getHighBit(), a2.getHighBit());
            ASSERT_EQ(a1.getChannelsNumber(), a2.getChannelsNumber());
            ASSERT_EQ(a1.getColorSpace(), a2.getColorSpace());
            ASSERT_EQ(a1.getDepth(), a2.getDepth());

            ReadingDataHandlerNumeric r1 = a1.getReadingDataHandler();
            ReadingDataHandlerNumeric r2 = a2.getReadingDataHandler();
            size_t k1, k2;
            const char *p1 = r1.data(&k1);
            const char *p2 = r2.data(&k2);

            ASSERT_EQ(k1, k2);

            for (size_t ip = 0; ip < k1; ip++) {
                ASSERT_EQ(p1[ip], p2[ip]);
            }


        }


        TEST(jpeg2000CodecTest, codecFactoryPipe) {


            const char *basePath = "dcmfiles";//遍历文件夹下的所有文件
            std::list<std::string> files;
            readFileList(basePath, files);
            std::list<std::string>::iterator bg;
            for (bg = files.begin(); bg != files.end(); ++bg) {
                std::string df = *bg;
                printf("%s\n",df.c_str());
                char name[] = "j2k_XXXXXX";
                int fd = mkstemp(name);
                close(fd);
                DataSet rgb = CodecFactory::load(df, 4096);
                tagsIds_t tags(rgb.getTags());
                uint32_t   sb = rgb.getUint32(TagId(tagId_t::SamplesPerPixel_0028_0002),0, -1);
                if(!( sb == 1  || sb == 3)){
                    printf("ErrorFILE:%s\n", df.c_str());
                    remove(name);
                    continue;
                }

                size_t numFrm = rgb.getUint32(TagId(tagId_t::NumberOfFrames_0028_0008), 0, 1);
                {

                    MutableDataSet md(uidJPEG2000ImageCompressionLosslessOnly_1_2_840_10008_1_2_4_90);
                    for (size_t si = 0; si < numFrm; si++) {
                        Image a1 = rgb.getImage(si);
                        md.setImage(si, a1, imageQuality_t::high);
                    };
                    CopyGroups(rgb, md);
                    CodecFactory::save(md, name, codecType_t::dicom);

                }


                {
                    DataSet reload = CodecFactory::load(name, 4096);
                    size_t numFrm2 = reload.getUint32(TagId(tagId_t::NumberOfFrames_0028_0008), 0, 1);
                    ASSERT_EQ(numFrm2, numFrm);

                    for (size_t si = 0; si < numFrm; si++) {
                        Image a1 = rgb.getImage(si);
                        Image a2 = reload.getImage(si);
                        ASSERT_EQ(a1.getHeight(), a2.getHeight());
                        ASSERT_EQ(a1.getWidth(), a2.getWidth());
                        ASSERT_EQ(a1.getHighBit(), a2.getHighBit());
                        ASSERT_EQ(a1.getChannelsNumber(), a2.getChannelsNumber());
                        ASSERT_EQ(a1.getColorSpace(), a2.getColorSpace());
                        ASSERT_EQ(a1.getDepth(), a2.getDepth());

                        ReadingDataHandlerNumeric r1 = a1.getReadingDataHandler();
                        ReadingDataHandlerNumeric r2 = a2.getReadingDataHandler();
                        size_t k1, k2;
                        const char *p1 = r1.data(&k1);
                        const char *p2 = r2.data(&k2);

                        ASSERT_EQ(k1, k2);

                        for (size_t ip = 0; ip < k1; ip++) {
                            ASSERT_EQ(p1[ip], p2[ip]);
                        }

                    }

                }

                remove(name);

            }


        }

        TEST(jpeg2000CodecPLATest, codecFactoryPipe) {

            char name[] = "j2k_XXXXXX";

            int fd = mkstemp(name);
            close(fd);
            printf("%s\n", name);
            const char *basePath = "pla";//遍历文件夹下的所有文件
            std::list<std::string> files;
            readFileList(basePath, files);
            std::list<std::string>::iterator bg;
            for (bg = files.begin(); bg != files.end(); ++bg) {
                std::string df = *bg;
                DataSet rgb = CodecFactory::load(df, 4096);
                size_t numFrm = rgb.getUint32(TagId(tagId_t::NumberOfFrames_0028_0008), 0);
                MutableDataSet md(uidJPEG2000ImageCompressionLosslessOnly_1_2_840_10008_1_2_4_90);
                for (size_t si = 0; si < numFrm; si++) {
                    Image a1 = rgb.getImage(si);
                    md.setImage(si, a1, imageQuality_t::high);
                };
                CopyGroups(rgb, md);
                CodecFactory::save(md, name, codecType_t::dicom);
                DataSet reload = CodecFactory::load(name, 4096);
                size_t numFrm2 = reload.getUint32(TagId(tagId_t::NumberOfFrames_0028_0008), 0);
                ASSERT_EQ(numFrm2, numFrm);
                for (size_t si = 0; si < numFrm; si++) {
                    Image a1 = rgb.getImage(si);
                    Image a2 = reload.getImage(si);
                    ASSERT_EQ(a1.getHeight(), a2.getHeight());
                    ASSERT_EQ(a1.getWidth(), a2.getWidth());
                    ASSERT_EQ(a1.getHighBit(), a2.getHighBit());
                    ASSERT_EQ(a1.getChannelsNumber(), a2.getChannelsNumber());
                    ASSERT_EQ(a1.getColorSpace(), a2.getColorSpace());
                    ASSERT_EQ(a1.getDepth(), a2.getDepth());

                    ReadingDataHandlerNumeric r1 = a1.getReadingDataHandler();
                    ReadingDataHandlerNumeric r2 = a2.getReadingDataHandler();
                    size_t k1, k2;
                    const char *p1 = r1.data(&k1);
                    const char *p2 = r2.data(&k2);
                    ASSERT_EQ(k1, k2);
                    for (size_t ip = 0; ip < k1; ip++) {
                        ASSERT_EQ(p1[ip], p2[ip]);
                    }
                };
            }
            remove(name);
        }



        TEST(jpeg2000CodecGDCMTest, codecFactoryPipe) {


            const char *basePath = "gdcmdata";//遍历文件夹下的所有文件
            std::list<std::string> files;
            readFileList(basePath, files);
            std::list<std::string>::iterator bg;
            for (bg = files.begin(); bg != files.end(); ++bg) {
                std::string df = *bg;

                char name[] = "j2k_XXXXXX";
                int fd = mkstemp(name);
                close(fd);
                try {
                    DataSet rgb = CodecFactory::load(df, 4096);
                    tagsIds_t tags(rgb.getTags());
                    uint32_t   sb = rgb.getUint32(TagId(tagId_t::SamplesPerPixel_0028_0002),0, -1);
                    if(!( sb == 1  || sb == 3)){
                        printf("\r\n===============ErrorFILE:%s\n", df.c_str());
                        remove(name);
                        continue;
                    }

                    size_t numFrm = rgb.getUint32(TagId(tagId_t::NumberOfFrames_0028_0008), 0, 1);
                    {

                        MutableDataSet md(uidExplicitVRLittleEndian_1_2_840_10008_1_2_1);
                        for (size_t si = 0; si < numFrm; si++) {
                            Image a1 = rgb.getImage(si);
                            md.setImage(si, a1, imageQuality_t::high);
                        };
                        CopyGroups(rgb, md);
                        CodecFactory::save(md, name, codecType_t::dicom);

                    }


                    {
                        DataSet reload = CodecFactory::load(name, 4096);
                        size_t numFrm2 = reload.getUint32(TagId(tagId_t::NumberOfFrames_0028_0008), 0, 1);
                        ASSERT_EQ(numFrm2, numFrm);

                        for (size_t si = 0; si < numFrm; si++) {
                            Image a1 = rgb.getImage(si);
                            Image a2 = reload.getImage(si);
                            ASSERT_EQ(a1.getHeight(), a2.getHeight());
                            ASSERT_EQ(a1.getWidth(), a2.getWidth());
                            ASSERT_EQ(a1.getHighBit(), a2.getHighBit());
                            ASSERT_EQ(a1.getChannelsNumber(), a2.getChannelsNumber());
                            ASSERT_EQ(a1.getColorSpace(), a2.getColorSpace());
                            ASSERT_EQ(a1.getDepth(), a2.getDepth());

                            ReadingDataHandlerNumeric r1 = a1.getReadingDataHandler();
                            ReadingDataHandlerNumeric r2 = a2.getReadingDataHandler();
                            size_t k1, k2;
                            const char *p1 = r1.data(&k1);
                            const char *p2 = r2.data(&k2);

                            ASSERT_EQ(k1, k2);

                            for (size_t ip = 0; ip < k1; ip++) {
                                ASSERT_EQ(p1[ip], p2[ip]);
                            }

                        }

                    }

                }
                catch (std::exception& ex) {

                    printf("\r\n===============%s:%s\n", df.c_str(), ex.what());
                }

                remove(name);

            }


        }



//        TEST(gdcmReaderTest, codecFactoryPipe) {
//            const char *basePath = "gdcmdata";//遍历文件夹下的所有文件
//            std::list<std::string> files;
//            readFileList(basePath, files);
//            std::list<std::string>::iterator bg;
//            for (bg = files.begin(); bg != files.end(); ++bg) {
//                std::string df = *bg;
//              //  printf("deal:%s\n",df.c_str());
//                char name[] = "j2k_XXXXXX";
//                int fd = mkstemp(name);
//                close(fd);
//
//                gdcm::ImageReader reader;
//                reader.SetFileName( df.c_str() );
//                bool   brt =  reader.Read();
//                if(!brt){
//                    printf("=====errorFile:%s\n",df.c_str());
//                }
//
//                remove(name);
//
//            }
//
//
//        }

    } // namespace tests

} // namespace imebra
