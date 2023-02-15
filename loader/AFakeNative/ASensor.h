/*
 * based on frameworks/native/include/android/sensor.h,
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef ANDROID_SENSOR_H
#define ANDROID_SENSOR_H

#include <stdint.h>
#include <stdlib.h>

#include "ALooper.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A sensor event.
 */

/* NOTE: changes to these structs have to be backward compatible */
typedef struct ASensorVector {
    union {
        float v[3];
        struct {
            float x;
            float y;
            float z;
        };
        struct {
            float azimuth;
            float pitch;
            float roll;
        };
    };
    int8_t status;
    uint8_t reserved[3];
} ASensorVector;

typedef struct AMetaDataEvent {
    int32_t what;
    int32_t sensor;
} AMetaDataEvent;

typedef struct AUncalibratedEvent {
    union {
        float uncalib[3];
        struct {
            float x_uncalib;
            float y_uncalib;
            float z_uncalib;
        };
    };
    union {
        float bias[3];
        struct {
            float x_bias;
            float y_bias;
            float z_bias;
        };
    };
} AUncalibratedEvent;

typedef struct AHeartRateEvent {
    float bpm;
    int8_t status;
} AHeartRateEvent;

typedef struct ADynamicSensorEvent {
    int32_t  connected;
    int32_t  handle;
} ADynamicSensorEvent;

typedef struct AAdditionalInfoEvent {
    /**
     * Event type, such as ASENSOR_ADDITIONAL_INFO_BEGIN, ASENSOR_ADDITIONAL_INFO_END and others.
     * Refer to {@link ASENSOR_TYPE_ADDITIONAL_INFO} for the expected reporting behavior.
     */
    int32_t type;
    int32_t serial;
    union {
        int32_t data_int32[14];
        float   data_float[14];
    };
} AAdditionalInfoEvent;

typedef struct AHeadTrackerEvent {
    /**
     * The fields rx, ry, rz are an Euler vector (rotation vector, i.e. a vector
     * whose direction indicates the axis of rotation and magnitude indicates
     * the angle to rotate around that axis) representing the transform from
     * the (arbitrary, possibly slowly drifting) reference frame to the
     * head frame. Expressed in radians. Magnitude of the vector must be
     * in the range [0, pi], while the value of individual axes are
     * in the range [-pi, pi].
     */
    float rx;
    float ry;
    float rz;

    /**
     * The fields vx, vy, vz are an Euler vector (rotation vector) representing
     * the angular velocity of the head (relative to itself), in radians per
     * second. The direction of this vector indicates the axis of rotation, and
     * the magnitude indicates the rate of rotation.
     */
    float vx;
    float vy;
    float vz;

    /**
     * This value changes each time the reference frame is suddenly and
     * significantly changed, for example if an orientation filter algorithm
     * used for determining the orientation has had its state reset.
     */
    int32_t discontinuity_count;
} AHeadTrackerEvent;

typedef struct ALimitedAxesImuEvent {
    union {
        float calib[3];
        struct {
            float x;
            float y;
            float z;
        };
    };
    union {
        float supported[3];
        struct {
            float x_supported;
            float y_supported;
            float z_supported;
        };
    };
} ALimitedAxesImuEvent;

typedef struct ALimitedAxesImuUncalibratedEvent {
    union {
        float uncalib[3];
        struct {
            float x_uncalib;
            float y_uncalib;
            float z_uncalib;
        };
    };
    union {
        float bias[3];
        struct {
            float x_bias;
            float y_bias;
            float z_bias;
        };
    };
    union {
        float supported[3];
        struct {
            float x_supported;
            float y_supported;
            float z_supported;
        };
    };
} ALimitedAxesImuUncalibratedEvent;

typedef struct AHeadingEvent {
    /**
     * The direction in which the device is pointing relative to true north in
     * degrees. The value must be between 0.0 (inclusive) and 360.0 (exclusive),
     * with 0 indicating north, 90 east, 180 south, and 270 west.
     */
    float heading;
    /**
     * Accuracy is defined at 68% confidence. In the case where the underlying
     * distribution is assumed Gaussian normal, this would be considered one
     * standard deviation. For example, if the heading returns 60 degrees, and
     * accuracy returns 10 degrees, then there is a 68 percent probability of
     * the true heading being between 50 degrees and 70 degrees.
     */
    float accuracy;
} AHeadingEvent;

// LINT.IfChange
/**
 * Information that describes a sensor event, refer to
 * <a href="/reference/android/hardware/SensorEvent">SensorEvent</a> for additional
 * documentation.
 *
 * NOTE: changes to this struct has to be backward compatible and reflected in
 * sensors_event_t
 */
typedef struct ASensorEvent {
    int32_t version; /* sizeof(struct ASensorEvent) */
    int32_t sensor;  /** The sensor that generates this event */
    int32_t type;    /** Sensor type for the event, such as {@link ASENSOR_TYPE_ACCELEROMETER} */
    int32_t reserved0; /** do not use */
    /**
     * The time in nanoseconds at which the event happened, and its behavior
     * is identical to <a href="/reference/android/hardware/SensorEvent#timestamp">
     * SensorEvent::timestamp</a> in Java API.
     */
    int64_t timestamp;
    union {
        union {
            float           data[16];
            ASensorVector   vector;
            ASensorVector   acceleration;
            ASensorVector   gyro;
            ASensorVector   magnetic;
            float           temperature;
            float           distance;
            float           light;
            float           pressure;
            float           relative_humidity;
            AUncalibratedEvent uncalibrated_acceleration;
            AUncalibratedEvent uncalibrated_gyro;
            AUncalibratedEvent uncalibrated_magnetic;
            AMetaDataEvent meta_data;
            AHeartRateEvent heart_rate;
            ADynamicSensorEvent dynamic_sensor_meta;
            AAdditionalInfoEvent additional_info;
            AHeadTrackerEvent head_tracker;
            ALimitedAxesImuEvent limited_axes_imu;
            ALimitedAxesImuUncalibratedEvent limited_axes_imu_uncalibrated;
            AHeadingEvent heading;
        };
        union {
            uint64_t        data[8];
            uint64_t        step_counter;
        } u64;
    };

    uint32_t flags;
    int32_t reserved1[3];
} ASensorEvent;

struct ASensorManager;
/**
 * {@link ASensorManager} is an opaque type to manage sensors and
 * events queues.
 *
 * {@link ASensorManager} is a singleton that can be obtained using
 * ASensorManager_getInstance().
 *
 * This file provides a set of functions that uses {@link
 * ASensorManager} to access and list hardware sensors, and
 * create and destroy event queues:
 * - ASensorManager_getSensorList()
 * - ASensorManager_getDefaultSensor()
 * - ASensorManager_getDefaultSensorEx()
 * - ASensorManager_createEventQueue()
 * - ASensorManager_destroyEventQueue()
 */
typedef struct ASensorManager ASensorManager;


struct ASensorEventQueue;
/**
 * {@link ASensorEventQueue} is an opaque type that provides access to
 * {@link ASensorEvent} from hardware sensors.
 *
 * A new {@link ASensorEventQueue} can be obtained using ASensorManager_createEventQueue().
 *
 * This file provides a set of functions to enable and disable
 * sensors, check and get events, and set event rates on a {@link
 * ASensorEventQueue}.
 * - ASensorEventQueue_enableSensor()
 * - ASensorEventQueue_disableSensor()
 * - ASensorEventQueue_hasEvents()
 * - ASensorEventQueue_getEvents()
 * - ASensorEventQueue_setEventRate()
 * - ASensorEventQueue_requestAdditionalInfoEvents()
 */
typedef struct ASensorEventQueue ASensorEventQueue;

struct ASensor;
/**
 * {@link ASensor} is an opaque type that provides information about
 * an hardware sensors.
 *
 * A {@link ASensor} pointer can be obtained using
 * ASensorManager_getDefaultSensor(),
 * ASensorManager_getDefaultSensorEx() or from a {@link ASensorList}.
 *
 * This file provides a set of functions to access properties of a
 * {@link ASensor}:
 * - ASensor_getName()
 * - ASensor_getVendor()
 * - ASensor_getType()
 * - ASensor_getResolution()
 * - ASensor_getMinDelay()
 * - ASensor_getFifoMaxEventCount()
 * - ASensor_getFifoReservedEventCount()
 * - ASensor_getStringType()
 * - ASensor_getReportingMode()
 * - ASensor_isWakeUpSensor()
 * - ASensor_getHandle()
 */
typedef struct ASensor ASensor;

/**
 * Enable the selected sensor at default sampling rate.
 *
 * Start event reports of a sensor to specified sensor event queue at a default rate.
 *
 * \param queue {@link ASensorEventQueue} for sensor event to be report to.
 * \param sensor {@link ASensor} to be enabled.
 *
 * \return 0 on success or a negative error code on failure.
 */
int ASensorEventQueue_enableSensor(ASensorEventQueue* queue, ASensor const* sensor);

/**
 * Disable the selected sensor.
 *
 * Stop event reports from the sensor to specified sensor event queue.
 *
 * \param queue {@link ASensorEventQueue} to be changed
 * \param sensor {@link ASensor} to be disabled
 * \return 0 on success or a negative error code on failure.
 */
int ASensorEventQueue_disableSensor(ASensorEventQueue* queue, ASensor const* sensor);


/**
 * Retrieve pending events in sensor event queue
 *
 * Retrieve next available events from the queue to a specified event array.
 *
 * \param queue {@link ASensorEventQueue} to get events from
 * \param events pointer to an array of {@link ASensorEvent}.
 * \param count max number of event that can be filled into array event.
 * \return number of events returned on success; negative error code when
 *         no events are pending or an error has occurred.
 *
 * Examples:
 *
 *     ASensorEvent event;
 *     ssize_t numEvent = ASensorEventQueue_getEvents(queue, &event, 1);
 *
 *     ASensorEvent eventBuffer[8];
 *     ssize_t numEvent = ASensorEventQueue_getEvents(queue, eventBuffer, 8);
 *
 */
ssize_t ASensorEventQueue_getEvents(ASensorEventQueue* queue, ASensorEvent* events, size_t count);

/**
 * Sets the delivery rate of events in microseconds for the given sensor.
 *
 * This function has to be called after {@link ASensorEventQueue_enableSensor}.
 * Note that this is a hint only, generally event will arrive at a higher
 * rate. It is an error to set a rate inferior to the value returned by
 * ASensor_getMinDelay().
 *
 * \param queue {@link ASensorEventQueue} to which sensor event is delivered.
 * \param sensor {@link ASensor} of which sampling rate to be updated.
 * \param usec sensor sampling period (1/sampling rate) in microseconds
 * \return 0 on sucess or a negative error code on failure.
 */
int ASensorEventQueue_setEventRate(ASensorEventQueue* queue, ASensor const* sensor, int32_t usec);

/**
 * Creates a new sensor event queue and associate it with a looper.
 *
 * "ident" is a identifier for the events that will be returned when
 * calling ALooper_pollOnce(). The identifier must be >= 0, or
 * ALOOPER_POLL_CALLBACK if providing a non-NULL callback.
 */
ASensorEventQueue* ASensorManager_createEventQueue(ASensorManager* manager,
                                                   ALooper* looper, int ident, ALooper_callbackFunc callback, void* data);

/**
 * Returns the default sensor for the given type, or NULL if no sensor
 * of that type exists.
 */
ASensor const* ASensorManager_getDefaultSensor(ASensorManager* manager, int type);

/**
 * Get a reference to the sensor manager. ASensorManager is a singleton
 * per package as different packages may have access to different sensors.
 *
 * Deprecated: Use ASensorManager_getInstanceForPackage(const char*) instead.
 *
 * Example:
 *
 *     ASensorManager* sensorManager = ASensorManager_getInstance();
 *
 */
ASensorManager* ASensorManager_getInstance();

#ifdef __cplusplus
};
#endif

#endif // ANDROID_SENSOR_H
