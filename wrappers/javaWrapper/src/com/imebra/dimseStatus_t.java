/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.12
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.imebra;

public enum dimseStatus_t {
  success,
  warning,
  failure,
  cancel,
  pending;

  public final int swigValue() {
    return swigValue;
  }

  public static dimseStatus_t swigToEnum(int swigValue) {
    dimseStatus_t[] swigValues = dimseStatus_t.class.getEnumConstants();
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (dimseStatus_t swigEnum : swigValues)
      if (swigEnum.swigValue == swigValue)
        return swigEnum;
    throw new IllegalArgumentException("No enum " + dimseStatus_t.class + " with value " + swigValue);
  }

  @SuppressWarnings("unused")
  private dimseStatus_t() {
    this.swigValue = SwigNext.next++;
  }

  @SuppressWarnings("unused")
  private dimseStatus_t(int swigValue) {
    this.swigValue = swigValue;
    SwigNext.next = swigValue+1;
  }

  @SuppressWarnings("unused")
  private dimseStatus_t(dimseStatus_t swigEnum) {
    this.swigValue = swigEnum.swigValue;
    SwigNext.next = this.swigValue+1;
  }

  private final int swigValue;

  private static class SwigNext {
    private static int next = 0;
  }
}

