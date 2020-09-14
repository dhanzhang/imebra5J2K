/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.12
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.imebra;

public class CMoveResponse extends CPartialResponse {
  private transient long swigCPtr;

  protected CMoveResponse(long cPtr, boolean cMemoryOwn) {
    super(imebraJNI.CMoveResponse_SWIGUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(CMoveResponse obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        imebraJNI.delete_CMoveResponse(swigCPtr);
      }
      swigCPtr = 0;
    }
    super.delete();
  }

  public CMoveResponse(CMoveCommand receivedCommand, dimseStatusCode_t responseCode, long remainingSubOperations, long completedSubOperations, long failedSubOperations, long warningSubOperations, DataSet identifier) {
    this(imebraJNI.new_CMoveResponse__SWIG_0(CMoveCommand.getCPtr(receivedCommand), receivedCommand, responseCode.swigValue(), remainingSubOperations, completedSubOperations, failedSubOperations, warningSubOperations, DataSet.getCPtr(identifier), identifier), true);
  }

  public CMoveResponse(CMoveCommand receivedCommand, dimseStatusCode_t responseCode, long remainingSubOperations, long completedSubOperations, long failedSubOperations, long warningSubOperations) {
    this(imebraJNI.new_CMoveResponse__SWIG_1(CMoveCommand.getCPtr(receivedCommand), receivedCommand, responseCode.swigValue(), remainingSubOperations, completedSubOperations, failedSubOperations, warningSubOperations), true);
  }

  public CMoveResponse(CMoveResponse source) {
    this(imebraJNI.new_CMoveResponse__SWIG_2(CMoveResponse.getCPtr(source), source), true);
  }

}
