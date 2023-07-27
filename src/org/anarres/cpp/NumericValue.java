/*
 * Anarres C Preprocessor
 * Copyright (c) 2007-2015, Shevek
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied.  See the License for the specific language governing
 * permissions and limitations under the License.
 */
package org.anarres.cpp;

import javax.annotation.CheckForNull;
import javax.annotation.Nonnegative;
import javax.annotation.Nonnull;

public class NumericValue extends Number {

    public static final int F_UNSIGNED = 1;
    public static final int F_INT = 2;
    public static final int F_LONG = 4;
    public static final int F_LONGLONG = 8;
    public static final int F_FLOAT = 16;
    public static final int F_DOUBLE = 32;

    public static final int FF_SIZE = F_INT | F_LONG | F_LONGLONG | F_FLOAT | F_DOUBLE;

    private final int base;
    private final String integer;
    private String fraction;
    private int expbase = 0;
    private String exponent;

    public NumericValue(@Nonnegative int base, @Nonnull String integer) {
        this.base = base;
        this.integer = integer;
    }

    @Nonnegative
    public int getBase() {
        return base;
    }

    @Nonnull
    public String getIntegerPart() {
        return integer;
    }

    @CheckForNull
    public String getFractionalPart() {
        return fraction;
    }

    void setFractionalPart(@Nonnull String fraction) {
        this.fraction = fraction;
    }

    @CheckForNull
    public String getExponent() {
        return exponent;
    }

    void setExponent(@Nonnegative int expbase, @Nonnull String exponent) {
        this.expbase = expbase;
        this.exponent = exponent;
    }

    void setFlags(int flags) {
    }

    private int exponentValue() {
        return Integer.parseInt(exponent, 10);
    }

    @Override
    public int intValue() {
        int v = integer.isEmpty() ? 0 : Integer.parseInt(integer, base);
        if (expbase == 2)
            v = v << exponentValue();
        else if (expbase != 0)
            v = (int) (v * Math.pow(expbase, exponentValue()));
        return v;
    }

    @Override
    public long longValue() {
        long v = integer.isEmpty() ? 0 : Long.parseLong(integer, base);
        if (expbase == 2)
            v = v << exponentValue();
        else if (expbase != 0)
            v = (long) (v * Math.pow(expbase, exponentValue()));
        return v;
    }

    @Override
    public float floatValue() {
        if (getBase() != 10)
            return longValue();
        return Float.parseFloat(toString());
    }

    @Override
    public double doubleValue() {
        if (getBase() != 10)
            return longValue();
        return Double.parseDouble(toString());
    }

    @Override
    public String toString() {
        StringBuilder buf = new StringBuilder();
        switch (base) {
            case 8:
                buf.append('0');
                break;
            case 10:
                break;
            case 16:
                buf.append("0x");
                break;
            case 2:
                buf.append('b');
                break;
            default:
                buf.append("[base-").append(base).append("]");
                break;
        }
        buf.append(getIntegerPart());
        if (getFractionalPart() != null)
            buf.append('.').append(getFractionalPart());
        if (getExponent() != null) {
            buf.append(base > 10 ? 'p' : 'e');
            buf.append(getExponent());
        }
        return buf.toString();
    }
}
