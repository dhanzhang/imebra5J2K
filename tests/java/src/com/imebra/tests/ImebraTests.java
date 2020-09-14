package com.imebra.tests;


import com.imebra.*;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.UnsupportedEncodingException;

import static junit.framework.TestCase.assertEquals;

public class ImebraTests {

    @BeforeClass
    public static void SetupTest() throws Exception
    {
        System.loadLibrary("imebrajni");
    }

    @Test
    public void testUnicode()
    {
        byte patientName0Bytes[]= new byte[28];
        patientName0Bytes[0] = 0;
        patientName0Bytes[1] = 0;
        patientName0Bytes[2] = 0x06;
        patientName0Bytes[3] = 0x28;
        patientName0Bytes[4] = 0;
        patientName0Bytes[5] = 0;
        patientName0Bytes[6] = 0x06;
        patientName0Bytes[7] = 0x2a;
        patientName0Bytes[8] = 0;
        patientName0Bytes[9] = 0;
        patientName0Bytes[10] = 0x06;
        patientName0Bytes[11] = 0x2b;
        patientName0Bytes[12] = 0;
        patientName0Bytes[13] = 0;
        patientName0Bytes[14] = 0;
        patientName0Bytes[15] = 0x40;
        patientName0Bytes[16] = 0;
        patientName0Bytes[17] = 0;
        patientName0Bytes[18] = 0;
        patientName0Bytes[19] = 0x41;
        patientName0Bytes[20] = 0;
        patientName0Bytes[21] = 0;
        patientName0Bytes[22] = 0;
        patientName0Bytes[23] = 0x42;
        patientName0Bytes[24] = 0;
        patientName0Bytes[25] = 0;
        patientName0Bytes[26] = 0;
        patientName0Bytes[27] = 0x43;

        String patientName0 = null;
        try {
            patientName0 = new String(patientName0Bytes, "UTF-32BE");

        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        String patientName1 = new String("\u0420\u062a\u062b^\u0410\u0628\u062a");

        com.imebra.ReadWriteMemory streamMemory = new com.imebra.ReadWriteMemory();
        {
            com.imebra.FileParts charsetsList = new FileParts();
            charsetsList.add("ISO_IR 6");
            MutableDataSet testDataSet = new com.imebra.MutableDataSet("1.2.840.10008.1.2.1", charsetsList);

            {
                WritingDataHandler handler = testDataSet.getWritingDataHandler(new TagId(0x10, 0x10), 0);

                handler.setUnicodeString(0, patientName0);
                handler.setUnicodeString(1, patientName1);

                handler.delete();
            }

            MemoryStreamOutput writeStream = new MemoryStreamOutput(streamMemory);
            StreamWriter writer = new StreamWriter(writeStream);
            CodecFactory.save(testDataSet, writer, codecType_t.dicom);
        }

        {
            MemoryStreamInput readStream = new MemoryStreamInput(streamMemory);
            StreamReader reader = new StreamReader(readStream);
            DataSet testDataSet = CodecFactory.load(reader);

            assertEquals(patientName0, testDataSet.getUnicodeString(new TagId(0x0010, 0x0010), 0));
            assertEquals(patientName1, testDataSet.getUnicodeString(new TagId(0x0010, 0x0010), 1));
        }
    }

    public class SCPThread extends Thread
    {
        public void run() {
            try
            {
                PresentationContext context = new PresentationContext("1.2.840.10008.1.1");
                context.addTransferSyntax("1.2.840.10008.1.2");
                PresentationContexts presentationContexts = new PresentationContexts();
                presentationContexts.addPresentationContext(context);

                TCPListener listener = new TCPListener(new TCPPassiveAddress("", "20002"));

                TCPStream stream = listener.waitForConnection();

                AssociationSCP scp = new AssociationSCP("SCP", 1, 1, presentationContexts, new StreamReader(stream), new StreamWriter(stream), 0, 10);

                DimseService dimseService = new DimseService(scp);

                CStoreCommand command = (CStoreCommand)dimseService.getCommand();

                dimseService.sendCommandOrResponse(new CStoreResponse(command, dimseStatusCode_t.success));

            }
            catch(Exception e)
            {
                System.out.println(e.getMessage());

            }

        }
    }

    @Test
    public void testStorageSCU()
    {
        Thread scpThread = new SCPThread();
        scpThread.start();

        try {
            Thread.sleep(10000);

            TCPStream stream = new TCPStream(new TCPActiveAddress("", "20002"));

            StreamReader readSCU = new StreamReader(stream);
            StreamWriter writeSCU = new StreamWriter(stream);

            PresentationContext context = new PresentationContext("1.2.840.10008.1.1");
            context.addTransferSyntax("1.2.840.10008.1.2");
            PresentationContexts presentationContexts = new PresentationContexts();
            presentationContexts.addPresentationContext(context);

            AssociationSCU scu = new AssociationSCU("SCU", "SCP", 1, 1, presentationContexts, readSCU, writeSCU, 0);

            DimseService dimse = new DimseService(scu);

            DataSet payload = new DataSet ("1.2.840.10008.1.2");
            payload.setString(new TagId(0x0008, 0x0016), "1.1.1.1.1");
            payload.setString(new TagId(0x0008, 0x0018), "1.1.1.1.2");
            payload.setString(new TagId(0x0010, 0x0010), "Test^Patient");
            CStoreCommand storeCommand = new CStoreCommand(
                    "1.2.840.10008.1.1",
                    dimse.getNextCommandID(),
                    dimseCommandPriority_t.medium,
                    payload.getString(new TagId(0x0008, 0x0016), 0),
                    payload.getString(new TagId(0x0008, 0x0018), 0),
                    "Origin",
                    0,
                    payload);
            dimse.sendCommandOrResponse(storeCommand);
            CStoreResponse response = dimse.getCStoreResponse(storeCommand);

            scpThread.join();
        }
        catch(Exception e)
        {
            System.out.println(e.getMessage());

        }

    }


}
