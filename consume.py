from __future__ import print_function
from is_wire.core import Channel, Subscription
from is_msgs.camera_pb2 import FrameTransformation
from is_msgs.common_pb2 import Tensor
import numpy as np
from google.protobuf.json_format import MessageToJson
import sys
import signal

signal.signal(signal.SIGINT, lambda signal, frame: sys.exit(0))

if len(sys.argv) != 3:
    print("USAGE: ./xxxx <FROM> <TO>")
    sys.exit(0)

c = Channel("amqp://10.10.2.15:30000")
s = Subscription(c)

np.set_printoptions(precision=3, suppress=True)


def OnPose(msg, ctx):
    tf = msg.unpack(Tensor)
    T = np.matrix(tf.doubles).reshape(tf.shape.dims[0].size, tf.shape.dims[1].size)
    print(T, np.linalg.norm(T[:3]))


s.subscribe("FrameTransformation.{}.{}".format(*sys.argv[1:3]), OnPose)

c.listen()
