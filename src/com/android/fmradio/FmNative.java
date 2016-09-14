/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.fmradio;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import java.util.Locale;

/**
 * This class define FM native interface, will description FM native interface
 */
public class FmNative {

    private static final String TAG = FmNative.class.getSimpleName();
    private Handler mHandler;

    /**
     * Indicates that the FmReceiver is in an idle state. No resources are
     * allocated and power consumption is kept to a minimum.
     */
    public static final int STATE_IDLE = 0;

    /**
     * Indicates that the FmReceiver is allocating resources and preparing to
     * receive FM radio.
     */
    public static final int STATE_STARTING = 1;

    /**
     * Indicates that the FmReceiver is receiving FM radio. Note that the
     * FmReceiver is considered to be started even if it is receiving noise or
     * gets a signal with not good enough quality to consider a valid channel.
     */
    public static final int STATE_STARTED = 2;

    /**
     * Indicates that the FmReceiver has allocated resources and is ready to
     * instantly receive FM radio.
     */
    public static final int STATE_PAUSED = 3;

    /**
     * Indicates that the FmReceiver is scanning. FM radio will not be received
     * in this state.
     */
    public static final int STATE_SCANNING = 4;

    public int CURRENT_STATE = STATE_IDLE;
    
    private FmBand mFmBand;

    private int[] frequency = null;
    
    public FmNative(Handler handler) {
        this.mHandler = handler;
    }

    public void start(FmBand band) {
        mFmBand = band;
        _fm_receiver_start(mFmBand.getMinFrequency(), mFmBand.getMaxFrequency(), mFmBand
                .getDefaultFrequency(), mFmBand.getChannelOffset());
    }

    public boolean startAsync(FmBand band) {
        mFmBand = band;
        _fm_receiver_startAsync(mFmBand.getMinFrequency(), mFmBand.getMaxFrequency(), mFmBand
                .getDefaultFrequency(), mFmBand.getChannelOffset());
        try {
             while (getState() == STATE_IDLE) {
               Thread.sleep(700);
             }
        } catch (InterruptedException e) {
          e.printStackTrace();
          return false;
        }
        return true;
    }

    public boolean reset() {
        _fm_receiver_reset();
        return true;
    }

    public void pause() {
        _fm_receiver_pause();
    }

    public void resume() {
        _fm_receiver_resume();
    }

    public int getState() {
        return _fm_receiver_getState();
    }

    public boolean setFrequency(Float frequency) {
        _fm_receiver_setFrequency(FmUtils.fixFrequency(frequency));
        return true;
    }

    public int getFrequency() {
        return _fm_receiver_getFrequency();
    }

    public int scan(boolean isUp) {

        if (isUp) {
          _fm_receiver_scanUp();
        } else {
          _fm_receiver_scanDown();
        }
        try {
             while (getState() == STATE_SCANNING) {
               Thread.sleep(700);
             }
        } catch (InterruptedException e) {
          e.printStackTrace();
          stopScan();
          return mFmBand.getMaxFrequency();
        }

        return getFrequency();
    }

    public int[] startFullScan() {
        _fm_receiver_startFullScan();
        try {
             while (getState() == STATE_SCANNING) {
               Thread.sleep(700);
             }
        } catch (InterruptedException e) {
          e.printStackTrace();
          stopScan();
          return null;
        }
        return frequency;
    }

    public boolean stopScan() {
        _fm_receiver_stopScan();
        return true;
    }

    public boolean isRDSDataSupported() {
        return false;//_fm_receiver_isRDSDataSupported();
    }

    public boolean isTunedToValidChannel() {
        return _fm_receiver_isTunedToValidChannel();
    }

    public void setThreshold(int threshold) {
        _fm_receiver_setThreshold(threshold);
    }

    public int getThreshold() {
        return _fm_receiver_getThreshold();
    }

    public int getSignalStrength() {
        return _fm_receiver_getSignalStrength();
    }

    public boolean isPlayingInStereo() {
        return _fm_receiver_isPlayingInStereo();
    }

    public void setForceMono(boolean forceMono) {
        _fm_receiver_setForceMono(forceMono);
    }

    public void setAutomaticAFSwitching(boolean automatic) {
        _fm_receiver_setAutomaticAFSwitching(automatic);
    }

    public void setAutomaticTASwitching(boolean automatic) {
        _fm_receiver_setAutomaticTASwitching(automatic);
    }

    public boolean sendExtraCommand(String command, String[] extras) {
        return _fm_receiver_sendExtraCommand(command, extras);
    }

    static {
        System.loadLibrary("fmjni");
    }

    private void notifyOnStateChanged(int oldState, int newState) {
        Log.d(TAG, String.format(Locale.ENGLISH, "notifyOnStateChang oldstate: %s, newstate %s", oldState, newState));
    }

    private void notifyOnStarted() {
        Log.d(TAG, String.format(Locale.ENGLISH, "notifyOnStarted"));
    }

    private void notifyOnRDSDataFound(Bundle bundle, int frequency) {
        Log.d(TAG, String.format(Locale.ENGLISH, "notifyOnRDSDataFound, freq:%s", frequency));
        Log.d(TAG, String.format(Locale.ENGLISH, "notifyOnRDSDataFound, PSN:%s", bundle.get("PSN")));
        Message msg = mHandler.obtainMessage(Messages.RDS_CHANGED, bundle.get("PSN"));
        mHandler.sendMessage(msg);
    }

    private void notifyOnSignalStrengthChanged(int signalStrength) {
        Log.d(TAG, String.format(Locale.ENGLISH, "notifyOnSignalStrengthChanged, stringth:%s", signalStrength));
        Message msg = mHandler.obtainMessage(Messages.SIGNAL_CHANGED, signalStrength);
        mHandler.sendMessage(msg);
    }

    private void notifyOnScan(int frequency, int signalLevel, int scanDirection, boolean aborted) {
        Log.i(TAG, String.format(Locale.ENGLISH, "notifyOnScan, freq:%s", frequency));
        Message msg = mHandler.obtainMessage(Messages.SCAN_COMPLETE, frequency);
        mHandler.sendMessage(msg);
    }

    private void notifyOnFullScan(int[] frequency, int[] signalLevel, boolean aborted) {
        for(int i = 0; i < frequency.length; i++) {

          String freq = String.valueOf(frequency[i]);
          int length = freq.length();
          int fixedFreq;
          if (length == 7) { //1044000
             fixedFreq = Integer.parseInt(freq.substring(0, freq.length() - 3));
          } else { //973000
             fixedFreq = Integer.parseInt(freq.substring(0, freq.length() - 2));
          }
          frequency[i] = fixedFreq;
        }
        this.frequency = frequency;
    }

    private native void _fm_receiver_start(int minFreq, int maxFreq, int defaultFreq, int offset);

    private native void _fm_receiver_startAsync(int minFreq, int maxFreq, int defaultFreq,
                                                int offset);

    private native int _fm_receiver_reset();

    private native void _fm_receiver_pause();

    private native void _fm_receiver_resume();

    private native int _fm_receiver_getState();

    private native void _fm_receiver_setFrequency(int frequency);

    private native int _fm_receiver_getFrequency();

    private native void _fm_receiver_scanUp();

    private native void _fm_receiver_scanDown();

    private native void _fm_receiver_startFullScan();

    private native void _fm_receiver_stopScan();

    private native boolean _fm_receiver_isRDSDataSupported();

    private native boolean _fm_receiver_isTunedToValidChannel();

    private native void _fm_receiver_setThreshold(int threshold);

    private native int _fm_receiver_getThreshold();

    private native int _fm_receiver_getSignalStrength();

    private native boolean _fm_receiver_isPlayingInStereo();

    private native void _fm_receiver_setForceMono(boolean forceMono);

    private native void _fm_receiver_setAutomaticAFSwitching(boolean automatic);

    private native void _fm_receiver_setAutomaticTASwitching(boolean automatic);

    private native void _fm_receiver_setRDS(boolean receiveRDS);

    private native boolean _fm_receiver_sendExtraCommand(String command, String[] extras);
}
