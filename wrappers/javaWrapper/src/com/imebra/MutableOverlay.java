/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.12
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.imebra;

public class MutableOverlay extends Overlay {
  private transient long swigCPtr;

  protected MutableOverlay(long cPtr, boolean cMemoryOwn) {
    super(imebraJNI.MutableOverlay_SWIGUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(MutableOverlay obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        imebraJNI.delete_MutableOverlay(swigCPtr);
      }
      swigCPtr = 0;
    }
    super.delete();
  }

  public MutableOverlay(overlayType_t overlayType, String overlaySubType, long firstFrame, int zeroBasedOriginX, int zeroBasedOriginY, String label, String description) {
    this(imebraJNI.new_MutableOverlay__SWIG_0(overlayType.swigValue(), overlaySubType, firstFrame, zeroBasedOriginX, zeroBasedOriginY, label, description), true);
  }

  public MutableOverlay(MutableOverlay source) {
    this(imebraJNI.new_MutableOverlay__SWIG_1(MutableOverlay.getCPtr(source), source), true);
  }

  public void setROIArea(long pixels) {
    imebraJNI.MutableOverlay_setROIArea(swigCPtr, this, pixels);
  }

  public void setROIMean(double mean) {
    imebraJNI.MutableOverlay_setROIMean(swigCPtr, this, mean);
  }

  public void setROIStandardDeviation(double standardDeviation) {
    imebraJNI.MutableOverlay_setROIStandardDeviation(swigCPtr, this, standardDeviation);
  }

  public void setImage(long frame, Image image) {
    imebraJNI.MutableOverlay_setImage(swigCPtr, this, frame, Image.getCPtr(image), image);
  }

}
