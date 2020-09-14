/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.12
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.imebra;

public class CMoveCommand extends DimseCommand {
  private transient long swigCPtr;

  protected CMoveCommand(long cPtr, boolean cMemoryOwn) {
    super(imebraJNI.CMoveCommand_SWIGUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(CMoveCommand obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        imebraJNI.delete_CMoveCommand(swigCPtr);
      }
      swigCPtr = 0;
    }
    super.delete();
  }

  public CMoveCommand(String abstractSyntax, int messageID, dimseCommandPriority_t priority, String affectedSopClassUid, String destinationAET, DataSet identifier) {
    this(imebraJNI.new_CMoveCommand__SWIG_0(abstractSyntax, messageID, priority.swigValue(), affectedSopClassUid, destinationAET, DataSet.getCPtr(identifier), identifier), true);
  }

  public CMoveCommand(CMoveCommand source) {
    this(imebraJNI.new_CMoveCommand__SWIG_1(CMoveCommand.getCPtr(source), source), true);
  }

  public String getDestinationAET() {
    return imebraJNI.CMoveCommand_getDestinationAET(swigCPtr, this);
  }

}
