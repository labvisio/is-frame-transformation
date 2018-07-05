from __future__ import print_function
from is_wire.core import Channel, Subscription
from is_msgs.camera_pb2 import FrameTransformation
from is_msgs.common_pb2 import Tensor
import numpy as np
from google.protobuf.json_format import MessageToJson
import sys
import signal
import json

signal.signal(signal.SIGINT, lambda signal, frame: sys.exit(0))

if len(sys.argv) < 3:
    print("USAGE: ./xxxx <FROM> <HINTS> <TO>")
    sys.exit(0)

c = Channel(json.load(file("options.json"))["broker_uri"])
s = Subscription(c)

np.set_printoptions(precision=3, suppress=True)


def OnPose(msg, ctx):
    tf = msg.unpack(Tensor)
    T = np.matrix(tf.doubles).reshape(tf.shape.dims[0].size, tf.shape.dims[1].size)
    print(T, np.linalg.norm(T[:3]))


topic = "FrameTransformation.%s" % ".".join(sys.argv[1:])
s.subscribe(topic, OnPose)

c.listen()
