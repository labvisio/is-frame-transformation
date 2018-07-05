Frame Conversion Service
==================

Allow users to query/track changes on transformations between coordinate frames.

RPCs
------
| Service | Request | Reply | Description | 
| ------- | ------- | ------| ----------- |
| FrameConversion.GetCalibration | [GetCalibrationRequest] | [GetCalibrationReply] | Given a list of camera ids returns a list of the corresponding calibrations |


Streams
---------
| Name | Input (Topic/Message) | Output (Topic/Message) | Description | 
| ---- | --------------------- | ---------------------- | ----------- |
| FrameConversion.NewPose | **ANYTHING.Pose** [FrameTransformation] | **FrameTransformation.IDS...** [Tensor] | Consumes any message where the topic ends with ".Pose" and update the list of available transformations, updating users interested in that transformation if necessary. To use this service simply subscribe to the topic "FrameTransformation.IDS" where IDS are the ids of the desired transformation separated by dots. For instance, to get updates for the transformation between the frames with id 100 and 1000 subscribe to "FrameTransformation.100.1000". Hints can be passed to the service by simply appending more IDs, "FrameTransformation.100.0.1000" will be the transformation from 100 to 1000 passing through 0.


[FrameTransformation]: https://github.com/labviros/is-msgs/blob/modern-cmake/docs/README.md#is.vision.FrameTransformation
[Tensor]: https://github.com/labviros/is-msgs/blob/modern-cmake/docs/README.md#is.common.Tensor
[GetCalibrationReply]: https://github.com/labviros/is-msgs/blob/modern-cmake/docs/README.md#is.vision.GetCalibrationReply
[GetCalibrationRequest]: https://github.com/labviros/is-msgs/blob/modern-cmake/docs/README.md#is.vision.GetCalibrationRequest
