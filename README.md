Frame Transformation Service
==================
[![Docker image tag](https://img.shields.io/docker/v/labvisio/is-frame-transformation?sort=semver&style=flat-square)](https://hub.docker.com/r/labvisio/is-frame-transformation/tags)
[![Docker image size](https://img.shields.io/docker/image-size/labvisio/is-frame-transformation?sort=semver&style=flat-square)](https://hub.docker.com/r/labvisio/is-frame-transformation)
[![Docker pulls](https://img.shields.io/docker/pulls/labvisio/is-frame-transformation?style=flat-square)](https://hub.docker.com/r/labvisio/is-frame-transformation)

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


[FrameTransformations]: https://github.com/labvisio/is-msgs/blob/modern-cmake/docs/README.md#is.vision.FrameTransformations
[FrameTransformation]: https://github.com/labvisio/is-msgs/blob/modern-cmake/docs/README.md#is.vision.FrameTransformation
[GetCalibrationReply]: https://github.com/labvisio/is-msgs/blob/modern-cmake/docs/README.md#is.vision.GetCalibrationReply
[GetCalibrationRequest]: https://github.com/labvisio/is-msgs/blob/modern-cmake/docs/README.md#is.vision.GetCalibrationRequest
