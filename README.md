Frame Transformation Service
==================

Allow users to query/track changes on transformations between coordinate frames.

RPCs
------
| Service | Request | Reply | Description | 
| ------- | ------- | ------| ----------- |
| FrameTransformation.GetCalibration | [GetCalibrationRequest] | [GetCalibrationReply] | Given a list of camera ids returns a list of the corresponding calibrations |


Streams
---------
| Name | Input (Topic/Message) | Output (Topic/Message) | Description | 
| ---- | --------------------- | ---------------------- | ----------- |
| FrameTransformation.Watch | **(ANY).FrameTransformations** [FrameTransformations] | **FrameTransformation.(ID)...** [FrameTransformation] | Consumes messages from topics which end in ".FrameTransformations" storing all the transformations in the message. Users can then watch/track transformation updates by subscribing to a topic using the following pattern: *FrameTransformation.(ID1).(ID2).(IDN)*. For instance, to get updates for the transformation between the frames with id 100 and 1000 subscribe to "FrameTransformation.100.1000". Hints can be passed to the service by simply appending more IDs, "FrameTransformation.100.0.1000" will be the transformation from 100 to 1000 passing through 0.


[FrameTransformations]: https://github.com/labviros/is-msgs/blob/modern-cmake/docs/README.md#is.vision.FrameTransformations
[FrameTransformation]: https://github.com/labviros/is-msgs/blob/modern-cmake/docs/README.md#is.vision.FrameTransformation
[GetCalibrationReply]: https://github.com/labviros/is-msgs/blob/modern-cmake/docs/README.md#is.vision.GetCalibrationReply
[GetCalibrationRequest]: https://github.com/labviros/is-msgs/blob/modern-cmake/docs/README.md#is.vision.GetCalibrationRequest
